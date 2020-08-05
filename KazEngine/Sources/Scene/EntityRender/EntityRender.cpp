#include "EntityRender.h"

namespace Engine
{
    void EntityRender::Clear()
    {
        Entity::RemoveListener(this);

        if(Vulkan::HasInstance()) {

            vkDeviceWaitIdle(Vulkan::GetDevice());

            // Descriptor Sets
            this->texture_descriptor.Clear();

            // Pipelines
            for(auto& render_group : this->render_groups) {
                render_group.graphics_pipeline.Clear();
                render_group.indirect_descriptor.Clear();
                render_group.compute_pipeline.Clear();
            }
            this->render_groups.clear();

            // Secondary graphics command buffers
            for(auto command_buffer : this->graphics_command_buffers)
                if(command_buffer != nullptr) vkFreeCommandBuffers(Vulkan::GetDevice(), this->graphics_command_pool, 1, &command_buffer);

            // Compute command pool
            if(this->compute_command_pool != nullptr) vkDestroyCommandPool(Vulkan::GetDevice(), this->compute_command_pool, nullptr);

            // Semaphores
            for(auto& semaphore : this->compute_semaphores)
                if(semaphore != nullptr) vkDestroySemaphore(Vulkan::GetDevice(), semaphore, nullptr);
        }

        this->need_graphics_update.clear();
        this->need_compute_update.clear();
        this->graphics_command_buffers.clear();
        this->compute_command_buffers.clear();
        this->render_groups.clear();
        this->skeleton_descriptor.Clear();
        this->texture_descriptor.Clear();
        this->compute_semaphores.clear();

        this->compute_command_pool = nullptr;
    }

    EntityRender::EntityRender(VkCommandPool command_pool) : graphics_command_pool(command_pool)
    {
        this->Clear();

        Entity::AddListener(this);

        uint32_t frame_count = Vulkan::GetConcurrentFrameCount();
        this->need_graphics_update.resize(frame_count, true);
        this->need_compute_update.resize(frame_count, true);

        this->graphics_command_buffers.resize(frame_count);
        if(!Vulkan::GetInstance().AllocateCommandBuffer(this->graphics_command_pool, this->graphics_command_buffers, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "EntityRender::EntityRender() => AllocateCommandBuffer [draw] : Failed" << std::endl;
            #endif
            return;
        }

        if(!Vulkan::GetInstance().CreateCommandPool(this->compute_command_pool, Vulkan::GetInstance().GetComputeQueue().index)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "EntityRender::EntityRender() => CreateCommandPool [compute] : Failed" << std::endl;
            #endif
            return;
        }

        this->compute_command_buffers.resize(frame_count);
        if(!Vulkan::GetInstance().AllocateCommandBuffer(this->compute_command_pool, this->compute_command_buffers)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "EntityRender::EntityRender() => AllocateCommandBuffer [compute] : Failed" << std::endl;
            #endif
            return;
        }

        this->compute_semaphores.resize(frame_count);
        for(uint8_t i=0; i<frame_count; i++) {
            if(!Vulkan::GetInstance().CreateSemaphore(this->compute_semaphores[i])) {
                #if defined(DISPLAY_LOGS)
                std::cout << "EntityRender::EntityRender() => CreateSemaphore [compute] : Failed" << std::endl;
                #endif
                return;
            }
        }

        this->SetupDescriptorSets();
        this->SetupPipelines();
    }

    bool EntityRender::SetupDescriptorSets()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        uint8_t instance_count = DataBank::GetManagedBuffer().GetInstanceCount();

        /////////////
        // Texture //
        /////////////

        if(!this->texture_descriptor.PrepareBindlessTexture(1)) return false;

        //////////////
        // Skeleton //
        //////////////

        bindings = {
            DescriptorSet::CreateSimpleBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT),
            DescriptorSet::CreateSimpleBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT),
            DescriptorSet::CreateSimpleBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT),
            DescriptorSet::CreateSimpleBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
        };

        if(!this->skeleton_descriptor.Prepare(bindings, instance_count)) return false;

        this->skeleton_bones_chunk = DataBank::GetManagedBuffer().ReserveChunk(SIZE_MEGABYTE(5));
        this->skeleton_offsets_ids_chunk = DataBank::GetManagedBuffer().ReserveChunk(SIZE_KILOBYTE(1));
        this->skeleton_offsets_chunk = DataBank::GetManagedBuffer().ReserveChunk(SIZE_MEGABYTE(1));
        this->skeleton_animations_chunk = DataBank::GetManagedBuffer().ReserveChunk(SIZE_KILOBYTE(1));

        if(this->skeleton_bones_chunk == nullptr
            || this->skeleton_offsets_chunk == nullptr
            || this->skeleton_offsets_ids_chunk == nullptr
            || this->skeleton_animations_chunk == nullptr)
            return false;

        for(uint8_t i=0; i<instance_count; i++) {
            uint32_t id = this->skeleton_descriptor.Allocate({
                DataBank::GetManagedBuffer().GetBufferInfos(this->skeleton_bones_chunk, i),
                DataBank::GetManagedBuffer().GetBufferInfos(this->skeleton_offsets_ids_chunk, i),
                DataBank::GetManagedBuffer().GetBufferInfos(this->skeleton_offsets_chunk, i),
                DataBank::GetManagedBuffer().GetBufferInfos(this->skeleton_animations_chunk, i)
            });

            if(id == UINT32_MAX) return false;
        }

        return true;
    }

    bool EntityRender::SetupPipelines()
    {
        std::vector<VkVertexInputBindingDescription> vertex_binding_description;
        std::vector<VkVertexInputAttributeDescription> vertex_attribute_description;
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        VkPipelineShaderStageCreateInfo compute_shader_stage;
        Vulkan::PIPELINE graphics_pipeline;
        Vulkan::PIPELINE compute_pipeline;
        DescriptorSet indirect_descriptor;
        uint8_t instance_count = DataBank::GetManagedBuffer().GetInstanceCount();

        ///////////////////
        // Textured Mesh //
        ///////////////////

        shader_stages = {
            Vulkan::GetInstance().LoadShaderModule("./Shaders/textured_model.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            Vulkan::GetInstance().LoadShaderModule("./Shaders/textured_model.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        vertex_attribute_description = Vulkan::CreateVertexInputDescription({
            {Vulkan::POSITION, Vulkan::UV},
            {Vulkan::MATRIX}
        }, vertex_binding_description);

        VkPushConstantRange push_constant;
        push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        push_constant.offset = 0;
        push_constant.size = sizeof(LoadedMesh::PUSH_CONSTANT_MATERIAL);

        auto camera_layout = Camera::GetInstance().GetDescriptorSet().GetLayout();
        auto texture_layout = this->texture_descriptor.GetLayout();
        
        bool success = Vulkan::GetInstance().CreatePipeline(
            true, {texture_layout, camera_layout},
            shader_stages, vertex_binding_description, vertex_attribute_description, {push_constant}, graphics_pipeline
        );

        for(auto& stage : shader_stages) vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);
        shader_stages.clear();
        
        if(!success) return false;

        if(!indirect_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(LODGroup::INDIRECT_COMMAND)}
        }, instance_count)) return false;

        auto indirect_layout = indirect_descriptor.GetLayout();
        auto entity_layout = StaticEntity::GetMatrixDescriptor().GetLayout();
        auto lod_layout = LODGroup::GetDescriptor().GetLayout();

        compute_shader_stage = Vulkan::GetInstance().LoadShaderModule("./Shaders/cull_lod.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
        success = Vulkan::GetInstance().CreateComputePipeline(compute_shader_stage, {camera_layout, entity_layout, indirect_layout, lod_layout}, {}, compute_pipeline);
        
        vkDestroyShaderModule(Vulkan::GetDevice(), compute_shader_stage.module, nullptr);

        if(!success) {
            indirect_descriptor.Clear();
            return false;
        }

        this->render_groups.push_back({
            graphics_pipeline, compute_pipeline,
            Model::Mesh::RENDER_POSITION | Model::Mesh::RENDER_UV | Model::Mesh::RENDER_TEXTURE,
            {StaticEntity::GetMatrixDescriptor().GetChunk()},
            std::move(indirect_descriptor)
        });

        indirect_descriptor.Clear();

        ///////////////////
        // Animated Mesh //
        ///////////////////

        shader_stages = {
            Vulkan::GetInstance().LoadShaderModule("./Shaders/dynamic_model.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            Vulkan::GetInstance().LoadShaderModule("./Shaders/textured_model.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        vertex_attribute_description = Vulkan::CreateVertexInputDescription({
            {Vulkan::POSITION, Vulkan::UV, Vulkan::BONE_WEIGHTS, Vulkan::BONE_IDS},
            {Vulkan::MATRIX},
            {Vulkan::UINT_ID, Vulkan::UINT_ID}
        }, vertex_binding_description);

        auto skeleton_layout = this->skeleton_descriptor.GetLayout();

        success = Vulkan::GetInstance().CreatePipeline(
            true, {texture_layout, camera_layout, skeleton_layout},
            shader_stages, vertex_binding_description, vertex_attribute_description, {push_constant}, graphics_pipeline
        );

        for(auto& stage : shader_stages) vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);
        shader_stages.clear();
        
        if(!success) return false;

        if(!indirect_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(LODGroup::INDIRECT_COMMAND)}
        }, instance_count)) return false;

        indirect_layout = indirect_descriptor.GetLayout();
        auto animation_layout = DynamicEntity::GetAnimationDescriptor().GetLayout();

        compute_shader_stage = Vulkan::GetInstance().LoadShaderModule("./Shaders/cull_lod_anim.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
        success = Vulkan::GetInstance().CreateComputePipeline(compute_shader_stage, {camera_layout, entity_layout, indirect_layout, animation_layout, lod_layout}, {}, compute_pipeline);
        
        vkDestroyShaderModule(Vulkan::GetDevice(), compute_shader_stage.module, nullptr);

        if(!success) {
            indirect_descriptor.Clear();
            return false;
        }

        this->render_groups.push_back({
            graphics_pipeline, compute_pipeline,
            Model::Mesh::RENDER_POSITION | Model::Mesh::RENDER_UV | Model::Mesh::RENDER_TEXTURE | Model::Mesh::RENDER_SKELETON,
            { DynamicEntity::GetMatrixDescriptor().GetChunk(), DynamicEntity::GetAnimationDescriptor().GetChunk() },
            std::move(indirect_descriptor)
        });

        indirect_descriptor.Clear();

        return true;
    }

    bool EntityRender::LoadSkeleton(std::string name)
    {
        // Skeleton already loaded
        if(this->skeletons.count(name) > 0) return true;

        // Skeleton is not in data bank
        if(!DataBank::HasSkeleton(name)) return false;

        /////////////////////////
        //   Mesh offsets SBO  //
        // Mesh offets ids SBO //
        /////////////////////////

        // Output buffers
        std::vector<char> offsets_sbo;
        std::vector<char> offsets_ids;

        // Build buffers
        uint32_t sbo_alignment = static_cast<uint32_t>(Vulkan::GetDeviceLimits().minStorageBufferOffsetAlignment);
        DataBank::GetSkeleton(name).BuildBoneOffsetsSBO(offsets_sbo, offsets_ids, this->skeletons[name].dynamic_offsets, sbo_alignment);

        // Write to GPU memory
        DataBank::GetManagedBuffer().WriteData(offsets_sbo.data(), offsets_sbo.size(), this->skeleton_offsets_chunk->offset);
        DataBank::GetManagedBuffer().WriteData(offsets_ids.data(), offsets_ids.size(), this->skeleton_offsets_ids_chunk->offset);

        ////////////////////
        // Animations SBO //
        ////////////////////

        auto const animations = DataBank::GetSkeleton(name).ListAnimations();
        uint32_t animation_offset = 0;
        uint32_t frame_id = 0;

        for(uint8_t i=0; i<animations.size(); i++) {

            DataBank::BAKED_ANIMATION baked_animation;
            baked_animation.animation_id = i;
            baked_animation.duration = DataBank::GetSkeleton(name).GetAnimationTotalDuration(animations[i].name);

            // Build buffer
            std::vector<char> skeleton_sbo;
            uint32_t bone_count;
            DataBank::GetSkeleton(name).BuildAnimationSBO(skeleton_sbo, animations[i].name, baked_animation.frame_count, bone_count, 30);
            
            // Write the bone count before frame offsets
            if(!i) DataBank::GetManagedBuffer().WriteData(&bone_count, sizeof(uint32_t), this->skeleton_animations_chunk->offset);

            // Write the frame offset
            DataBank::GetManagedBuffer().WriteData(&frame_id, sizeof(uint32_t), this->skeleton_animations_chunk->offset + sizeof(uint32_t) * (i + 1));

            // Write animation to GPU memory
            DataBank::GetManagedBuffer().WriteData(skeleton_sbo.data(), skeleton_sbo.size(), this->skeleton_bones_chunk->offset + animation_offset);

            frame_id += static_cast<uint32_t>(skeleton_sbo.size() / sizeof(Maths::Matrix4x4));
            animation_offset += Vulkan::GetInstance().ComputeStorageBufferAlignment(static_cast<uint32_t>(skeleton_sbo.size()));
            DataBank::AddAnimation(baked_animation, animations[i].name);
        }

        // Success
        return true;
    }

    bool EntityRender::LoadTexture(std::string name)
    {
        // Texture not in data bank
        if(!DataBank::HasTexture(name)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "EntityRender::LoadTexture() => Texture[" + name + "] : Not in data bank" << std::endl;
            #endif
            return false;
        }

        uint32_t texture_id = this->texture_descriptor.AllocateTexture(DataBank::GetTexture(name));
        if(texture_id >= 0) {
            this->textures[name] = texture_id;
            return true;
        }

        return false;
    }

    bool EntityRender::DRAWABLE_BIND::AddInstance(RENDER_GOURP& group, Entity& entity)
    {
        ENTITY_MESH_CHUNK emc;
        emc.chunk = group.indirect_descriptor.ReserveRange(sizeof(LODGroup::INDIRECT_COMMAND));

        if(emc.chunk == nullptr) {
            if(!group.indirect_descriptor.ResizeChunk(group.indirect_descriptor.GetChunk()->range
                                                                        + sizeof(LODGroup::INDIRECT_COMMAND) * 10)) {
                #if defined(DISPLAY_LOGS)
                std::cout << "EntityRender::AddEntity : Not enough memory" << std::endl;
                #endif
                return false;
            }else{
                emc.chunk = group.indirect_descriptor.ReserveRange(sizeof(LODGroup::INDIRECT_COMMAND));
                if(emc.chunk == nullptr) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "EntityRender::AddEntity : Not enough memory" << std::endl;
                    #endif
                    return false;
                }
            }
        }

        emc.entity = &entity;
        emc.instance_id = entity.GetInstanceId();
        LODGroup::INDIRECT_COMMAND indirect = {}; // = this->mesh.GetIndirectCommand(emc.instance_id);
        indirect.firstInstance = emc.instance_id;
        indirect.lodIndex = lod->GetLodIndex();
        group.indirect_descriptor.WriteData(&indirect, sizeof(LODGroup::INDIRECT_COMMAND), static_cast<uint32_t>(emc.chunk->offset));
        this->entities.push_back(emc);

        return true;
    }

    void EntityRender::Refresh(uint8_t frame_index)
    {
        this->need_graphics_update[frame_index] = true;
        this->need_compute_update[frame_index] = true;
        vkResetCommandBuffer(this->graphics_command_buffers[frame_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        vkResetCommandBuffer(this->compute_command_buffers[frame_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    void EntityRender::UpdateDescriptorSet(uint8_t frame_index)
    {
        for(auto& render : this->render_groups) {
            if(render.indirect_descriptor.NeedUpdate(frame_index)) {
                this->need_compute_update[frame_index] = true;
                vkResetCommandBuffer(this->compute_command_buffers[frame_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
                render.indirect_descriptor.Update(frame_index);
            }
        }
    }

    void EntityRender::AddLOD(Entity& entity, std::shared_ptr<LODGroup> lod)
    {
        for(uint8_t i=0; i<this->render_groups.size(); i++) {

            if(lod->GetRenderMask() != this->render_groups[i].mask) continue;

            ////////////////////////////
            // Search for loaded LOD  //
            ////////////////////////////

            for(auto& drawable_bind : this->render_groups[i].drawables) {
                if(drawable_bind.lod == lod) {

                    drawable_bind.AddInstance(this->render_groups[i], entity);

                    for(uint8_t i=0; i<this->need_graphics_update.size(); i++) this->need_graphics_update[i] = true;
                    for(uint8_t i=0; i<this->need_compute_update.size(); i++) this->need_compute_update[i] = true;
                    return;
                }
            }

            // No loaded mesh found
            DRAWABLE_BIND new_bind;
            new_bind.lod = lod;

            //////////////////
            // Load Texture //
            //////////////////

            if(lod->GetRenderMask() & Model::Mesh::RENDER_TEXTURE) {
                for(auto& material : lod->GetMeshMaterials()) {
                    if(DataBank::HasMaterial(material.first)) {
                        if(!DataBank::GetMaterial(material.first).texture.empty()) {
                            if(!this->textures.count(DataBank::GetMaterial(material.first).texture)
                                && !this->LoadTexture(DataBank::GetMaterial(material.first).texture))
                                    return;
                                    
                            new_bind.texture_id = this->textures[DataBank::GetMaterial(material.first).texture];
                        }
                    }else{
                        #if defined(DISPLAY_LOGS)
                        std::cout << "Dynamics::AddEntity() => Material[" + material.first + "] : Not in data bank" << std::endl;
                        #endif
                        return;
                    }
                }
            }

            ///////////////
            // Load LOD  //
            ///////////////

            if(!new_bind.lod->Build(this->textures)) {//mesh, this->textures)) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Dynamics::AddEntity() => Drawable.Load(LOD) : Failed" << std::endl;
                #endif
                return;
            }

            ///////////////////
            // Load Skeleton //
            ///////////////////

            if(lod->GetRenderMask() & Model::Mesh::RENDER_SKELETON) {

                std::string const& skeleton = lod->GetMeshSkeleton();
                if(!this->skeletons.count(skeleton) && !this->LoadSkeleton(skeleton)) return;

                new_bind.dynamic_offsets.insert(new_bind.dynamic_offsets.end(), {
                    this->skeletons[skeleton].skeleton_dynamic_offset,
                    this->skeletons[skeleton].dynamic_offsets[lod->GetMesh()->name].first,
                    this->skeletons[skeleton].dynamic_offsets[lod->GetMesh()->name].second,
                    this->skeletons[skeleton].animations_data_dynamic_offset
                });
                new_bind.has_skeleton = true;
            }else{
                new_bind.has_skeleton = false;
            }

            ///////////////////////
            // Setup loaded mesh //
            ///////////////////////

            new_bind.AddInstance(this->render_groups[i], entity);

            // Add loaded mesh
            this->render_groups[i].drawables.push_back(std::move(new_bind));

            // Finish
            for(uint8_t i=0; i<this->need_graphics_update.size(); i++) this->need_graphics_update[i] = true;
            for(uint8_t i=0; i<this->need_compute_update.size(); i++) this->need_compute_update[i] = true;
            return;
        }
    }

    /*void EntityRender::AddMesh(Entity& entity, std::shared_ptr<Model::Mesh> mesh)
    {
        for(uint8_t i=0; i<this->render_groups.size(); i++) {
            for(auto mesh : entity.GetMeshes()) {

                if(mesh->render_mask != this->render_groups[i].mask) continue;

                ////////////////////////////
                // Search for loaded mesh //
                ////////////////////////////

                for(auto& drawable_bind : this->render_groups[i].drawables) {
                    if(drawable_bind.mesh.IsSameMesh(mesh)) {

                        drawable_bind.AddInstance(this->render_groups[i], entity);

                        for(uint8_t i=0; i<this->need_graphics_update.size(); i++) this->need_graphics_update[i] = true;
                        for(uint8_t i=0; i<this->need_compute_update.size(); i++) this->need_compute_update[i] = true;
                        return;
                    }
                }

                // No loaded mesh found
                DRAWABLE_BIND new_bind;

                //////////////////
                // Load Texture //
                //////////////////

                if(mesh->render_mask & Model::Mesh::RENDER_TEXTURE) {
                    for(auto& material : mesh->materials) {
                        if(DataBank::HasMaterial(material.first)) {
                            if(!DataBank::GetMaterial(material.first).texture.empty()) {
                                if(!this->textures.count(DataBank::GetMaterial(material.first).texture)
                                    && !this->LoadTexture(DataBank::GetMaterial(material.first).texture))
                                        return;
                                    
                                new_bind.texture_id = this->textures[DataBank::GetMaterial(material.first).texture];
                            }
                        }else{
                            #if defined(DISPLAY_LOGS)
                            std::cout << "Dynamics::AddEntity() => Material[" + material.first + "] : Not in data bank" << std::endl;
                            #endif
                            return;
                        }
                    }
                }

                ///////////////
                // Load Mesh //
                ///////////////

                if(!new_bind.mesh.Load(mesh, this->textures)) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "Dynamics::AddEntity() => Drawable.Load(" + mesh->name + ") : Failed" << std::endl;
                    #endif
                    return;
                }

                ///////////////////
                // Load Skeleton //
                ///////////////////

                if(mesh->render_mask & Model::Mesh::RENDER_SKELETON) {

                    if(!this->skeletons.count(mesh->skeleton) && !this->LoadSkeleton(mesh->skeleton)) return;

                    new_bind.dynamic_offsets.insert(new_bind.dynamic_offsets.end(), {
                        this->skeletons[mesh->skeleton].skeleton_dynamic_offset,
                        this->skeletons[mesh->skeleton].dynamic_offsets[mesh->name].first,
                        this->skeletons[mesh->skeleton].dynamic_offsets[mesh->name].second,
                        this->skeletons[mesh->skeleton].animations_data_dynamic_offset
                    });
                    new_bind.has_skeleton = true;
                }else{
                    new_bind.has_skeleton = false;
                }

                ///////////////////////
                // Setup loaded mesh //
                ///////////////////////

                new_bind.AddInstance(this->render_groups[i], entity);

                // Add loaded mesh
                this->render_groups[i].drawables.push_back(std::move(new_bind));

                // Finish
                for(uint8_t i=0; i<this->need_graphics_update.size(); i++) this->need_graphics_update[i] = true;
                for(uint8_t i=0; i<this->need_compute_update.size(); i++) this->need_compute_update[i] = true;
                return;
            }
        }
    }*/

    VkSemaphore EntityRender::SubmitComputeShader(uint8_t frame_index)
    {
        if(!this->BuildComputeCommandBuffer(frame_index)) return nullptr;

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pCommandBuffers = &this->compute_command_buffers[frame_index];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &this->compute_semaphores[frame_index];
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;

        VkResult result = vkQueueSubmit(Vulkan::GetComputeQueue().handle, 1, &submit_info, nullptr);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DrawScene => vkQueueSubmit : Failed" << std::endl;
            #endif
            return nullptr;
        }

        return this->compute_semaphores[frame_index];
    }

    bool EntityRender::BuildComputeCommandBuffer(uint8_t frame_index)
    {
        if(!this->need_compute_update[frame_index]) return true;
        this->need_compute_update[frame_index] = false;

        VkCommandBuffer command_buffer = this->compute_command_buffers[frame_index];

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildComputeCommandBuffer => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        for(uint8_t i=0; i<this->render_groups.size(); i++) {
		    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->render_groups[i].compute_pipeline.handle);

            std::vector<VkDescriptorSet> bind_descriptor_sets;

            if(this->render_groups[i].mask & Model::Mesh::RENDER_SKELETON) {
                bind_descriptor_sets = {
                    Camera::GetInstance().GetDescriptorSet().Get(frame_index),
                    DynamicEntity::GetMatrixDescriptor().Get(frame_index),
                    this->render_groups[i].indirect_descriptor.Get(frame_index),
                    DynamicEntity::GetAnimationDescriptor().Get(frame_index),
                    LODGroup::GetDescriptor().Get(frame_index)
                };
            }else{
                bind_descriptor_sets = {
                    Camera::GetInstance().GetDescriptorSet().Get(frame_index),
                    StaticEntity::GetMatrixDescriptor().Get(frame_index),
                    this->render_groups[i].indirect_descriptor.Get(frame_index),
                    LODGroup::GetDescriptor().Get(frame_index)
                };
            }

            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->render_groups[i].compute_pipeline.layout, 0,
                                    static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(), 0, nullptr);

            uint32_t count = static_cast<uint32_t>(this->render_groups[i].indirect_descriptor.GetChunk()->range / sizeof(LODGroup::INDIRECT_COMMAND));
		    vkCmdDispatch(command_buffer, count, 1, 1);
        }

		vkEndCommandBuffer(command_buffer);

        return true;
    }

    VkCommandBuffer EntityRender::GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer)
    {
        VkCommandBuffer command_buffer = this->graphics_command_buffers[frame_index];
        if(!this->need_graphics_update[frame_index]) return command_buffer;
        this->need_graphics_update[frame_index] = false;

        VkCommandBufferInheritanceInfo inheritance_info = {};
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;
        inheritance_info.framebuffer = framebuffer;
        inheritance_info.renderPass = Vulkan::GetRenderPass();

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        command_buffer_begin_info.pInheritanceInfo = &inheritance_info;

        VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildRenderPass[Dynamics] => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        auto& surface = Vulkan::GetDrawSurface();
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(surface.width);
        viewport.height = static_cast<float>(surface.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = surface.width;
        scissor.extent.height = surface.height;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
        
        for(uint8_t i=0; i<this->render_groups.size(); i++) {

            if(!this->render_groups[i].drawables.size()) continue;

            auto pipeline = this->render_groups[i].graphics_pipeline;
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);

            std::vector<VkDescriptorSet> bind_descriptor_sets_1 = {
                Camera::GetInstance().GetDescriptorSet().Get(frame_index)
            };

            if(this->render_groups[i].mask & Model::Mesh::RENDER_TEXTURE)
                bind_descriptor_sets_1.insert(bind_descriptor_sets_1.begin(), this->texture_descriptor.Get());


            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                                    static_cast<uint32_t>(bind_descriptor_sets_1.size()), bind_descriptor_sets_1.data(), 0, nullptr);

            for(auto& bind : this->render_groups[i].drawables) {

                if(bind.has_skeleton) {
                    std::vector<VkDescriptorSet> bind_descriptor_sets_2 = {this->skeleton_descriptor.Get(frame_index)};

                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->render_groups[i].graphics_pipeline.layout,
                                    static_cast<uint32_t>(bind_descriptor_sets_1.size()),
                                    static_cast<uint32_t>(bind_descriptor_sets_2.size()), bind_descriptor_sets_2.data(),
                                    static_cast<uint32_t>(bind.dynamic_offsets.size()), bind.dynamic_offsets.data());
                }

                bind.lod->Render(command_buffer, DataBank::GetManagedBuffer().GetBuffer(frame_index).handle,
                                 pipeline.layout, static_cast<uint32_t>(bind.entities.size()),
                                 this->render_groups[i].instance_buffer_chunks,
                                 this->render_groups[i].indirect_descriptor.GetChunk()->offset);
            }
        }

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildMainRenderPass[Map] => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        // Succès
        return command_buffer;
    }
}