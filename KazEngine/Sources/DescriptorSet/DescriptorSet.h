#pragma once

#include "../Vulkan/Vulkan.h"

namespace Engine
{
    class DescriptorSet
    {
        public :
            
            static VkDescriptorSetLayoutBinding CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags);
            VkDescriptorSet Create();

        private :
            
            VkDescriptorPool pool;
            VkDescriptorSetLayout layout;
            VkDescriptorSet descriptor_set;
    };
}