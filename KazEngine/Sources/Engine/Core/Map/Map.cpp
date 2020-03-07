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

        // Initialisation de la pipeline
        std::array<std::string, 3> shaders;
        shaders[0] = "./Shaders/textured_map.vert.spv";
        shaders[1] = "./Shaders/textured_map.frag.spv";
        uint16_t schema = Renderer::POSITION_VERTEX | Renderer::UV_VERTEX | Renderer::TEXTURE;
        if(!this->renderer.Initialize(schema, shaders, DescriptorSetManager::GetInstance().GetLayoutArray(schema))) {
            this->DestroyInstance();
            #if defined(DISPLAY_LOGS)
            std::cout << "Map() => this->renderer.Initialize : Failed" << std::endl;
            #endif
            return;
        }

        // DEBUG DATA
        Tools::IMAGE_MAP image = Tools::LoadImageFile("./Assets/grass_tile.png");
        DescriptorSetManager::GetInstance().CreateTextureDescriptorSet(image, "grass");
    }

    Map::~Map()
    {
        if(Vulkan::HasInstance()) {

            // On attend que le GPU arrête de travailler
            vkDeviceWaitIdle(Vulkan::GetDevice());
            if(this->map_data_buffer.GetBuffer().memory != VK_NULL_HANDLE) vkFreeMemory(Vulkan::GetDevice(), this->map_data_buffer.GetBuffer().memory, nullptr);
            if(this->map_data_buffer.GetBuffer().handle != VK_NULL_HANDLE) vkDestroyBuffer(Vulkan::GetDevice(), this->map_data_buffer.GetBuffer().handle, nullptr);
            this->map_data_buffer.Clear();
        }
    }

    void Map::UpdateVertexBuffer()
    {
        return;

        struct SHADER_INPUT {
            Vector3 position;
            Vector2 uv;
        };

        std::vector<SHADER_INPUT> vertex_buffer(4);
        vertex_buffer[0].position = { -1.0f, 0.0f, -1.0f };
        vertex_buffer[0].uv = { 0.0f, 1.0f };
        vertex_buffer[1].position = { -1.0f, 0.0f, 1.0f };
        vertex_buffer[1].uv = { 0.0f, 0.0f };
        vertex_buffer[2].position = { 1.0f, 0.0f, 1.0f };
        vertex_buffer[2].uv = { 1.0f, 0.0f };
        vertex_buffer[3].position = { 1.0f, 0.0f, -1.0f };
        vertex_buffer[3].uv = { 1.0f, 1.0f };

        this->index_buffer_offet = vertex_buffer.size() * sizeof(SHADER_INPUT);
        this->map_data_buffer.WriteData(vertex_buffer.data(), this->index_buffer_offet, 0);

        std::vector<uint32_t> index_buffer = {0, 2, 1, 0, 3, 2};
        this->map_data_buffer.WriteData(index_buffer.data(), index_buffer.size() * sizeof(uint32_t), this->index_buffer_offet);

        this->map_data_buffer.Flush();
    }

    VkCommandBuffer Map::BuildCommandBuffer(uint32_t swap_chain_image_index, VkFramebuffer frame_buffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[swap_chain_image_index];
        this->UpdateVertexBuffer();

        VkCommandBufferInheritanceInfo inheritance_info = {};
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;
        inheritance_info.framebuffer = frame_buffer;
        inheritance_info.renderPass = Vulkan::GetRenderPass();

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = &inheritance_info;

        VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildRenderPass[Map] => vkBeginCommandBuffer : Failed" << std::endl;
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

        std::vector<VkDescriptorSet> bind_descriptor_sets = {
            DescriptorSetManager::GetInstance().GetCameraDescriptorSet(),
            DescriptorSetManager::GetInstance().GetTextureDescriptorSet("grass")
        };

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->renderer.GetPipeline().handle);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->renderer.GetPipeline().layout, 0,
                                static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(), 0, nullptr);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &this->map_data_buffer.GetBuffer().handle, &offset);
        vkCmdBindIndexBuffer(command_buffer, this->map_data_buffer.GetBuffer().handle, this->index_buffer_offet, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

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