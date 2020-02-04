#include "Core.h"

namespace Engine
{
    /**
     * Constructeur
     */
    Core::Core()
    {
    }

    /**
     * Destructeur, libération des ressources
     */
    Core::~Core()
    {
        this->Clear();
    }

    /**
     * Allocation des resources de la boucle principale
     */
    bool Core::Initialize()
    {
        this->device = Vulkan::GetDevice();
        this->current_frame_index = Vulkan::GetConcurrentFrameCount() - 1;

        if(Vulkan::GetGraphicsQueue().index == Vulkan::GetPresentQueue().index) {
            this->loop_resources.present_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
            this->loop_resources.graphics_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
        }else{
            this->loop_resources.present_queue_family_index = Vulkan::GetPresentQueue().index;
            this->loop_resources.graphics_queue_family_index = Vulkan::GetGraphicsQueue().index;
        }

        this->loop_resources.image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        this->loop_resources.image_subresource_range.baseMipLevel = 0;
        this->loop_resources.image_subresource_range.levelCount = 1;
        this->loop_resources.image_subresource_range.baseArrayLayer = 0;
        this->loop_resources.image_subresource_range.layerCount = 1;

        ///////////////////////////////////////////////
        //   Allocation des ressources permettant    //
        // le fonctionnement de la boucle principale //
        ///////////////////////////////////////////////

        if(!this->AllocateRenderingResources()) return false;
        
        ////////////////////
        //  Model buffer  //
        // Vertex / Index //
        ////////////////////

        Vulkan::DATA_BUFFER buffer;
        Vulkan::GetInstance().CreateDataBuffer(
                buffer, MODEL_BUFFER_SIZE,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        this->model_buffer.SetBuffer(buffer);

        ////////////////////
        //  Work buffer   //
        // Uniform Buffer //
        ////////////////////

        Vulkan::GetInstance().CreateDataBuffer(
                buffer, WORK_BUFFER_SIZE,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        this->work_buffer.SetBuffer(buffer);
        this->work_buffer.SetChunkAlignment(Vulkan::GetDeviceLimits().minUniformBufferOffsetAlignment);

        VkDescriptorBufferInfo camera_buffer_infos = this->work_buffer.CreateSubBuffer(
            SUB_BUFFER_TYPE::CAMERA_UBO, 0,
            Vulkan::GetInstance().ComputeUniformBufferAlignment(sizeof(Camera::CAMERA_UBO)));

        VkDescriptorBufferInfo entity_buffer_infos = this->work_buffer.CreateSubBuffer(
            SUB_BUFFER_TYPE::ENTITY_UBO,
            camera_buffer_infos.offset + camera_buffer_infos.range,
            Vulkan::GetInstance().ComputeUniformBufferAlignment(Entity::MAX_UBO_SIZE * 10));

        /*VkDescriptorBufferInfo light_buffer_infos = this->work_buffer.CreateSubBuffer(
            SUB_BUFFER_TYPE::LIGHT_UBO,
            entity_buffer_infos.offset + entity_buffer_infos.range,
            Vulkan::GetInstance().ComputeUniformBufferAlignment(sizeof(Lighting::LIGHTING_UBO)));*/

        VkDescriptorBufferInfo bone_offsets_buffer_infos = this->work_buffer.CreateSubBuffer(
            SUB_BUFFER_TYPE::BONE_OFFSETS_UBO,
            entity_buffer_infos.offset + entity_buffer_infos.range,
            Vulkan::GetDeviceLimits().maxUniformBufferRange);

        ////////////////////////////////////
        //         Storage buffer         //
        // Stockage des données en volume //
        ////////////////////////////////////

        Vulkan::GetInstance().CreateDataBuffer(
                buffer, STORAGE_BUFFER_SIZE,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        this->storage_buffer.SetBuffer(buffer);
        this->storage_buffer.SetChunkAlignment(Vulkan::GetDeviceLimits().minStorageBufferOffsetAlignment);
        VkDescriptorBufferInfo skeleton_buffer_infos = this->storage_buffer.GetBufferInfos();

        // Création des descriptor sets
        bool enable_geometry_shader = false;
        #if defined(DISPLAY_LOGS)
        enable_geometry_shader = true;
        #endif

        // Pour les buffers dynamiques, vulkan attend le range par section et non pas le range du buffer complet
        entity_buffer_infos.range = Entity::MAX_UBO_SIZE;
        bone_offsets_buffer_infos.range = Bone::MAX_BONES_PER_UBO * sizeof(Matrix4x4);

        this->ds_manager.CreateViewDescriptorSet(camera_buffer_infos, entity_buffer_infos, enable_geometry_shader);
        this->ds_manager.CreateTextureDescriptorSet();
        this->ds_manager.CreateSkeletonDescriptorSet(skeleton_buffer_infos, bone_offsets_buffer_infos);
        // this->ds_manager.CreateBoneOffsetsDescriptorSet(bone_offsets_buffer_infos);

        // On réserve un chunk pour la lumière
        /*uint32_t light_offset;
        if(!this->work_buffer.ReserveChunk(light_offset, sizeof(Lighting::LIGHTING_UBO), SUB_BUFFER_TYPE::LIGHT_UBO)) return false;
        this->lighting.light.color = {1.0f, 1.0f, 1.0f};
        this->lighting.light.ambient_strength = 0.2f;
        this->lighting.light.specular_strength = 5.0f;
        this->work_buffer.WriteData(&this->lighting.GetUniformBuffer(), sizeof(Lighting::LIGHTING_UBO), 0, SUB_BUFFER_TYPE::LIGHT_UBO);*/

        // On réserve un chunk pour la caméra
        uint32_t camera_offset;
        if(!this->work_buffer.ReserveChunk(camera_offset, sizeof(Camera::CAMERA_UBO), SUB_BUFFER_TYPE::CAMERA_UBO)) return false;

        ////////////////////////////
        // Création des pipelines //
        ////////////////////////////

        this->renderers.resize(6);
        std::array<std::string, 3> shaders;

        shaders[0] = "./Shaders/basic_model.vert.spv";
        shaders[1] = "./Shaders/colored_model.frag.spv";
        uint16_t schema = Renderer::POSITION_VERTEX; // 1
        if(!this->renderers[0].Initialize(schema, shaders, ds_manager.GetLayoutArray(schema))) {
            this->Clear();
            return false;
        }

        shaders[0] = "./Shaders/material_basic_model.vert.spv";
        shaders[1] = "./Shaders/material_basic_model.frag.spv";
        schema = Renderer::POSITION_VERTEX | Renderer::MATERIAL; // 33
        if(!this->renderers[1].Initialize(schema, shaders, ds_manager.GetLayoutArray(schema))) {
            this->Clear();
            return false;
        }

        shaders[0] = "./Shaders/texture_basic_model.vert.spv";
        shaders[1] = "./Shaders/textured_model.frag.spv";
        schema = Renderer::POSITION_VERTEX | Renderer::UV_VERTEX | Renderer::MATERIAL | Renderer::TEXTURE; // 101
        if(!this->renderers[2].Initialize(schema, shaders, ds_manager.GetLayoutArray(schema))) {
            this->Clear();
            return false;
        }

        shaders[0] = "./Shaders/dynamic_model.vert.spv";
        shaders[1] = "./Shaders/textured_model.frag.spv";
        schema = Renderer::POSITION_VERTEX | Renderer::UV_VERTEX | Renderer::MATERIAL | Renderer::TEXTURE | Renderer::SKELETON; // 229
        if(!this->renderers[3].Initialize(schema, shaders, ds_manager.GetLayoutArray(schema))) {
            this->Clear();
            return false;
        }

        shaders[0] = "./Shaders/material_skeleton_model.vert.spv";
        shaders[1] = "./Shaders/material_basic_model.frag.spv";
        schema = Renderer::POSITION_VERTEX | Renderer::MATERIAL | Renderer::SKELETON; // 161
        if(!this->renderers[4].Initialize(schema, shaders, ds_manager.GetLayoutArray(schema))) {
            this->Clear();
            return false;
        }

        shaders[0] = "./Shaders/global_bone_model.vert.spv";
        shaders[1] = "./Shaders/textured_model.frag.spv";
        schema = Renderer::POSITION_VERTEX | Renderer::UV_VERTEX | Renderer::MATERIAL | Renderer::TEXTURE | Renderer::SINGLE_BONE;
        if(!this->renderers[5].Initialize(schema, shaders, ds_manager.GetLayoutArray(schema))) {
            this->Clear();
            return false;
        }

        /*shaders[0] = "./Shaders/dynamic_model.vert.spv";
        shaders[1] = "./Shaders/base.frag.spv";
        shaders[2] = "./Shaders/normal_debug.geom.spv";
        schema = Renderer::POSITION_VERTEX | Renderer::NORMAL_VERTEX | Renderer::UV_VERTEX | Renderer::MATERIAL | Renderer::TEXTURE | Renderer::SKELETON;
        if(!normal_debug.Initialize(schema, shaders, ds_manager.GetLayoutArray(DescriptorSetManager::SKELETON_TEXTURE_LAYOUT_ARRAY))) {
            this->Clear();
            return false;
        }*/

        // On initialise les command buffers, même s'il n'y a encore rien à afficher,
        // cela évite des warnings dans le cas où la scène serait vide.
        this->RebuildCommandBuffers();

        #if defined(DISPLAY_LOGS)
        std::cout << "Core::Initialize : Success" << std::endl;
        #endif

        // Succès
        return true;
    }

    bool Core::AllocateRenderingResources()
    {
        // On veut autant de ressources qu'il y a d'images dans la Swap Chain
        this->resources.resize(Vulkan::GetConcurrentFrameCount());
        
        for(uint32_t i=0; i<this->resources.size(); i++) {
            
            // Frame Buffers
            if(!Vulkan::GetInstance().CreateFrameBuffer(this->resources[i].framebuffer, Vulkan::GetSwapChain().images[i].view)) return false;

            // Semaphores
            if(!Vulkan::GetInstance().CreateSemaphore(this->resources[i].renderpass_semaphore)) return false;
            if(!Vulkan::GetInstance().CreateSemaphore(this->resources[i].swap_chain_semaphore)) return false;
        }

        // Command Buffers
        std::vector<Vulkan::COMMAND_BUFFER> graphics_buffer(Vulkan::GetConcurrentFrameCount());
        if(!Vulkan::GetInstance().CreateCommandBuffer(Vulkan::GetCommandPool(), graphics_buffer)) return false;
        for(uint32_t i=0; i<this->resources.size(); i++) this->resources[i].graphics_command_buffer = graphics_buffer[i];

        return true;
    }

    /**
     * On recréé les Frame Buffers lorsque la fenêtre subit des changements
     */
    bool Core::RebuildFrameBuffers()
    {
        if(Vulkan::HasInstance()) {
            for(uint32_t i=0; i<this->resources.size(); i++) {
                if(this->resources[i].framebuffer != nullptr) {

                    // Destruction du Frame Buffer
                    vkDestroyFramebuffer(this->device, this->resources[i].framebuffer, nullptr);
                    this->resources[i].framebuffer = nullptr;

                    // Création du Frame Buffer
                    if(!Vulkan::GetInstance().CreateFrameBuffer(this->resources[i].framebuffer, Vulkan::GetSwapChain().images[i].view)) return false;
                }
            }
        }

        // Succès
        return true;
    }

    /**
     * Libération des resouces
     */
    void Core::Clear()
    {
        this->entities.clear();
        this->meshes.clear();
        this->model_manager.Clear();

        if(Vulkan::HasInstance()) {

            // On attend que le GPU arrête de travailler
            vkDeviceWaitIdle(this->device);

            // Descriptor Sets
            this->ds_manager.Clear();

            // Textures
            for(auto& texture : this->textures) {
                if(texture.second.view != VK_NULL_HANDLE) vkDestroyImageView(this->device, texture.second.view, nullptr);
                if(texture.second.handle != VK_NULL_HANDLE) vkDestroyImage(this->device, texture.second.handle, nullptr);
                if(texture.second.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, texture.second.memory, nullptr);
            }
            this->textures.clear();

            // Model buffer
            if(this->model_buffer.GetBuffer().memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->model_buffer.GetBuffer().memory, nullptr);
            if(this->model_buffer.GetBuffer().handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, this->model_buffer.GetBuffer().handle, nullptr);
            this->model_buffer.Clear();

            // Work buffer
            if(this->work_buffer.GetBuffer().memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->work_buffer.GetBuffer().memory, nullptr);
            if(this->work_buffer.GetBuffer().handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, this->work_buffer.GetBuffer().handle, nullptr);
            this->work_buffer.Clear();

            // Storage buffer
            if(this->storage_buffer.GetBuffer().memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->storage_buffer.GetBuffer().memory, nullptr);
            if(this->storage_buffer.GetBuffer().handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, this->storage_buffer.GetBuffer().handle, nullptr);
            this->storage_buffer.Clear();

            // Générateurs de rendu
            this->renderers.clear();
            #if defined(DISPLAY_LOGS)
            this->normal_debug.Release();
            #endif

            for(uint32_t i=0; i<this->resources.size(); i++) {

                // Frame Buffer
                if(this->resources[i].framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(this->device, this->resources[i].framebuffer, nullptr);

                // Semaphores
                if(this->resources[i].renderpass_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(this->device, this->resources[i].renderpass_semaphore, nullptr);
                if(this->resources[i].swap_chain_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(this->device, this->resources[i].swap_chain_semaphore, nullptr);

                // Fences
                if(this->resources[i].graphics_command_buffer.fence != VK_NULL_HANDLE) vkDestroyFence(this->device, this->resources[i].graphics_command_buffer.fence, nullptr);
            }
        }

        this->resources.clear();
        this->device = VK_NULL_HANDLE;
    }

    /**
     * Ajout d'un squelette dans la carte graphique
     */
    bool Core::AddSkeleton(std::string const& skeleton)
    {
        // Le squelette est déjà présent dans la carte graphique
        if(this->skeletons.count(skeleton) > 0) return true;

        // Le squelette n'est pas chargé dans le gestionnaire de models
        if(!this->model_manager.skeletons.count(skeleton)) return false;

        //////////////////////////
        // UBO des bone offsets //
        //////////////////////////

        // Construction du buffer
        std::vector<char> offsets_ubo;
        std::map<std::string, uint32_t> mesh_ubo_offsets;
        uint32_t ubo_alignment = static_cast<uint32_t>(Vulkan::GetDeviceLimits().minUniformBufferOffsetAlignment);
        this->model_manager.skeletons[skeleton]->BuildBoneOffsetsUBO(offsets_ubo, mesh_ubo_offsets, ubo_alignment);

        // Réservation d'un chunk sur le buffer des offsets
        uint32_t mesh_ubo_base_offset;
        if(!this->work_buffer.ReserveChunk(mesh_ubo_base_offset, offsets_ubo.size(), SUB_BUFFER_TYPE::BONE_OFFSETS_UBO)) return false;
        this->work_buffer.WriteData(offsets_ubo.data(), offsets_ubo.size(), mesh_ubo_base_offset, SUB_BUFFER_TYPE::BONE_OFFSETS_UBO);

        // On positionne tous les skeleton_buffer_offset des mesh
        for(auto& mesh : mesh_ubo_offsets)
            if(this->model_manager.models.count(mesh.first))
                this->model_manager.models[mesh.first]->skeleton_buffer_offset = mesh_ubo_base_offset + mesh.second;

        //////////////////////
        // SBO du squelette //
        //////////////////////

        // Construction du buffer
        std::vector<char> skeleton_sbo;
        SKELETAL_ANIMATION_SBO animation_sbo;
        this->model_manager.skeletons[skeleton]->BuildAnimationSBO(skeleton_sbo, "metarig|stand", animation_sbo.frame_count, animation_sbo.bone_per_frame, 30);

        // Réservation d'un chunk sur le buffer du squelette
        VkDeviceSize skeleton_offset;
        if(!this->storage_buffer.ReserveChunk(skeleton_offset, skeleton_sbo.size())) return false;
        this->storage_buffer.WriteData(skeleton_sbo.data(), skeleton_sbo.size(), skeleton_offset);
        animation_sbo.offset = static_cast<uint32_t>(skeleton_offset);
        this->skeletons[skeleton] = animation_sbo;
        this->storage_buffer.Flush();

        // Succès
        return true;
    }

    /**
     * Ajout d'une entité à la scène
     */
    bool Core::AddEntity(std::shared_ptr<Entity> entity)
    {
        for(auto& entity_mesh : entity->GetMeshes()) {

            // On cherche parmi tous les models enregistrés dans le vertex buffer
            // si ceux de l'entité y sont déjà
            bool model_found = false;
            for(auto& core_mesh : this->meshes) {
                if(core_mesh == entity_mesh) {
                    model_found = true;
                    break;
                }
            }

            // Si ce n'est pas le cas, on place le modèle dans le vertex buffer
            // et on l'ajoute à la liste de ceux qui s'y trouvent
            if(!model_found) {

                // En premier lieu, on vérifie qu'il existe un générateur de rendu capable d'afficher le modèle
                Renderer* match_renderer = nullptr;
                for(auto& renderer : this->renderers) {
                    if(renderer.MatchRenderMask(entity_mesh->render_mask)) {
                        match_renderer = &renderer;
                        break;
                    }
                }

                // Aucun générateur de rendu trouvé
                if(match_renderer == nullptr) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "AddEntity => MatchRenderMask(" << entity_mesh->name << ") : Failed" << std::endl;
                    #endif
                    return false;
                }

                // Le modèle est associé à des textures,
                // on les chargent dans la carte graphique si ce n'est pas déjà fait
                if(entity_mesh->render_mask & Renderer::SCHEMA_PRIMITIVE::TEXTURE) {
                    for(auto& material : entity_mesh->materials) {
                        std::string texture = this->model_manager.materials[material.first].texture;
                        if(!this->AddTexture(this->model_manager.textures[texture], texture, *match_renderer)) {
                            #if defined(DISPLAY_LOGS)
                            std::cout << "AddEntity->Model(" << entity_mesh->name << ") => AddTexture(" << texture << ") => Failed" << std::endl;
                            #endif
                            return false;
                        }
                    }
                }

                // Le modèle est associé à un squelette,
                // on le charge dans la carte graphique si ce n'est pas déjà fait
                if(!entity_mesh->skeleton.empty()) {
                    if(!this->AddSkeleton(entity_mesh->skeleton)) {
                        #if defined(DISPLAY_LOGS)
                        std::cout << "AddEntity->Model(" << entity_mesh->name << ") => AddSkeleton(" << entity_mesh->skeleton << ") => Failed" << std::endl;
                        #endif
                        return false;
                    }
                    std::shared_ptr<SkeletonEntity> skeleton_entity = std::dynamic_pointer_cast<SkeletonEntity>(entity);
                    if(skeleton_entity.get() != nullptr) skeleton_entity->bones_per_frame = this->skeletons[entity_mesh->skeleton].bone_per_frame;
                }
                
                VkDeviceSize vbo_size;
                std::unique_ptr<char> mesh_vbo = entity_mesh->BuildVBO(vbo_size);
                size_t move_offset = entity_mesh->vertex_buffer_offset;
                if(!this->model_buffer.ReserveChunk(entity_mesh->vertex_buffer_offset, vbo_size)) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "AddEntity->Model(" << entity_mesh->name << ") => ReserveChunk => Failed" << std::endl;
                    #endif
                    return false;
                }
                entity_mesh->UpdateIndexBufferOffset(entity_mesh->vertex_buffer_offset - move_offset);
                this->model_buffer.WriteData(mesh_vbo.get(), vbo_size, entity_mesh->vertex_buffer_offset);
                this->meshes.push_back(entity_mesh);
            }
        }

        // Mise à jour du vertex buffer
        this->model_buffer.Flush();

        // On ajoute l'entité à la scène
        if(!this->work_buffer.ReserveChunk(entity->dynamic_buffer_offset, entity->GetUboSize(), SUB_BUFFER_TYPE::ENTITY_UBO)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "AddEntity => ReserveChunk : Failed" << std::endl;
            #endif
            return false;
        }
        this->entities.push_back(entity);

        // Reconstruction des command buffers
        if(!this->RebuildCommandBuffers()) return false;

        #if defined(DISPLAY_LOGS)
        std::cout << "AddEntity : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Envoi d'un texture dans la carte graphique
     */
    bool Core::AddTexture(Tools::IMAGE_MAP const& image, std::string const& texture, Renderer& renderer)
    {
        // La texture existe déjà
        if(this->textures.count(texture)) return true;

        // Image Buffer représentant la texture
        Vulkan::IMAGE_BUFFER buffer;

        // Création du buffer
        buffer = Vulkan::GetInstance().CreateImageBuffer(
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            image.width, image.height);

        // En cas d'échec, on renvoie false
        if(buffer.handle == VK_NULL_HANDLE) return false;

        // Envoi de l'image dans la carte graphique
        if(!Vulkan::GetInstance().SendToBuffer(buffer, image.data.data(), image.data.size(), image.width, image.height)) return false;
        this->textures[texture] = buffer;

        // Création du descriptor set
        return this->ds_manager.CreateTextureDescriptorSet(buffer.view, texture);
    }

    bool Core::RebuildCommandBuffers()
    {
        for(uint32_t i=0; i<Vulkan::GetConcurrentFrameCount(); i++)
            if(!this->BuildCommandBuffer(i)) return false;

        return true;
    }

    bool Core::BuildCommandBuffer(uint32_t swap_chain_image_index)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        VkCommandBuffer command_buffer = this->resources[swap_chain_image_index].graphics_command_buffer.handle;
        VkFramebuffer frame_buffer = this->resources[swap_chain_image_index].framebuffer;

        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

        std::array<VkClearValue, 2> clear_value = {};
        clear_value[0].color = { 0.1f, 0.2f, 0.3f, 0.0f };
        clear_value[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = Vulkan::GetRenderPass();
        render_pass_begin_info.framebuffer = frame_buffer;
        render_pass_begin_info.renderArea.offset.x = 0;
        render_pass_begin_info.renderArea.offset.y = 0;
        render_pass_begin_info.renderArea.extent.width = Vulkan::GetDrawSurface().width;
        render_pass_begin_info.renderArea.extent.height = Vulkan::GetDrawSurface().height;
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_value.size());
        render_pass_begin_info.pClearValues = clear_value.data();

        // Début de la render pass
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

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

        for(auto& renderer : this->renderers) {
            
            bool pipeline_bound = false;

            for(auto& entity : this->entities) {
                for(auto& model : entity->GetMeshes()) {

                    // Si ce renderer n'est pas compatible avec le modèle à afficher, on passe au suivant
                    if(!renderer.MatchRenderMask(model->render_mask)) continue;

                    std::vector<VkDescriptorSet> bind_descriptor_sets = {this->ds_manager.GetViewDescriptorSet()};
                    std::vector<uint32_t> dynamic_offsets = {entity->dynamic_buffer_offset};

                    if(!pipeline_bound) {
                        // On associe la pipeline
                        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.GetPipeline().handle);
                        pipeline_bound = true;
                    }

                    // Si mesh n'a pas de sous-parties mais qu'il a quand-même un matériau, c'est un matériau global appliqué à tout le model
                    if(model->sub_meshes.empty() && !model->materials.empty()) {
                        auto& material = this->model_manager.materials[model->materials[0].first];
                        vkCmdPushConstants(command_buffer, renderer.GetPipeline().layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, Mesh::MATERIAL::Size(), &material);

                        // Si ce matériau est associé à une texture, on ajoute son descriptor set
                        if(!material.texture.empty() && !model->uv_buffer.empty())
                            bind_descriptor_sets.push_back(this->ds_manager.GetTextureDescriptorSet(material.texture));
                    }

                    // Si ce modèle a un squelette, on ajoute son descriptor set
                    if(!model->skeleton.empty()) {
                        bind_descriptor_sets.push_back(this->ds_manager.GetSkeletonDescriptorSet());
                        // dynamic_offsets.push_back(this->skeletons[model->skeleton].offset + 30 * this->skeletons[model->skeleton].size_per_frame);
                        dynamic_offsets.push_back(model->skeleton_buffer_offset);
                        if(model->render_mask & Renderer::SINGLE_BONE)
                            vkCmdPushConstants(command_buffer, renderer.GetPipeline().layout, VK_SHADER_STAGE_VERTEX_BIT,
                                               Mesh::MATERIAL::Size(), sizeof(uint32_t), &model->deformers[0].bone_ids[0]);
                    }

                    // On associe le vertex buffer
                    vkCmdBindVertexBuffers(command_buffer, 0, 1, &this->model_buffer.GetBuffer().handle, &model->vertex_buffer_offset);
                    
                    if(model->index_buffer_offset > 0) {

                        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.GetPipeline().layout, 0,
                            static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(),
                            static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());

                        // On associe l'index buffer
                        vkCmdBindIndexBuffer(command_buffer, this->model_buffer.GetBuffer().handle, model->index_buffer_offset, VK_INDEX_TYPE_UINT32);
                        if(model->sub_meshes.empty()) {
                            
                            // Affichage du model par index buffer
                            vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(model->index_buffer.size()), 1, 0, 0, 0);
                        }else{

                            // Le mesh est découpé en sous-parties, une partie pour chaque matériau
                            for(auto& sub_mesh : model->sub_meshes) {
                                
                                // On envoie le matériau au shader
                                Mesh::MATERIAL material = this->model_manager.materials[sub_mesh.first];
                                vkCmdPushConstants(command_buffer, renderer.GetPipeline().layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, Mesh::MATERIAL::Size(), &material);
                                
                                // affichage du sous-model par index buffer
                                vkCmdDrawIndexed(command_buffer, sub_mesh.second.index_count, 1, sub_mesh.second.first_index, 0, 0);
                            }
                        }
                    }else{

                        if(model->sub_meshes.empty()) {

                            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.GetPipeline().layout, 0,
                                static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(),
                                static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());

                            // Affichage générique, sans index buffer
                            vkCmdDraw(command_buffer, static_cast<uint32_t>(model->index_buffer.size()), 1, 0, 0);
                        }else{

                            // Le mesh est découpé en sous-parties, une partie pour chaque matériau/texture
                            for(auto& sub_mesh : model->sub_meshes) {
                                
                                Mesh::MATERIAL material = this->model_manager.materials[sub_mesh.first];
                                // On associe le descriptor set de la texture
                                if(!material.texture.empty() && !model->uv_buffer.empty()) {
                                    
                                    if(!model->skeleton.empty()) bind_descriptor_sets.resize(3);
                                    else bind_descriptor_sets.resize(2);

                                    // descriptor_set = renderer.GetDescriptorSet(material.texture);
                                    bind_descriptor_sets[1] = this->ds_manager.GetTextureDescriptorSet(material.texture);
                                }

                                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.GetPipeline().layout, 0,
                                    static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(),
                                    static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());

                                // En envoie le matériau au shader
                                vkCmdPushConstants(command_buffer, renderer.GetPipeline().layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, Mesh::MATERIAL::Size(), &material);
                                
                                // Affichage générique, sans index buffer
                                vkCmdDraw(command_buffer, sub_mesh.second.index_count, 1, sub_mesh.second.first_index, 0);
                            }
                        }
                    }
                }
            }
        }

        ////////////////////////////////////////////
        //            En cas d'activation         //
        //       de l'affichage des normales      //
        // -> uniquement disponible en mode debug //
        ////////////////////////////////////////////

        /*#if defined(DISPLAY_LOGS)
        if(this->normal_debug.GetPipeline().handle != nullptr) {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->normal_debug.GetPipeline().handle);
            for(auto& entity : this->entities) {
                for(auto& model : entity->GetMeshes()) {

                    if(!this->normal_debug.MatchRenderMask(model->render_mask)) continue;

                    std::vector<VkDescriptorSet> bind_descriptor_sets = {this->ds_manager.GetViewDescriptorSet()};
                    std::vector<uint32_t> dynamic_offsets = {entity->dynamic_buffer_offset};

                    if(model->sub_meshes.empty() && !model->materials.empty()) {
                        auto& material = this->model_manager.materials[model->materials[0].first];

                        if(!material.texture.empty() && !model->uv_buffer.empty())
                            bind_descriptor_sets.push_back(this->ds_manager.GetTextureDescriptorSet(material.texture));
                    }

                    if(!model->skeleton.empty()) {
                        bind_descriptor_sets.push_back(this->ds_manager.GetSkeletonDescriptorSet());
                        dynamic_offsets.push_back(this->skeletons[model->skeleton].offset);
                        dynamic_offsets.push_back(model->skeleton_buffer_offset);
                    }
                    
                    vkCmdBindVertexBuffers(command_buffer, 0, 1, &this->model_buffer.GetBuffer().handle, &model->vertex_buffer_offset);

                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->normal_debug.GetPipeline().layout, 0,
                                static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(),
                                static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());

                    vkCmdDraw(command_buffer, static_cast<uint32_t>(model->index_buffer.size()), 1, 0, 0);
                }
            }
        }
        #endif*/

        // Fin de la render pass
        vkCmdEndRenderPass(command_buffer);

        VkResult result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "BuildCommandBuffer : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Boucle d'affichage principale
     */
    void Core::DrawScene()
    {
        // Mise à jour des Uniform Buffers
        for(auto& entity : this->entities) entity->UpdateUBO(this->work_buffer);
        this->work_buffer.WriteData(&this->camera.GetUniformBuffer(), sizeof(Camera::CAMERA_UBO), 0, SUB_BUFFER_TYPE::CAMERA_UBO);
        if(!this->work_buffer.Flush()) return;

        // On pase à l'image suivante
        this->current_frame_index = (this->current_frame_index + 1) % static_cast<uint8_t>(Vulkan::GetConcurrentFrameCount());
        Vulkan::RENDERING_RESOURCES current_rendering_resource = this->resources[this->current_frame_index];

        // Acquisition d'une image de la Swap Chain
        uint32_t swap_chain_image_index;
        if(!Vulkan::GetInstance().AcquireNextImage(current_rendering_resource, swap_chain_image_index)) return;

        // Présentation de l'image
        // En cas d'échec on recréé les command buffers
        if(!Vulkan::GetInstance().PresentImage(
            current_rendering_resource, swap_chain_image_index,
            this->loop_resources.graphics_queue_family_index,
            this->loop_resources.present_queue_family_index,
            this->loop_resources.image_subresource_range)) {

            // On recréé la matrice de projection en cas de changement de ration
            auto& surface = Vulkan::GetDrawSurface();
            float aspect_ratio = static_cast<float>(surface.width) / static_cast<float>(surface.height);
            this->camera.GetUniformBuffer().projection = Matrix4x4::PerspectiveProjectionMatrix(aspect_ratio, 60.0f, 0.1f, 100.0f);

            // Reconstruction du Frame Buffer et des Command Buffers
            this->RebuildFrameBuffers();
            this->RebuildCommandBuffers();
        }
    }
}