#include "Core.h"

namespace Engine
{
    void Core::Clear()
    {
        if(this->map != nullptr) delete this->map;

        this->current_frame_index = 0;
        this->graphics_command_pool = nullptr;
        this->map = nullptr;
    }

    bool Core::Initialize()
    {
        this->Clear();

        if(!this->AllocateRenderingResources()) return false;

        uint8_t frame_count = Vulkan::GetConcurrentFrameCount();
        std::vector<uint32_t> queue_families = {Vulkan::GetGraphicsQueue().index};
        if(Vulkan::GetGraphicsQueue().index != Vulkan::GetTransferQueue().index) queue_families.push_back(Vulkan::GetTransferQueue().index);

        this->game_buffers.resize(frame_count);
        for(uint8_t i=0; i<frame_count; i++) {

            if(!Vulkan::GetInstance().CreateDataBuffer(this->staging_buffers[i], 1024 * 1024 * 5,
                                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                       queue_families)) {
                this->Clear();
                return false;
            }

            if(!this->game_buffers[i].Create(this->staging_buffers[i], 1024 * 1024 * 5, MULTI_USAGE_BUFFER_MASK, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                this->Clear();
                return false;
            }
        }

        this->map = new Map(this->graphics_command_pool);

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
            if(!Vulkan::GetInstance().CreateSemaphore(this->resources[i].renderpass_semaphore)) return false;
            if(!Vulkan::GetInstance().CreateSemaphore(this->resources[i].swap_chain_semaphore)) return false;
        }

        // Graphics Command Buffers
        std::vector<Vulkan::COMMAND_BUFFER> graphics_buffers(frame_count);
        if(!Vulkan::GetInstance().CreateCommandBuffer(this->graphics_command_pool, graphics_buffers)) return false;
        for(uint32_t i=0; i<this->resources.size(); i++) this->resources[i].graphics_command_buffer = graphics_buffers[i];

        return true;
    }

    bool Core::BuildRenderPass(uint32_t swap_chain_image_index)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        VkCommandBuffer command_buffer = this->resources[swap_chain_image_index].graphics_command_buffer.handle;
        VkFramebuffer frame_buffer = this->resources[swap_chain_image_index].framebuffer;

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
        /*VkCommandBuffer command_buffers[2] = {
            this->main_render_pass_command_buffers[swap_chain_image_index],
            Map::GetInstance().BuildCommandBuffer(swap_chain_image_index, frame_buffer)
        };
        vkCmdExecuteCommands(command_buffer, 2, command_buffers);*/
        // vkCmdExecuteCommands(command_buffer, 1, &this->main_render_pass_command_buffers[swap_chain_image_index]);

        VkCommandBuffer map_buffer = this->map->GetCommandBuffer(swap_chain_image_index);
        vkCmdExecuteCommands(command_buffer, 1, &map_buffer);

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

    void Core::Loop()
    {
        this->current_frame_index = (this->current_frame_index + 1) % static_cast<uint8_t>(Vulkan::GetConcurrentFrameCount());
        Vulkan::RENDERING_RESOURCES current_rendering_resource = this->resources[this->current_frame_index];

        uint32_t swap_chain_image_index;
        if(!Vulkan::GetInstance().AcquireNextImage(current_rendering_resource, swap_chain_image_index)) return;

        this->BuildRenderPass(swap_chain_image_index);

        if(!Vulkan::GetInstance().PresentImage(current_rendering_resource, swap_chain_image_index)) {

            // On recréé la matrice de projection en cas de changement de ration
            // auto& surface = Vulkan::GetDrawSurface();
            // float aspect_ratio = static_cast<float>(surface.width) / static_cast<float>(surface.height);
            // Camera::GetInstance().GetUniformBuffer().projection = Matrix4x4::PerspectiveProjectionMatrix(aspect_ratio, 60.0f, 0.1f, 100.0f);

            // Reconstruction du Frame Buffer et des Command Buffers
            this->RebuildFrameBuffers();
        }
    }
}