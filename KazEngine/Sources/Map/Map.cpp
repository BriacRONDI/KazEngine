#include "Map.h"

namespace Engine
{
    void Map::Clear()
    {
        GlobalData::GetInstance()->dynamic_entity_descriptor.RemoveListener(this);
        GlobalData::GetInstance()->selection_descriptor.RemoveListener(this);

        vk::Destroy(this->pipeline);
        vk::Destroy(this->command_pool);

        this->refresh.clear();
        this->command_buffers.clear();
    }

    bool Map::Initialize()
    {
        this->refresh.resize(Vulkan::GetSwapChainImageCount(), true);
        this->command_buffers.resize(Vulkan::GetSwapChainImageCount());

        if(!vk::CreateCommandPool(this->command_pool, Vulkan::GetGraphicsQueue().index)) {
            this->Clear();
            return false;
        }

        for(auto& command_buffer : this->command_buffers) {
            if(!vk::CreateCommandBuffer(this->command_pool, command_buffer, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) {
                this->Clear();
                return false;
            }
        }

        this->map_vbo_chunk = GlobalData::GetInstance()->instanced_buffer.GetChunk()->ReserveRange(SIZE_KILOBYTE(5));
        if(this->map_vbo_chunk == nullptr) {
            this->Clear();
            return false;
        }

        if(!this->SetupPipeline()) {
            this->Clear();
            return false;
        }

        GlobalData::GetInstance()->dynamic_entity_descriptor.AddListener(this);
        GlobalData::GetInstance()->selection_descriptor.AddListener(this);
        GlobalData::GetInstance()->group_descriptor.AddListener(this);

        return true;
    }

    bool Map::SetupPipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = vk::LoadShaderModule("./Shaders/map.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = vk::LoadShaderModule("./Shaders/map.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        std::vector<VkVertexInputBindingDescription> vertex_binding_description;
        auto vertex_attribute_description = vk::CreateVertexInputDescription({{vk::POSITION, vk::UV}}, vertex_binding_description);

        auto camera_layout = GlobalData::GetInstance()->camera_descriptor.GetLayout();
        auto texture_layout = GlobalData::GetInstance()->texture_descriptor.GetLayout();
        auto selection_layout = GlobalData::GetInstance()->selection_descriptor.GetLayout();
        auto entity_layout = GlobalData::GetInstance()->dynamic_entity_descriptor.GetLayout();
        auto groups_layout = GlobalData::GetInstance()->group_descriptor.GetLayout();

        bool success = vk::CreateGraphicsPipeline(
            true, {camera_layout, texture_layout, selection_layout, entity_layout, groups_layout},
            shader_stages, vertex_binding_description, vertex_attribute_description, {}, this->pipeline
        );

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        if(!success) return false;

        for(uint32_t i=0; i<Vulkan::GetSwapChainImageCount(); i++)
            if(!this->UpdateVertexBuffer(i)) return false;

        return true;
    }

    bool Map::UpdateVertexBuffer(uint8_t frame_index)
    {
        struct SHADER_INPUT {
            Maths::Vector3 position;
            Maths::Vector2 uv;
        };

        float far_clip = 2000.0f;
        std::vector<SHADER_INPUT> vertex_data(8);
        vertex_data[0].position = { -far_clip, 0.0f, -far_clip };
        vertex_data[0].uv = { 0.0f, far_clip };
        vertex_data[1].position = { -far_clip, 0.0f, far_clip };
        vertex_data[1].uv = { 0.0f, 0.0f };
        vertex_data[2].position = { far_clip, 0.0f, far_clip };
        vertex_data[2].uv = { far_clip, 0.0f };
        vertex_data[3].position = { far_clip, 0.0f, -far_clip };
        vertex_data[3].uv = { far_clip, far_clip };

        this->index_buffer_offet = static_cast<uint32_t>(vertex_data.size()) * sizeof(SHADER_INPUT);
        GlobalData::GetInstance()->instanced_buffer.WriteData(vertex_data.data(), this->index_buffer_offet, this->map_vbo_chunk->offset, frame_index);

        std::vector<uint32_t> index_buffer = {0, 2, 1, 0, 3, 2};
        GlobalData::GetInstance()->instanced_buffer.WriteData(index_buffer.data(), index_buffer.size() * sizeof(uint32_t),
                                                              this->map_vbo_chunk->offset + this->index_buffer_offet, frame_index);

        return true;
    }

    VkCommandBuffer Map::BuildCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];
        if(!this->refresh[frame_index]) return command_buffer;
        this->refresh[frame_index] = false;

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
            GlobalData::GetInstance()->camera_descriptor.Get(frame_index),
            GlobalData::GetInstance()->texture_descriptor.Get(),
            GlobalData::GetInstance()->selection_descriptor.Get(frame_index),
            GlobalData::GetInstance()->dynamic_entity_descriptor.Get(),
            GlobalData::GetInstance()->group_descriptor.Get()
        };

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.handle);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.layout, 0,
                                static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(),
                                0, nullptr);

        vkCmdBindVertexBuffers(command_buffer, 0, 1, &GlobalData::GetInstance()->instanced_buffer.GetBuffer(frame_index).handle, &this->map_vbo_chunk->offset);
        vkCmdBindIndexBuffer(command_buffer, GlobalData::GetInstance()->instanced_buffer.GetBuffer(frame_index).handle,
                             this->map_vbo_chunk->offset + this->index_buffer_offet, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "BuildMainRenderPass[Map] => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        return command_buffer;
    }
}