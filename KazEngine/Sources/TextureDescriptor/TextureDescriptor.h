#pragma once

#include <Tools.h>
#include <map>
#include "../Vulkan/VulkanTools.h"

namespace Engine
{
    class TextureDescriptor
    {
        public :

            TextureDescriptor();
            ~TextureDescriptor() { this->Clear(); }
            void Clear();
            bool PrepareBindlessTexture(uint32_t texture_count);
            bool AllocateTexture(Tools::IMAGE_MAP image, std::string name);
            int32_t GetTextureID(std::string name) { for(uint32_t i=0; i<this->image_buffers.size(); i++) if(this->image_buffers[i].first == name) return i; return -1; }
            VkDescriptorSetLayout const GetLayout() const { return this->layout; }
            VkDescriptorSet Get() { return this->set; }

        private :

            VkDescriptorPool pool;
            VkDescriptorSet set;
            VkDescriptorSetLayout layout;
            VkSampler sampler;
            VkCommandPool transfer_pool;
            VkCommandBuffer transfer_buffer;
            VkFence transfer_fence;
            vk::MAPPED_BUFFER staging_buffer;
            std::vector<std::pair<std::string, vk::IMAGE_BUFFER>> image_buffers;

            bool CreateSampler();
            bool AllocateTransferResources();
    };
}