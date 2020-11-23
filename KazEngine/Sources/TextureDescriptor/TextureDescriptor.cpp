#include "TextureDescriptor.h"
#include "../Vulkan/Vulkan.h"

namespace Engine
{
    TextureDescriptor::TextureDescriptor()
    {
        this->pool            = nullptr;
        this->set             = nullptr;
        this->layout          = nullptr;
        this->sampler         = nullptr;
        this->transfer_pool   = nullptr;
        this->transfer_buffer = nullptr;
        this->transfer_fence  = nullptr;
    }

    void TextureDescriptor::Clear()
    {
        vk::Destroy(this->transfer_fence);
        vk::Destroy(this->transfer_pool);
        vk::Destroy(this->sampler);
        vk::Destroy(this->layout);
        vk::Destroy(this->pool);
        vk::Destroy(this->staging_buffer);

        bool default_image_erased = false;
        for(auto& image : this->image_buffers) {
            if(image.first.empty()) {
                if(image.second.handle != nullptr && !default_image_erased) {
                    image.second.Clear();
                    default_image_erased = true;
                }
            }else{
                image.second.Clear();
            }
        }
        this->image_buffers.clear();

        this->pool            = nullptr;
        this->set             = nullptr;
        this->layout          = nullptr;
        this->sampler         = nullptr;
        this->transfer_pool   = nullptr;
        this->transfer_buffer = nullptr;
        this->transfer_fence  = nullptr;
    }

    bool TextureDescriptor::AllocateTransferResources()
    {
        if(this->set != nullptr) return true;

        if(!vk::CreateCommandPool(this->transfer_pool, Vulkan::GetTransferQueue().index)) return false;
        if(!vk::CreateFence(this->transfer_fence, false)) return false;
        if(!vk::CreateCommandBuffer(this->transfer_pool, this->transfer_buffer)) return false;
        if(!vk::CreateDataBuffer(this->staging_buffer, SIZE_MEGABYTE(5), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, {Vulkan::GetTransferQueue().index})) return false;

        constexpr auto offset = 0;
        VkResult result = vkMapMemory(Vulkan::GetDevice(), this->staging_buffer.memory, offset, SIZE_MEGABYTE(5), 0, (void**)&this->staging_buffer.pointer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "TextureDescriptor::AllocateTransferResources() => vkMapMemory(staging_buffer) : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    bool TextureDescriptor::CreateSampler()
    {
        if(this->sampler != nullptr) return true;

        VkSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = nullptr;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = VK_FILTER_LINEAR;
        sampler_create_info.minFilter = VK_FILTER_LINEAR;
        sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_create_info.mipLodBias = 0.0f;
        sampler_create_info.anisotropyEnable = VK_FALSE;
        sampler_create_info.maxAnisotropy = 1.0f;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_create_info.minLod = 0.0f;
        sampler_create_info.maxLod = 0.0f;
        sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;

        VkResult result = vkCreateSampler(Vulkan::GetDevice(), &sampler_create_info, nullptr, &this->sampler);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSampler => vkCreateSampler : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    bool TextureDescriptor::PrepareBindlessTexture(uint32_t texture_count)
    {
        if(!this->CreateSampler()) return false;
        if(!this->AllocateTransferResources()) return false;

        VkDescriptorSetLayoutBinding descriptor_set_binding;

        descriptor_set_binding.binding             = 0;
        descriptor_set_binding.descriptorType      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_binding.descriptorCount     = texture_count;
        descriptor_set_binding.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_binding.pImmutableSamplers  = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 1;
        descriptor_layout.pBindings = &descriptor_set_binding;

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::PrepareBindlessTexture => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        VkDescriptorPoolSize pool_size;
        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size.descriptorCount = texture_count;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &pool_size;
        poolInfo.maxSets = 1;

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::PrepareBindlessTexture => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->layout;

        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"DescriptorSet::PrepareBindlessTexture => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        vk::IMAGE_BUFFER image_buffer = vk::CreateImageBuffer(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 16, 16);
        
        vk::TransitionImageLayout(image_buffer,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                this->transfer_buffer, this->transfer_fence);
        
        this->image_buffers.resize(texture_count, {"", image_buffer});

        VkDescriptorImageInfo image_infos;
        image_infos.imageView = image_buffer.view;
        image_infos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_infos.sampler = this->sampler;

        std::vector<VkDescriptorImageInfo> image_info_array(texture_count, image_infos);

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = texture_count;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pBufferInfo = nullptr;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = image_info_array.data();
        write.dstSet = this->set;

        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        return true;
    }

    bool TextureDescriptor::AllocateTexture(Tools::IMAGE_MAP image, std::string name)
    {
        vk::IMAGE_BUFFER image_buffer = vk::CreateImageBuffer(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                              VK_IMAGE_ASPECT_COLOR_BIT, image.width, image.height);
        if(image_buffer.handle == nullptr) return false;

        size_t bytes_sent = vk::SendImageToBuffer(image_buffer, image, this->transfer_buffer, this->transfer_fence, this->staging_buffer);
        if(!bytes_sent) return false;

        vk::TransitionImageLayout(image_buffer,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                this->transfer_buffer, this->transfer_fence);

        VkDescriptorImageInfo image_infos;
        image_infos.imageView = image_buffer.view;
        image_infos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_infos.sampler = this->sampler;

        uint32_t index = 0;
        for(uint32_t i=0; i<this->image_buffers.size(); i++) if(this->image_buffers[i].first.empty()) { index = i; break; }
        VkWriteDescriptorSet write;

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = 0;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pBufferInfo = nullptr;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = &image_infos;
        write.dstSet = this->set;

        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        this->image_buffers[index] = {name, image_buffer};

        return true;
    }
}