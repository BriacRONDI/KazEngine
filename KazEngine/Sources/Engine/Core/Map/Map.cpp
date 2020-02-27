#include "Map.h"

namespace Engine
{
    // Instance du singleton
    Map* Map::instance = nullptr;

    void Map::DestroyInstance()
    {
        if(Map::instance == nullptr) return;
        delete Map::instance;
        Map::instance = nullptr;
    }

    Map::Map()
    {
        // Création d'un vertex buffer pour y placer la map
        Vulkan::DATA_BUFFER buffer;
        Vulkan::GetInstance().CreateDataBuffer(
                buffer, VERTEX_BUFFER_SIZE,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        this->map_data_buffer.SetBuffer(buffer);

        // Création des command buffers pour l'affichage
        std::vector<Vulkan::COMMAND_BUFFER> render_pass_buffers(Vulkan::GetConcurrentFrameCount());
        if(!Vulkan::GetInstance().CreateCommandBuffer(Vulkan::GetCommandPool(), render_pass_buffers, VK_COMMAND_BUFFER_LEVEL_SECONDARY, false)) return;
        this->command_buffers.resize(Vulkan::GetConcurrentFrameCount());
        for(uint32_t i=0; i<Vulkan::GetConcurrentFrameCount(); i++) this->command_buffers[i] = render_pass_buffers[i].handle;
    }

    Map::~Map()
    {
        if(this->map_data_buffer.GetBuffer().memory != VK_NULL_HANDLE) vkFreeMemory(Vulkan::GetDevice(), this->map_data_buffer.GetBuffer().memory, nullptr);
        if(this->map_data_buffer.GetBuffer().handle != VK_NULL_HANDLE) vkDestroyBuffer(Vulkan::GetDevice(), this->map_data_buffer.GetBuffer().handle, nullptr);
        this->map_data_buffer.Clear();
    }

    void Map::UpdateVertexBuffer()
    {
        
    }

    void Map::GetMapVisiblePlane()
    {
        
    }

    bool Map::BuildRenderPass(uint32_t swap_chain_image_index, VkFramebuffer frame_buffer, std::vector<Renderer> const& renderers)
    {
        VkCommandBuffer command_buffer = this->command_buffers[swap_chain_image_index];

        VkCommandBufferInheritanceInfo inheritance_info = {};
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;
        inheritance_info.framebuffer = frame_buffer;
        inheritance_info.renderPass = Vulkan::GetRenderPass();

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        command_buffer_begin_info.pInheritanceInfo = &inheritance_info;

        VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildRenderPass[Map] => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return false;
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

        

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildMainRenderPass[Map] => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        // Succès
        return true;
    }
}