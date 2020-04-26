#include "Map.h"

namespace Engine
{
    Map::Map(VkCommandPool command_pool, std::vector<ManagedBuffer>& uniform_buffers) : command_pool(command_pool), uniform_buffers(uniform_buffers)
    {
        // Allocate command buffers
        uint32_t count = Vulkan::GetConcurrentFrameCount();
        this->command_buffers.resize(count);
        if(!Vulkan::GetInstance().AllocateCommandBuffer(command_pool, count, this->command_buffers, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Map::Map() => AllocateCommandBuffer : Failed" << std::endl;
            #endif
        }

        this->CreatePipeline();
    }

    bool Map::CreatePipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = Vulkan::GetInstance().LoadShaderModule("./Shaders/map.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = Vulkan::GetInstance().LoadShaderModule("./Shaders/map.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            DescriptorSet::CreateSimpleBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), // Camera
            DescriptorSet::CreateSimpleBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)  // Properties
        };

        for(auto uniform_buffer : this->uniform_buffers) {

            std::vector<VkDescriptorBufferInfo> buffers_infos(2);
            buffers_infos[0].buffer = uniform_buffer.GetBuffer().handle;
            buffers_infos[0].offset = 0;
            buffers_infos[0].range = Vulkan::GetInstance().ComputeUniformBufferAlignment(sizeof(Camera::CAMERA_UBO));

            buffers_infos[1].buffer = uniform_buffer.GetBuffer().handle;
            buffers_infos[1].offset = buffers_infos[0].range;
            buffers_infos[1].range = Vulkan::GetInstance().ComputeUniformBufferAlignment(sizeof(MAP_UBO));

            DescriptorSet descriptor;
            descriptor.Create(bindings, buffers_infos);
            this->map_descriptors.push_back(descriptor);
        }

        VkVertexInputBindingDescription vertex_binding_description;
        auto vertex_attribute_description = Vulkan::CreateVertexInputDescription({Vulkan::POSITION, Vulkan::UV}, vertex_binding_description);
        
        auto data_buffer = Tools::GetBinaryFileContents("data.kea");
        auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
        auto image_package = DataPacker::Packer::FindPackedItem(data_tree, "/grass_tile2");
        auto image = Tools::LoadImageData(image_package.Data(data_buffer.data()), image_package.size);
        this->texture_descriptor.Create(image);

        bool success = Vulkan::GetInstance().CreatePipeline(
            true, {this->texture_descriptor.GetLayout(), this->map_descriptors[0].GetLayout()},
            shader_stages, vertex_binding_description, vertex_attribute_description, {}, this->pipeline
        );

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        return true;
    }

    Map::~Map()
    {
        if(Vulkan::HasInstance()) {
            vkFreeCommandBuffers(Vulkan::GetDevice(), this->command_pool, static_cast<uint32_t>(this->command_buffers.size()), this->command_buffers.data());
        }
    }

    VkCommandBuffer Map::GetCommandBuffer(uint8_t frame_index, VkFramebuffer frame_buffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];
        // this->UpdateVertexBuffer();

        VkCommandBufferInheritanceInfo inheritance_info = {};
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;
        inheritance_info.framebuffer = frame_buffer;
        inheritance_info.renderPass = Vulkan::GetRenderPass();

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
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
            this->texture_descriptor.Get(), this->map_descriptors[frame_index].Get()
        };

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.handle);

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.layout, 0,
                                static_cast<uint32_t>(bind_descriptor_sets.size()), bind_descriptor_sets.data(), 0, nullptr);

        VkDeviceSize offset = 0;
        // vkCmdBindVertexBuffers(command_buffer, 0, 1, &this->map_data_buffer.GetBuffer().handle, &offset);
        // vkCmdBindIndexBuffer(command_buffer, this->map_data_buffer.GetBuffer().handle, this->index_buffer_offet, VK_INDEX_TYPE_UINT32);
        // vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

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