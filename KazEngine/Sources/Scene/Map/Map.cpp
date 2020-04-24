#include "Map.h"

namespace Engine
{
    Map::Map(VkCommandPool command_pool) : command_pool(command_pool)
    {
        // Allocate command buffers
        uint32_t count = Vulkan::GetConcurrentFrameCount();
        this->command_buffers.resize(count);
        if(!Vulkan::GetInstance().AllocateCommandBuffer(command_pool, count, this->command_buffers, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Map::Map() => AllocateCommandBuffer : Failed" << std::endl;
            #endif
        }
    }

    bool Map::CreatePipeline()
    {
        std::vector<VkVertexInputAttributeDescription> vertex_attribute_description;

        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
        shader_stages[0] = Vulkan::GetInstance().LoadShaderModule("map.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = Vulkan::GetInstance().LoadShaderModule("map.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        /*bool success = Vulkan::GetInstance().CreatePipeline(
            true,
            descriptor_set_layouts,
            shader_stages, vertex_binding_description,
            vertex_attribute_description,
            push_constant_ranges,
            this->pipeline,
            polygon_mode
        );*/

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        /*bool CreatePipeline(bool dynamic_viewport,
                                std::vector<VkDescriptorSetLayout> const& descriptor_set_layouts,
                                std::vector<VkPipelineShaderStageCreateInfo> const& shader_stages,
                                VkVertexInputBindingDescription const& vertex_binding_description,
                                std::vector<VkVertexInputAttributeDescription> const& vertex_attribute_descriptions,
                                std::vector<VkPushConstantRange> const& push_constant_rages,
                                PIPELINE& pipeline,
                                VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL);*/
    }

    Map::~Map()
    {
        if(Vulkan::HasInstance()) {
            vkFreeCommandBuffers(Vulkan::GetDevice(), this->command_pool, static_cast<uint32_t>(this->command_buffers.size()), this->command_buffers.data());
        }
    }

    VkCommandBuffer Map::GetCommandBuffer(uint8_t frame_index)
    {
        return this->command_buffers[frame_index];
    }
}