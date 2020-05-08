#pragma once

#include "../Vulkan/Vulkan.h"
#include "../ManagedBuffer/ManagedBuffer.h"

namespace Engine
{
    class DescriptorSet
    {
        public :
            
            DescriptorSet();
            inline ~DescriptorSet() { this->Clear(); }
            void Clear();
            static VkDescriptorSetLayoutBinding CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags);
            bool Create(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, std::vector<VkDescriptorBufferInfo> const& buffers_infos);
            bool Create(Tools::IMAGE_MAP const& image);
            bool Prepare(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, uint32_t max_sets = 1);
            uint32_t Allocate(std::vector<VkDescriptorBufferInfo> const& buffers_infos);
            bool PrepareBindlessTexture(uint32_t texture_count = 32);
            int32_t AllocateTexture(Tools::IMAGE_MAP const& image);
            inline VkDescriptorSet Get(uint32_t id = 0) { return this->sets[0]; }

            inline VkDescriptorSetLayout GetLayout() { return this->layout; }

        private :

            VkDescriptorPool pool;
            VkDescriptorSetLayout layout;
            std::vector<VkDescriptorSet> sets;
            VkSampler sampler;
            std::vector<Vulkan::IMAGE_BUFFER> image_buffers;
            std::vector<VkDescriptorSetLayoutBinding> layout_bindings;

            bool CreateSampler();
    };
}