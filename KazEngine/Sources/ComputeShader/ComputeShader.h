#pragma once

#include "../Vulkan/Vulkan.h"
#include "../GlobalData/GlobalData.h"

namespace Engine
{
    class ComputeShader
    {
        public :

            ComputeShader();
            ~ComputeShader() { this->Clear(); };
            void Clear();
            bool Load(std::string path, std::vector<VkDescriptorSetLayout> descriptor_set_layouts, std::vector<VkPushConstantRange> push_constant_ranges = {});
            VkCommandBuffer BuildCommandBuffer(uint8_t frame_index, std::vector<VkDescriptorSet> descriptor_sets, std::array<uint32_t, 3> count, std::vector<const void*> push_constants = {});
            void Refresh(uint8_t frame_index) { this->refresh[frame_index] = true; }
            void Refresh() { std::fill(this->refresh.begin(), this->refresh.end(), true); }
            std::array<uint32_t, 3> const& GetCount(uint8_t frame_index) const { return this->count[frame_index]; }

        private :

            VkCommandPool command_pool;
            std::vector<bool> refresh;
            std::vector<VkCommandBuffer> command_buffers;
            vk::PIPELINE pipeline;
            std::vector<std::array<uint32_t, 3>> count;
            std::vector<VkPushConstantRange> push_constant_ranges;
    };
}