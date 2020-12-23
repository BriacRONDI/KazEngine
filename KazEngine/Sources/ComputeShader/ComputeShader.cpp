#include "ComputeShader.h"

namespace Engine
{
    ComputeShader::ComputeShader()
    {
        this->command_pool = nullptr;
    }

    void ComputeShader::Clear()
    {
        vk::Destroy(this->command_pool);
        vk::Destroy(this->pipeline);

        this->command_pool = nullptr;
        this->refresh.clear();
        this->command_buffers.clear();
        this->count.clear();
    }

    bool ComputeShader::Load(std::string path, std::vector<VkDescriptorSetLayout> descriptor_set_layouts, std::vector<VkPushConstantRange> push_constant_ranges)
    {
        this->refresh.resize(Vulkan::GetSwapChainImageCount(), true);
        this->command_buffers.resize(Vulkan::GetSwapChainImageCount());
        this->count.resize(Vulkan::GetSwapChainImageCount());
        this->push_constant_ranges = push_constant_ranges;

        if(!vk::CreateCommandPool(this->command_pool, Vulkan::GetComputeQueue().index)) {
            this->Clear();
            return false;
        }

        for(auto& command_buffer : this->command_buffers) {
            if(!vk::CreateCommandBuffer(this->command_pool, command_buffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY)) {
                this->Clear();
                return false;
            }
        }

        auto compute_shader_stage = vk::LoadShaderModule(path, VK_SHADER_STAGE_COMPUTE_BIT);
        bool success = vk::CreateComputePipeline(compute_shader_stage, descriptor_set_layouts, push_constant_ranges, pipeline);

        vk::Destroy(compute_shader_stage);

        if(!success) {
            this->Clear();
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "ComputeShader::Load(" << path << ") : Success" << std::endl;
        #endif

        return true;
    }

    VkCommandBuffer ComputeShader::BuildCommandBuffer(uint8_t frame_index, std::vector<VkDescriptorSet> descriptor_sets, std::array<uint32_t, 3> count, std::vector<const void*> push_constants)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];

        if(!this->refresh[frame_index]) return command_buffer;
        this->refresh[frame_index] = false;

        this->count[frame_index] = count;

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "ComputeShader::BuildCommandBuffer() => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline.handle);

        for(uint8_t i=0; i<this->push_constant_ranges.size(); i++) {
            vkCmdPushConstants(
                command_buffer, this->pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT,
                this->push_constant_ranges[i].offset, this->push_constant_ranges[i].size,
                push_constants[i]);
        }

        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline.layout, 0,
                                static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);
            
		vkCmdDispatch(command_buffer, count[0], count[1], count[2]);

		vkEndCommandBuffer(command_buffer);

        return command_buffer;
    }
}