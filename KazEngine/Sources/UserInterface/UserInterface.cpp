#include "UserInterface.h"

namespace Engine
{
    UserInterface::UserInterface(VkCommandPool command_pool) : command_pool(command_pool)
    {
    }

    bool UserInterface::SetupPipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = Vulkan::GetInstance().LoadShaderModule("./Shaders/interface.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = Vulkan::GetInstance().LoadShaderModule("./Shaders/interface.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        size_t chunk_size = Vulkan::GetInstance().ComputeUniformBufferAlignment(SIZE_KILOBYTE(5));
        this->ui_vbo_chunk = DataBank::GetManagedBuffer().ReserveChunk(chunk_size);

        VkVertexInputBindingDescription vertex_binding_description;
        auto vertex_attribute_description = Vulkan::CreateVertexInputDescription({Vulkan::POSITION_2D, Vulkan::UV}, vertex_binding_description);
        
        auto data_buffer = Tools::GetBinaryFileContents("data.kea");
        auto image = DataBank::GetImageFromPackage(data_buffer, "/grass_tile2");
        this->texture_descriptor.Create(image);

        bool success = Vulkan::GetInstance().CreatePipeline(
            true, {this->texture_descriptor.GetLayout()},
            shader_stages, vertex_binding_description, vertex_attribute_description, {}, this->pipeline,
            VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
        );

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        return true;
    }
}