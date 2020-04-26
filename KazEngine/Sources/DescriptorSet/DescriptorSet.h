#pragma once

#include "../Vulkan/Vulkan.h"

namespace Engine
{
    class DescriptorSet
    {
        public :
            
            DescriptorSet();
            static VkDescriptorSetLayoutBinding CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags);
            bool Create(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, std::vector<VkDescriptorBufferInfo> const& buffers_infos);
            bool Create(Tools::IMAGE_MAP const& image);
            inline VkDescriptorSet Get() { return this->descriptor_set; }

            inline VkDescriptorSetLayout GetLayout() { return this->layout; }

        private :
            
            VkDescriptorPool pool;
            VkDescriptorSetLayout layout;
            VkDescriptorSet descriptor_set;
            VkSampler sampler;

            Vulkan::IMAGE_BUFFER image_buffer;

            bool CreateSampler();
    };
}