#include "Core.h"

namespace Engine
{
    void Core::Clear()
    {
        // UnitControl
        UnitControl::Clear();

        // Entity
        StaticEntity::Clear();
        DynamicEntity::Clear();

        // LOD
        LODGroup::Clear();

        // User Interface
        if(this->user_interface != nullptr) {
            this->user_interface->RemoveListener(this);
            delete this->user_interface;
        }

        // Timer
        Timer::Clear();

        // Map
        if(this->map != nullptr) delete this->map;

        // Entity Render
        if(this->entity_render != nullptr) delete this->entity_render;

        // Camera
        Camera::DestroyInstance();

        if(Vulkan::HasInstance()) {
            vkDeviceWaitIdle(Vulkan::GetDevice());

            // Stop listening to window events
            Vulkan::GetInstance().GetDrawWindow()->RemoveListener(this);

            // Transfer command buffers
            for(auto& buffer : this->transfer_buffers)
                if(buffer.fence != nullptr) vkDestroyFence(Vulkan::GetDevice(), buffer.fence, nullptr);

            // Rendering resources
            this->DestroyRenderingResources();
        }

        // DataBank
        DataBank::DestroyInstance();

        this->graphics_command_pool = nullptr;
        this->user_interface        = nullptr;
        this->map                   = nullptr;
        this->entity_render         = nullptr;
        this->draw                  = true;
    }

    bool Core::Initialize()
    {
        // Clean all objects
        this->Clear();
        
        // Listen to window events
        Vulkan::GetInstance().GetDrawWindow()->AddListener(this);

        // Create a storage for engine objects
        DataBank::CreateInstance();

        // Allocate vulkan resources
        if(!this->AllocateRenderingResources()) return false;

        // Allocate transfer command buffers
        uint8_t frame_count = Vulkan::GetConcurrentFrameCount();
        this->transfer_buffers.resize(frame_count);
        if(!Vulkan::GetInstance().CreateCommandBuffer(this->graphics_command_pool, this->transfer_buffers)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Core::Initialize() => CreateCommandBuffer [transfer] : Failed" << std::endl;
            #endif
        }

        // Allocate data buffers
        DataBank::GetInstancedBuffer().Allocate(
            SIZE_MEGABYTE(100), MULTI_USAGE_BUFFER_MASK, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            frame_count, true, {Vulkan::GetGraphicsQueue().index}
        );

        DataBank::GetCommonBuffer().Allocate(
            SIZE_MEGABYTE(32), MULTI_USAGE_BUFFER_MASK, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            1, true, {Vulkan::GetGraphicsQueue().index}
        );

        // Initialize Timer
        Timer::Initialize();

        // Initialize LOD
        if(!LODGroup::Initialize()) return false;

        // Initialize Entity
        if(!StaticEntity::Initialize()) return false;
        if(!DynamicEntity::Initialize()) return false;

        // Initialize UnitControl
        if(!UnitControl::Initialize()) return false;

        // Initialize Camera
        Camera::CreateInstance();

        // Create a renderer for entities
        this->entity_render = new EntityRender(this->graphics_command_pool);

        // Create the map
        this->map = new Map(this->graphics_command_pool);
        // this->entity_render->GetEntityDescriptor().Update();

        // Create the user interface
        this->user_interface = new UserInterface(this->graphics_command_pool);
        this->user_interface->AddListener(this);
        for(uint8_t i=0; i<frame_count; i++)
            this->resources[i].comand_buffers.push_back(this->user_interface->BuildCommandBuffer(i, this->resources[i].framebuffer));

        // Success
        return true;
    }

    bool Core::AllocateRenderingResources()
    {
        if(!Vulkan::GetInstance().CreateCommandPool(this->graphics_command_pool, Vulkan::GetInstance().GetGraphicsQueue().index)) return false;

        // On veut autant de ressources qu'il y a d'images dans la Swap Chain
        uint8_t frame_count = Vulkan::GetConcurrentFrameCount();
        this->resources.resize(frame_count);
        
        for(uint32_t i=0; i<this->resources.size(); i++) {
            
            // Frame Buffers
            if(!Vulkan::GetInstance().CreateFrameBuffer(this->resources[i].framebuffer, Vulkan::GetSwapChain().images[i].view)) return false;

            // Semaphores
            if(!Vulkan::GetInstance().CreateSemaphore(this->resources[i].semaphore)) return false;
        }

        // Graphics Command Buffers
        std::vector<Vulkan::COMMAND_BUFFER> graphics_buffers(frame_count);
        if(!Vulkan::GetInstance().CreateCommandBuffer(this->graphics_command_pool, graphics_buffers, VK_COMMAND_BUFFER_LEVEL_PRIMARY, false)) return false;
        for(uint32_t i=0; i<this->resources.size(); i++) {
            this->resources[i].comand_buffers.push_back(graphics_buffers[i].handle);
            this->resources[i].fence = Vulkan::GetInstance().CreateFence();
        }

        // SwapChain semaphores
        this->swap_chain_semaphores.resize(frame_count);
        for(auto& semaphore : this->swap_chain_semaphores)
            if(!Vulkan::GetInstance().CreateSemaphore(semaphore)) return false;

        // Write semaphores
        this->write_semaphores.resize(frame_count + 1);
        for(auto& semaphore : this->write_semaphores)
            if(!Vulkan::GetInstance().CreateSemaphore(semaphore)) return false;

        // Read semaphores
        this->read_semaphores.resize(frame_count + 1);
        for(auto& semaphore : this->read_semaphores)
            if(!Vulkan::GetInstance().CreateSemaphore(semaphore)) return false;

        return true;
    }

    void Core::DestroyRenderingResources()
    {
        // Write semaphores
        for(auto& semaphore : this->write_semaphores) vkDestroySemaphore(Vulkan::GetDevice(), semaphore, nullptr);
        this->write_semaphores.clear();

        // Read semaphores
        for(auto& semaphore : this->read_semaphores) vkDestroySemaphore(Vulkan::GetDevice(), semaphore, nullptr);
        this->read_semaphores.clear();

        // SwapChain semaphores
        for(auto& semaphore : this->swap_chain_semaphores) vkDestroySemaphore(Vulkan::GetDevice(), semaphore, nullptr);
        this->swap_chain_semaphores.clear();

        for(auto& resource : this->resources)
            if(resource.fence != nullptr) vkDestroyFence(Vulkan::GetDevice(), resource.fence, nullptr);
            
        if(this->graphics_command_pool != nullptr) vkDestroyCommandPool(Vulkan::GetDevice(), this->graphics_command_pool, nullptr);

        for(uint32_t i=0; i<this->resources.size(); i++) {
            
            // Semaphore
            if(this->resources[i].semaphore != nullptr) vkDestroySemaphore(Vulkan::GetDevice(), this->resources[i].semaphore, nullptr);

            // Frame Buffer
            if(this->resources[i].semaphore != nullptr) vkDestroyFramebuffer(Vulkan::GetDevice(), this->resources[i].framebuffer, nullptr);
        }

        this->resources.clear();
    }

    bool Core::BuildRenderPass(uint32_t swap_chain_image_index)
    {
        Vulkan::RENDERING_RESOURCES const& resources = this->resources[swap_chain_image_index];
        VkCommandBuffer command_buffer = resources.comand_buffers[0];
        VkFramebuffer frame_buffer = resources.framebuffer;

        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildCommandBuffer[" << swap_chain_image_index << "] => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

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

        // Début de la render pass primaire
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        auto& surface = Vulkan::GetDrawSurface();
        if(surface.width > 0 && surface.height > 0) {
            // Exécution des render passes secondaires
            VkCommandBuffer command_buffers[2] = {
                this->entity_render->GetCommandBuffer(swap_chain_image_index, frame_buffer),
                this->map->GetCommandBuffer(swap_chain_image_index, frame_buffer)
            };
            vkCmdExecuteCommands(command_buffer, 2, command_buffers);
        }

        /*VkCommandBuffer command_buffers[2] = {
            this->entity_render->GetCommandBuffer(swap_chain_image_index, frame_buffer)
        };
        vkCmdExecuteCommands(command_buffer, 1, command_buffers);*/

        // Fin de la render pass primaire
        vkCmdEndRenderPass(command_buffer);

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildCommandBuffer[" << swap_chain_image_index << "] => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    bool Core::RebuildFrameBuffers()
    {
        if(Vulkan::HasInstance()) {
            for(uint32_t i=0; i<this->resources.size(); i++) {
                if(this->resources[i].framebuffer != nullptr) {

                    // Destruction du Frame Buffer
                    vkDestroyFramebuffer(Vulkan::GetDevice(), this->resources[i].framebuffer, nullptr);
                    this->resources[i].framebuffer = nullptr;

                    // Création du Frame Buffer
                    if(!Vulkan::GetInstance().CreateFrameBuffer(this->resources[i].framebuffer, Vulkan::GetSwapChain().images[i].view)) return false;
                }
            }
        }

        // Succès
        return true;
    }

    void Core::SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end)
    {
        DynamicEntity::UpdateSelection(DynamicEntity::SquareSelection(box_start, box_end));
        // this->map->UpdateSelection(DynamicEntity::SquareSelection(box_start, box_end));
    }

    void Core::ToggleSelection(Point<uint32_t> mouse_position)
    {
        DynamicEntity* entity = DynamicEntity::ToggleSelection(mouse_position);
        if(entity == nullptr) DynamicEntity::UpdateSelection({});
        else DynamicEntity::UpdateSelection({entity});
        /*if(entity == nullptr) this->map->UpdateSelection({});
        else this->map->UpdateSelection({entity});*/
    }

    void Core::MoveToPosition(Point<uint32_t> mouse_position)
    {
        auto& camera = Engine::Camera::GetInstance();
        Area<float> const& near_plane_size = camera.GetFrustum().GetNearPlaneSize();
        Maths::Vector3 camera_position = -camera.GetUniformBuffer().position;
        Surface& draw_surface = Engine::Vulkan::GetDrawSurface();
        Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

        float x = static_cast<float>(mouse_position.X) - float_draw_surface.Width / 2.0f;
        float y = static_cast<float>(mouse_position.Y) - float_draw_surface.Height / 2.0f;

        x /= float_draw_surface.Width / 2.0f;
        y /= float_draw_surface.Height / 2.0f;

        Maths::Vector3 mouse_ray = camera.GetFrontVector() * camera.GetNearClipDistance()
                                 + camera.GetRightVector() * near_plane_size.Width * x
                                 - camera.GetUpVector() * near_plane_size.Height * y;

        mouse_ray = mouse_ray.Normalize();

        float ray_length;
        if(!Maths::intersect_plane({0, 1, 0}, {}, camera_position, mouse_ray, ray_length)) return;
        Maths::Vector3 destination = camera_position + mouse_ray * ray_length;

        UnitControl::Instance().MoveUnits(destination);

        // DynamicEntity::MoveToPosition(destination);
        /*for(auto& entity : this->map->GetSelectedEntities()) {
            entity->MoveToPosition(destination);
        }*/
    }

    void Core::SizeChanged(Area<uint32_t> size)
    {
        auto& surface = Vulkan::GetDrawSurface();

        if(!surface.width || !surface.height) {
            this->draw = false;
            return;
        }else{
            this->draw = true;
        }

        Vulkan::GetInstance().OnWindowSizeChanged();

        float aspect_ratio = static_cast<float>(surface.width) / static_cast<float>(surface.height);
        Camera::GetInstance().GetUniformBuffer().projection = Maths::Matrix4x4::PerspectiveProjectionMatrix(aspect_ratio, 60.0f, 0.1f, 2000.0f);

        // Reconstruction du Frame Buffer et des Command Buffers
        this->RebuildFrameBuffers();

        // Force rebuild external command buffers
        this->map->Refresh();
        this->user_interface->Refresh();
        this->entity_render->Refresh();
    }

    void Core::Loop()
    {
        if(!this->draw) return;

        static uint32_t current_semaphore_index = (current_semaphore_index + 1) % this->swap_chain_semaphores.size();
        VkSemaphore present_semaphore = this->swap_chain_semaphores[current_semaphore_index];

        // Acquire image
        uint32_t image_index;
        if(!Vulkan::GetInstance().AcquireNextImage(image_index, present_semaphore)) return;

        VkResult result = vkWaitForFences(Vulkan::GetDevice(), 1, &this->resources[image_index].fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Core::BuildRenderPass => vkWaitForFences : Timeout" << std::endl;
            #endif
            return;
        }
        vkResetFences(Vulkan::GetDevice(), 1, &this->resources[image_index].fence);

        // Update global timer
        Timer::Update(image_index);

        // UnitControl
        UnitControl::Instance().Update();

        bool map_refreshed = false;
        bool entity_renderer_refreshed = false;

        Camera::GetInstance().Update(image_index);
        this->map->Update(image_index);
        this->user_interface->Update(image_index);

        if(DynamicEntity::GetMatrixDescriptor().NeedUpdate(0)) {
            vkQueueWaitIdle(Vulkan::GetGraphicsQueue().handle);
            for(uint8_t i=0; i<Vulkan::GetConcurrentFrameCount(); i++) {
                this->map->Refresh(i);
                this->entity_render->Refresh(i);
                UnitControl::Instance().Refresh();
                DynamicEntity::UpdateMatrixDescriptor(i);
                // DynamicEntity::UpdateMovementDescriptor(i);
            }
            entity_renderer_refreshed = true;
            map_refreshed = true;
        }

        if(StaticEntity::GetMatrixDescriptor().NeedUpdate(image_index)) {
            if(!entity_renderer_refreshed) { this->entity_render->Refresh(image_index); entity_renderer_refreshed = true; }
            StaticEntity::UpdateMatrixDescriptor(image_index);
        }

        if(DynamicEntity::GetSelectionDescriptor().NeedUpdate(image_index)) {
            if(!map_refreshed) { this->map->Refresh(image_index); map_refreshed = true; }
            if(!entity_renderer_refreshed) { this->entity_render->Refresh(image_index); entity_renderer_refreshed = true; }
            DynamicEntity::UpdateSelectionDescriptor(image_index);
        }

        if(DynamicEntity::GetAnimationDescriptor().NeedUpdate(image_index)) {
            if(!entity_renderer_refreshed) { this->entity_render->Refresh(image_index); entity_renderer_refreshed = true; }
            DynamicEntity::UpdateAnimationDescriptor(image_index);
        }

        if(LODGroup::GetDescriptor().NeedUpdate(image_index)) {
            if(!entity_renderer_refreshed) { this->entity_render->Refresh(image_index); entity_renderer_refreshed = true; }
            LODGroup::UpdateDescriptor(image_index);
        }

        this->entity_render->UpdateDescriptorSet(image_index);

        DynamicEntity::ReadMatrix();
        // DynamicEntity::ReadMovement();
        // DynamicEntity::Update(image_index);

        bool instanced_write = DataBank::GetInstancedBuffer().FlushWrite(this->transfer_buffers[image_index], image_index, this->write_semaphores[image_index + 1]);
        bool common_write = DataBank::GetCommonBuffer().FlushWrite(this->transfer_buffers[image_index], 0, this->write_semaphores[0]);

        // DataBank::GetInstancedBuffer().FlushRead(this->transfer_buffers[image_index], image_index, this->read_semaphores[image_index + 1], this->write_semaphores[image_index + 1]);
        VkSemaphore wait_semaphore = common_write ? this->write_semaphores[0] : nullptr;
        bool common_read = DataBank::GetCommonBuffer().FlushRead(this->transfer_buffers[image_index], 0, this->read_semaphores[0], wait_semaphore);

        // Build image
        this->BuildRenderPass(image_index);
        this->user_interface->BuildCommandBuffer(image_index, this->resources[image_index].framebuffer);

        std::vector<VkSemaphore> wait_semaphores;
        if(instanced_write) wait_semaphores.push_back(this->write_semaphores[image_index + 1]);
        if(common_read) wait_semaphores.push_back(this->read_semaphores[0]);
        VkSemaphore compute_semaphore = this->entity_render->SubmitComputeShader(image_index, wait_semaphores);

        wait_semaphores = {present_semaphore, compute_semaphore};
        std::vector<VkPipelineStageFlags> semaphore_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};

        // Present image
        if(!Vulkan::GetInstance().PresentImage(this->resources[image_index], wait_semaphores, semaphore_stages, image_index)) {

            // On recréé la matrice de projection en cas de changement de ratio
            auto& surface = Vulkan::GetDrawSurface();
            float aspect_ratio = static_cast<float>(surface.width) / static_cast<float>(surface.height);
            Camera::GetInstance().GetUniformBuffer().projection = Maths::Matrix4x4::PerspectiveProjectionMatrix(aspect_ratio, 60.0f, 0.1f, 2000.0f);

            // Reconstruction du Frame Buffer et des Command Buffers
            this->RebuildFrameBuffers();

            // Force rebuild command buffers
            this->map->Refresh();
            this->user_interface->Refresh();
            this->entity_render->Refresh();
        }
    }
}