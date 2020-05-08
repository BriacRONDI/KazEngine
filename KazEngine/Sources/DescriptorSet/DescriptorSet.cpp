#include "DescriptorSet.h"

namespace Engine
{
    DescriptorSet::DescriptorSet()
    {
        this->sampler = nullptr;
    }

    void DescriptorSet::Clear()
    {
        if(Vulkan::HasInstance()) {
            vkDeviceWaitIdle(Vulkan::GetDevice());

            for(auto& image : this->image_buffers) {
                if(image.view != nullptr) vkDestroyImageView(Vulkan::GetDevice(), image.view, nullptr);
                if(image.handle != nullptr) vkDestroyImage(Vulkan::GetDevice(), image.handle, nullptr);
                if(image.memory != nullptr) vkFreeMemory(Vulkan::GetDevice(), image.memory, nullptr);
            }

            if(this->sampler != nullptr) vkDestroySampler(Vulkan::GetDevice(), this->sampler, nullptr);
            if(this->layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->layout, nullptr);
            if(this->pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->pool, nullptr);
        }

        this->sets.clear();
        this->image_buffers.clear();

        this->layout            = nullptr;
        this->pool              = nullptr;
        this->sampler           = nullptr;
    }

    VkDescriptorSetLayoutBinding DescriptorSet::CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags)
    {
        VkDescriptorSetLayoutBinding layout_binding;
        layout_binding.binding = binding;
        layout_binding.descriptorType = type;
        layout_binding.stageFlags = stage_flags;
        layout_binding.descriptorCount = 1;
        layout_binding.pImmutableSamplers = nullptr;
        return layout_binding;
    }

    bool DescriptorSet::CreateSampler()
    {
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

    bool DescriptorSet::Create(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, std::vector<VkDescriptorBufferInfo> const& buffers_infos)
    {
        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = static_cast<uint32_t>(layout_bindings.size());
        descriptor_layout.pBindings = layout_bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::Create => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        std::vector<VkDescriptorPoolSize> pool_sizes(layout_bindings.size());
        for(uint8_t i=0; i<pool_sizes.size(); i++) {
            pool_sizes[i].type = layout_bindings[i].descriptorType;
            pool_sizes[i].descriptorCount = layout_bindings[i].descriptorCount;
        }

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = 1;

        result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::Create => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->layout;

        VkDescriptorSet new_set;
        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &new_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"DescriptorSet::Create => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        this->sets.push_back(new_set);

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        std::vector<VkWriteDescriptorSet> writes(layout_bindings.size());

        for(uint8_t i=0; i<writes.size(); i++) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].pNext = nullptr;
            writes[i].dstBinding = layout_bindings[i].binding;
            writes[i].dstArrayElement = 0;
            writes[i].descriptorCount = layout_bindings[i].descriptorCount;
            writes[i].descriptorType = layout_bindings[i].descriptorType;
            writes[i].pBufferInfo = &buffers_infos[i];
            writes[i].pTexelBufferView = nullptr;
            writes[i].pImageInfo = nullptr;
            writes[i].dstSet = this->sets[0];
        }

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"DescriptorSet::Create : Success" << std::endl;
        #endif

        return true;
    }

    bool DescriptorSet::Create(Tools::IMAGE_MAP const& image)
    {
        if(!this->CreateSampler()) return false;

        this->image_buffers.resize(1);
        this->image_buffers[0] = Vulkan::GetInstance().CreateImageBuffer(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                         VK_IMAGE_ASPECT_COLOR_BIT, image.width, image.height);
        if(this->image_buffers[0].handle == nullptr) return false;

        size_t bytes_sent = Vulkan::GetInstance().SendToBuffer(this->image_buffers[0], image);
        if(!bytes_sent) return false;

        VkDescriptorImageInfo image_infos;
        image_infos.imageView = this->image_buffers[0].view;
        image_infos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_infos.sampler = this->sampler;

        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutBinding descriptor_set_binding;

        descriptor_set_binding.binding             = 0;
        descriptor_set_binding.descriptorType      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_binding.descriptorCount     = 1;
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
            std::cout << "DescriptorSet::Create => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        VkDescriptorPoolSize pool_size;
        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &pool_size;
        poolInfo.maxSets = 1;

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::Create => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->layout;

        VkDescriptorSet new_set;
        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &new_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"DescriptorSet::Create => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        this->sets.push_back(new_set);

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        VkWriteDescriptorSet write;

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pBufferInfo = nullptr;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = &image_infos;
        write.dstSet = this->sets[0];

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"DescriptorSet::Create : Success" << std::endl;
        #endif

        return true;
    }

    bool DescriptorSet::Prepare(std::vector<VkDescriptorSetLayoutBinding> const& layout_bindings, uint32_t max_sets)
    {
        this->layout_bindings = layout_bindings;

        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = static_cast<uint32_t>(this->layout_bindings.size());
        descriptor_layout.pBindings = this->layout_bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::PrepareDescriptor => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        std::vector<VkDescriptorPoolSize> pool_sizes(this->layout_bindings.size());
        for(uint8_t i=0; i<pool_sizes.size(); i++) {
            pool_sizes[i].type = this->layout_bindings[i].descriptorType;
            pool_sizes[i].descriptorCount = this->layout_bindings[i].descriptorCount * max_sets;
        }

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = max_sets;

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::PrepareDescriptor => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    uint32_t DescriptorSet::Allocate(std::vector<VkDescriptorBufferInfo> const& buffers_infos)
    {
        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->layout;

        VkDescriptorSet new_set;
        VkResult result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &new_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"DescriptorSet::Create => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        uint32_t set_id = static_cast<uint32_t>(this->sets.size());
        this->sets.push_back(new_set);

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        std::vector<VkWriteDescriptorSet> writes(this->layout_bindings.size());

        for(uint8_t i=0; i<writes.size(); i++) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].pNext = nullptr;
            writes[i].dstBinding = this->layout_bindings[i].binding;
            writes[i].dstArrayElement = 0;
            writes[i].descriptorCount = this->layout_bindings[i].descriptorCount;
            writes[i].descriptorType = this->layout_bindings[i].descriptorType;
            writes[i].pBufferInfo = &buffers_infos[i];
            writes[i].pTexelBufferView = nullptr;
            writes[i].pImageInfo = nullptr;
            writes[i].dstSet = this->sets[set_id];
        }

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"DescriptorSet::Create : Success" << std::endl;
        #endif

        return set_id;
    }

    bool DescriptorSet::PrepareBindlessTexture(uint32_t texture_count)
    {
        if(!this->CreateSampler()) return false;

        ////////////////////////
        // Création du layout //
        ////////////////////////

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

        //////////////////////
        // Création du pool //
        //////////////////////

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

        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->layout;

        VkDescriptorSet new_set;
        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &new_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"DescriptorSet::PrepareBindlessTexture => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        this->sets.push_back(new_set);

        return true;
    }

    int32_t DescriptorSet::AllocateTexture(Tools::IMAGE_MAP const& image)
    {
        Vulkan::IMAGE_BUFFER image_buffer;
        image_buffer = Vulkan::GetInstance().CreateImageBuffer(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                               VK_IMAGE_ASPECT_COLOR_BIT, image.width, image.height);
        if(image_buffer.handle == nullptr) return -1;

        size_t bytes_sent = Vulkan::GetInstance().SendToBuffer(image_buffer, image);
        if(!bytes_sent) return -1;

        VkDescriptorImageInfo image_infos;
        image_infos.imageView = image_buffer.view;
        image_infos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_infos.sampler = this->sampler;

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        int32_t index = static_cast<int32_t>(this->image_buffers.size());
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
        write.dstSet = this->sets[0];

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"DescriptorSet::Create : Success" << std::endl;
        #endif

        this->image_buffers.push_back(image_buffer);

        return index;
    }
}