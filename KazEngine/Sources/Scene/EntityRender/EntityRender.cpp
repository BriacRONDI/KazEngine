#include "EntityRender.h"

namespace Engine
{
    void EntityRender::Clear()
    {
        if(Vulkan::HasInstance()) {

            vkDeviceWaitIdle(Vulkan::GetDevice());

            // Descriptor Sets
            // this->entities_descriptor.Clear();
            this->texture_descriptor.Clear();

            // Pipelines
            for(auto& render_group : this->render_groups) render_group.pipeline.Clear();
            this->render_groups.clear();

            // Secondary graphics command buffers
            for(auto command_buffer : this->command_buffers)
                if(command_buffer != nullptr) vkFreeCommandBuffers(Vulkan::GetDevice(), this->command_pool, 1, &command_buffer);
        }

        this->need_update.clear();
        this->command_buffers.clear();
        this->entities.clear();
        // this->entities_descriptor.Clear();
        this->render_groups.clear();
        this->skeleton_descriptor.Clear();
        this->texture_descriptor.Clear();
    }

    EntityRender::EntityRender(VkCommandPool command_pool) : command_pool(command_pool)
    {
        this->Clear();

        // Entity::UpdateUboSize();

        uint32_t frame_count = Vulkan::GetConcurrentFrameCount();
        this->command_buffers.resize(frame_count);
        this->need_update.resize(frame_count, true);

        if(!Vulkan::GetInstance().AllocateCommandBuffer(command_pool, this->command_buffers, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Dynamics::Dynamics() => AllocateCommandBuffer [draw] : Failed" << std::endl;
            #endif
        }

        this->SetupDescriptorSets();
        this->SetupPipelines();
    }

    bool EntityRender::SetupDescriptorSets()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        uint8_t instance_count = DataBank::GetManagedBuffer().GetInstanceCount();

        ////////////
        // Entity //
        ////////////

        /*if(!this->entities_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, SIZE_KILOBYTE(5)},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, Entity::GetUboSize() * 100}
        }, instance_count)) return false;

        this->entity_data_chunk = this->entities_descriptor.ReserveRange(Entity::GetUboSize() * 100, Vulkan::SboAlignment(), ENTITY_DATA_BINDING);*/
        this->instance_buffer_chunk = DataBank::GetManagedBuffer().ReserveChunk(sizeof(Maths::Matrix4x4) * 100);

        /////////////
        // Texture //
        /////////////

        if(!this->texture_descriptor.PrepareBindlessTexture(2)) return false;

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
        Vulkan::PIPELINE pipeline;

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

        auto texture_layout = this->texture_descriptor.GetLayout();
        auto camera_layout = Camera::GetInstance().GetDescriptorSet(0).GetLayout();
        // auto entities_layout = this->entities_descriptor.GetLayout();
        
        bool success = Vulkan::GetInstance().CreatePipeline(
            true, {texture_layout, camera_layout},
            shader_stages, vertex_binding_description, vertex_attribute_description, {push_constant}, pipeline
        );

        for(auto& stage : shader_stages) vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);
        shader_stages.clear();
        
        if(!success) return false;

        SkeletonEntity::InitilizeInstanceChunk();

        this->render_groups.push_back({
            pipeline,
            Model::Mesh::RENDER_POSITION | Model::Mesh::RENDER_UV | Model::Mesh::RENDER_TEXTURE,
            {Entity::static_data_chunk}
        });

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
            shader_stages, vertex_binding_description, vertex_attribute_description, {push_constant}, pipeline
        );

        for(auto& stage : shader_stages) vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);
        shader_stages.clear();
        
        if(!success) return false;

        this->render_groups.push_back({
            pipeline,
            Model::Mesh::RENDER_POSITION | Model::Mesh::RENDER_UV | Model::Mesh::RENDER_TEXTURE | Model::Mesh::RENDER_SKELETON,
            {SkeletonEntity::absolute_skeleton_data_chunk, SkeletonEntity::animation_data_chunk}
        });

        /////////////////
        // Debug Cross //
        /////////////////

        /*shader_stages = {
            Vulkan::GetInstance().LoadShaderModule("./Shaders/cross.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            Vulkan::GetInstance().LoadShaderModule("./Shaders/cross.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        vertex_attribute_description = Vulkan::CreateVertexInputDescription({{Vulkan::POSITION}}, vertex_binding_description);

        success = Vulkan::GetInstance().CreatePipeline(
            true, {camera_layout, entities_layout}, shader_stages,
            vertex_binding_description, vertex_attribute_description, {}, pipeline,
            VK_POLYGON_MODE_LINE, VK_PRIMITIVE_TOPOLOGY_LINE_LIST
        );

        for(auto& stage : shader_stages) vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);
        shader_stages.clear();
        
        if(!success) return false;
        
        this->render_groups.push_back({
            pipeline,
            Model::Mesh::RENDER_POSITION
        });*/

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

    bool EntityRender::AddEntity(Entity& entity)
    {
        auto meshes = entity.GetMeshes();
        if(meshes == nullptr) return false;

        for(uint8_t i=0; i<this->render_groups.size(); i++) {
            for(auto mesh : *entity.GetMeshes()) {

                if(mesh->render_mask != this->render_groups[i].mask) continue;

                ////////////////////////////
                // Search for loaded mesh //
                ////////////////////////////

                for(auto& drawable_bind : this->render_groups[i].drawables) {
                    if(drawable_bind.mesh.IsSameMesh(mesh)) {
                        
                        if(this->render_groups[i].indirect_commands_chunk == nullptr)
                            this->render_groups[i].indirect_commands_chunk = DataBank::GetManagedBuffer().ReserveChunk(sizeof(VkDrawIndirectCommand));

                        if(this->render_groups[i].indirect_commands_chunk == nullptr) {
                            #if defined(DISPLAY_LOGS)
                            std::cout << "EntityRender::AddEntity : Not enough memory" << std::endl;
                            #endif
                            return false;
                        }

                        ENTITY_MESH_CHUNK emc;
                        emc.chunk = this->render_groups[i].indirect_commands_chunk->ReserveRange(sizeof(VkDrawIndirectCommand));

                        if(emc.chunk == nullptr) {
                            bool relocated;
                            if(!DataBank::GetManagedBuffer().ResizeChunk(this->render_groups[i].indirect_commands_chunk,
                                                this->render_groups[i].indirect_commands_chunk->range + sizeof(VkDrawIndirectCommand) * 10, relocated)) {
                                #if defined(DISPLAY_LOGS)
                                std::cout << "EntityRender::AddEntity : Not enough memory" << std::endl;
                                #endif
                                return false;
                            }else{
                                emc.chunk = this->render_groups[i].indirect_commands_chunk->ReserveRange(sizeof(VkDrawIndirectCommand));
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
                        VkDrawIndirectCommand indirect = drawable_bind.mesh.GetIndirectCommand(emc.instance_id);
                        DataBank::GetManagedBuffer().WriteData(&indirect, sizeof(VkDrawIndirectCommand),
                                                               this->render_groups[i].indirect_commands_chunk->offset + emc.chunk->offset);
                        drawable_bind.entities.push_back(emc);

                        for(uint8_t i=0; i<this->need_update.size(); i++) this->need_update[i] = true;
                        this->entities.push_back(&entity);
                        return true;
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
                                        return false;
                                    
                                new_bind.texture_id = this->textures[DataBank::GetMaterial(material.first).texture];
                            }
                        }else{
                            #if defined(DISPLAY_LOGS)
                            std::cout << "Dynamics::AddEntity() => Material[" + material.first + "] : Not in data bank" << std::endl;
                            #endif
                            return false;
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
                    return false;
                }

                ///////////////////
                // Load Skeleton //
                ///////////////////

                if(mesh->render_mask & Model::Mesh::RENDER_SKELETON) {

                    if(!this->skeletons.count(mesh->skeleton) && !this->LoadSkeleton(mesh->skeleton)) return false;

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

                if(this->render_groups[i].indirect_commands_chunk == nullptr)
                    this->render_groups[i].indirect_commands_chunk = DataBank::GetManagedBuffer().ReserveChunk(sizeof(VkDrawIndirectCommand));

                if(this->render_groups[i].indirect_commands_chunk == nullptr) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "EntityRender::AddEntity : Not enough memory" << std::endl;
                    #endif
                    return false;
                }

                ENTITY_MESH_CHUNK emc;
                emc.chunk = this->render_groups[i].indirect_commands_chunk->ReserveRange(sizeof(VkDrawIndirectCommand));

                if(emc.chunk == nullptr) {
                    bool relocated;
                    if(!DataBank::GetManagedBuffer().ResizeChunk(this->render_groups[i].indirect_commands_chunk,
                                    this->render_groups[i].indirect_commands_chunk->range + sizeof(VkDrawIndirectCommand) * 10, relocated)) {
                        #if defined(DISPLAY_LOGS)
                        std::cout << "EntityRender::AddEntity : Not enough memory" << std::endl;
                        #endif
                        return false;
                    }else{
                        emc.chunk = this->render_groups[i].indirect_commands_chunk->ReserveRange(sizeof(VkDrawIndirectCommand));
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
                VkDrawIndirectCommand indirect = new_bind.mesh.GetIndirectCommand(emc.instance_id);
                DataBank::GetManagedBuffer().WriteData(&indirect, sizeof(VkDrawIndirectCommand),
                                                        this->render_groups[i].indirect_commands_chunk->offset + emc.chunk->offset);
                new_bind.entities.push_back(emc);

                // Add loaded mesh
                this->render_groups[i].drawables.push_back(std::move(new_bind));

                // Finish
                this->entities.push_back(&entity);
                for(uint8_t i=0; i<this->need_update.size(); i++) this->need_update[i] = true;
                return true;
            }
        }

        return false;
    }

    void EntityRender::Update(uint8_t frame_index)
    {
        for(auto entity : this->entities) {
            entity->Update(frame_index);
        }
    }

    void EntityRender::Refresh(uint8_t frame_index)
    {
        this->need_update[frame_index] = true;
        vkResetCommandBuffer(this->command_buffers[frame_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    VkCommandBuffer EntityRender::GetCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];
        if(!this->need_update[frame_index]) return command_buffer;
        this->need_update[frame_index] = false;

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

            auto pipeline = this->render_groups[i].pipeline;
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);

            std::vector<VkDescriptorSet> bind_descriptor_sets_1 = {
                // this->texture_descriptor.Get(),
                Camera::GetInstance().GetDescriptorSet(frame_index).Get()
            };

            if(this->render_groups[i].mask & Model::Mesh::RENDER_TEXTURE)
                bind_descriptor_sets_1.insert(bind_descriptor_sets_1.begin(), this->texture_descriptor.Get());


            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                                    static_cast<uint32_t>(bind_descriptor_sets_1.size()), bind_descriptor_sets_1.data(), 0, nullptr);

            for(auto& bind : this->render_groups[i].drawables) {

                if(bind.has_skeleton) {
                    std::vector<VkDescriptorSet> bind_descriptor_sets_2 = {this->skeleton_descriptor.Get(frame_index)};

                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->render_groups[i].pipeline.layout,
                                    static_cast<uint32_t>(bind_descriptor_sets_1.size()),
                                    static_cast<uint32_t>(bind_descriptor_sets_2.size()), bind_descriptor_sets_2.data(),
                                    static_cast<uint32_t>(bind.dynamic_offsets.size()), bind.dynamic_offsets.data());
                }

                bind.mesh.Render(command_buffer, DataBank::GetManagedBuffer().GetBuffer(frame_index).handle,
                                    pipeline.layout, static_cast<uint32_t>(bind.entities.size()),
                                    this->render_groups[i].instance_buffer_chunks,
                                    this->render_groups[i].indirect_commands_chunk->offset);
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

    std::vector<Entity*> EntityRender::SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end)
    {
        auto& camera = Engine::Camera::GetInstance();
        Area<float> const& near_plane_size = camera.GetFrustum().GetNearPlaneSize();
        Maths::Vector3 camera_position = -camera.GetUniformBuffer().position;
        Surface& draw_surface = Engine::Vulkan::GetDrawSurface();
        Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

        float x1 = static_cast<float>(box_start.X) - float_draw_surface.Width / 2.0f;
        float x2 = static_cast<float>(box_end.X) - float_draw_surface.Width / 2.0f;

        float y1 = static_cast<float>(box_start.Y) - float_draw_surface.Height / 2.0f;
        float y2 = static_cast<float>(box_end.Y) - float_draw_surface.Height / 2.0f;

        x1 /= float_draw_surface.Width / 2.0f;
        x2 /= float_draw_surface.Width / 2.0f;
        y1 /= float_draw_surface.Height / 2.0f;
        y2 /= float_draw_surface.Height / 2.0f;

        float left_x = std::min<float>(x1, x2);
        float right_x = std::max<float>(x1, x2);
        float top_y = std::min<float>(y1, y2);
        float bottom_y = std::max<float>(y1, y2);

        Maths::Vector3 base_near = camera_position + camera.GetFrontVector() * camera.GetNearClipDistance();
        Maths::Vector3 base_with = camera.GetRightVector() * near_plane_size.Width;
        Maths::Vector3 base_height = camera.GetUpVector() * near_plane_size.Height;

        Maths::Vector3 top_left_position = base_near + base_with * left_x - base_height * top_y;
        Maths::Vector3 bottom_right_position = base_near + base_with * right_x - base_height * bottom_y;

        Maths::Vector3 top_left_ray = (top_left_position - camera_position).Normalize();
        Maths::Vector3 bottom_right_ray = (bottom_right_position - camera_position).Normalize();

        Maths::Plane left_plane = {top_left_position, top_left_ray.Cross(-camera.GetUpVector())};
        Maths::Plane right_plane = {bottom_right_position, bottom_right_ray.Cross(camera.GetUpVector())};
        Maths::Plane top_plane = {top_left_position, top_left_ray.Cross(-camera.GetRightVector())};
        Maths::Plane bottom_plane = {bottom_right_position, bottom_right_ray.Cross(camera.GetRightVector())};

        std::vector<Entity*> return_value;
        for(Entity* entity : this->entities) {
            if(entity->InSelectBox(left_plane, right_plane, top_plane, bottom_plane)) {
                entity->selected = VK_TRUE;
                return_value.push_back(entity);
            }else{
                entity->selected = VK_FALSE;
            }
        }

        return return_value;
    }

    Entity* EntityRender::ToggleSelection(Point<uint32_t> mouse_position)
    {
        auto& camera = Engine::Camera::GetInstance();
        Area<float> const& near_plane_size = camera.GetFrustum().GetNearPlaneSize();
        Maths::Vector3 camera_position = -camera.GetUniformBuffer().position;
        Surface& draw_surface = Engine::Vulkan::GetDrawSurface();
        Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

        Point<float> real_mouse = {
            static_cast<float>(mouse_position.X) - float_draw_surface.Width / 2.0f,
            static_cast<float>(mouse_position.Y) - float_draw_surface.Height / 2.0f
        };

        real_mouse.X /= float_draw_surface.Width / 2.0f;
        real_mouse.Y /= float_draw_surface.Height / 2.0f;

        Maths::Vector3 mouse_world_position = camera_position + camera.GetFrontVector() * camera.GetNearClipDistance() + camera.GetRightVector()
                                            * near_plane_size.Width * real_mouse.X - camera.GetUpVector() * near_plane_size.Height * real_mouse.Y;
        Maths::Vector3 mouse_ray = mouse_world_position - camera_position;
        mouse_ray = mouse_ray.Normalize();

        Entity* selected_entity = nullptr;
        for(Entity* entity : this->entities) {
            if(selected_entity != nullptr) {
                entity->selected = VK_FALSE;
            }else if(entity->IntersectRay(camera_position, mouse_ray)) {
                selected_entity = entity;
                entity->selected = VK_TRUE;
            }else {
                entity->selected = VK_FALSE;
            }
        }

        return selected_entity;
    }
}