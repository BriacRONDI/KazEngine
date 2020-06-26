#include "Core.h"

namespace Engine
{
    void Core::Clear()
    {
        // Entity
        StaticEntity::Clear();
        DynamicEntity::Clear();

        // User Interface
        if(this->user_interface != nullptr) {
            this->user_interface->RemoveListener(this);
            delete this->user_interface;
        }

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
    }

    bool Core::Initialize()
    {
        // Clean all objects
        this->Clear();
        
        // Listen to window events
        Vulkan::GetInstance().GetDrawWindow()->AddListener(this);

        // Create a storage for engine objects
        DataBank::CreateInstance();

        // Alocate vulkan resources
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
        DataBank::GetManagedBuffer().Allocate(SIZE_MEGABYTE(100), MULTI_USAGE_BUFFER_MASK, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                     frame_count, true, {Vulkan::GetGraphicsQueue().index});

        // Initialize Entity
        StaticEntity::Initialize();
        DynamicEntity::Initialize();

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

        return true;
    }

    void Core::DestroyRenderingResources()
    {
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

        // Exécution des render passes secondaires
        VkCommandBuffer command_buffers[2] = {
            this->entity_render->GetCommandBuffer(swap_chain_image_index, frame_buffer),
            this->map->GetCommandBuffer(swap_chain_image_index, frame_buffer)
        };
        vkCmdExecuteCommands(command_buffer, 2, command_buffers);

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
        this->map->UpdateSelection(DynamicEntity::SquareSelection(box_start, box_end));
    }

    void Core::ToggleSelection(Point<uint32_t> mouse_position)
    {
        DynamicEntity* entity = DynamicEntity::ToggleSelection(mouse_position);
        if(entity == nullptr) this->map->UpdateSelection({});
        else this->map->UpdateSelection({entity});
    }

    void Core::SizeChanged(Area<uint32_t> size)
    {
        Vulkan::GetInstance().OnWindowSizeChanged();

        // On recréé la matrice de projection en cas de changement de ratio
        auto& surface = Vulkan::GetDrawSurface();
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
        std::chrono::high_resolution_clock timer;
        auto start_time = timer.now();

        static uint32_t current_semaphore_index = (current_semaphore_index + 1) % this->swap_chain_semaphores.size();
        VkSemaphore present_semaphore = this->swap_chain_semaphores[current_semaphore_index];

        // Acquire image
        uint32_t image_index;
        if(!Vulkan::GetInstance().AcquireNextImage(image_index, present_semaphore)) return;

        // Update global timer
        Timer::Update();

        auto prepare_time = timer.now();

        // Update uniform buffer
        Camera::GetInstance().Update(image_index);
        auto update_camera_time = timer.now();
        this->map->Update(image_index);
        auto update_map_time = timer.now();
        auto update_entities_time = timer.now();
        this->user_interface->Update(image_index);
        auto update_interface_time = timer.now();

        DynamicEntity::UpdateAnimationTimer();

        DataBank::GetManagedBuffer().Flush(this->transfer_buffers[image_index], image_index);

        auto update_time = timer.now();

        VkResult result = vkWaitForFences(Vulkan::GetDevice(), 1, &this->resources[image_index].fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Core::BuildRenderPass => vkWaitForFences : Timeout" << std::endl;
            #endif
            return;
        }
        vkResetFences(Vulkan::GetDevice(), 1, &this->resources[image_index].fence);

        auto fence_time = timer.now();

        if(StaticEntity::GetMatrixDescriptor().NeedUpdate(image_index)) {
            // this->map->Refresh(image_index);
            this->entity_render->Refresh(image_index);
            StaticEntity::UpdateMatrixDescriptor(image_index);
        }

        if(DynamicEntity::GetMatrixDescriptor().NeedUpdate(image_index)) {
            this->map->Refresh(image_index);
            this->entity_render->Refresh(image_index);
            DynamicEntity::UpdateMatrixDescriptor(image_index);
        }

        this->map->UpdateDescriptorSet(image_index);
        this->entity_render->UpdateDescriptorSet(image_index);

        auto descriptor_time = timer.now();

        // Build image
        this->BuildRenderPass(image_index);
        this->user_interface->BuildCommandBuffer(image_index, this->resources[image_index].framebuffer);

        auto build_time = timer.now();

        VkSemaphore compute_semaphore = this->entity_render->SubmitComputeShader(image_index);

        std::vector<VkSemaphore> wait_semaphores = {present_semaphore, compute_semaphore};
        std::vector<VkPipelineStageFlags> semaphore_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};

        // std::vector<VkSemaphore> wait_semaphores = {present_semaphore};
        // std::vector<VkPipelineStageFlags> semaphore_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

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

        auto finish_time = timer.now();

        auto total_duration = finish_time - start_time;
        float prepare_duration = (float)((prepare_time - start_time).count() * 100.0f) / total_duration.count();
        float update_duration = (float)((update_time - prepare_time).count() * 100.0f) / total_duration.count();
        float fence_duration = (float)((fence_time - update_time).count() * 100.0f) / total_duration.count();
        float descriptor_duration = (float)((descriptor_time - fence_time).count() * 100.0f) / total_duration.count();
        float build_duration = (float)((build_time - descriptor_time).count() * 100.0f) / total_duration.count();
        float present_duration = (float)((finish_time - build_time).count() * 100.0f) / total_duration.count();

        float update_camera_duration = (float)((update_camera_time - prepare_time).count() * 100.0f) / (update_time - prepare_time).count();
        float update_map_duration = (float)((update_map_time - update_camera_time).count() * 100.0f) / (update_time - prepare_time).count();
        float update_entities_duration = (float)((update_entities_time - update_map_time).count() * 100.0f) / (update_time - prepare_time).count();
        float update_interface_duration = (float)((update_interface_time - update_entities_time).count() * 100.0f) / (update_time - prepare_time).count();
        float flush_duration = (float)((update_time - update_interface_time).count() * 100.0f) / (update_time - prepare_time).count();
    }
}