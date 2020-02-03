#include "DescriptorSetManager.h"

namespace Engine
{
    /**
     * Initialisation
     */
    DescriptorSetManager::DescriptorSetManager()
    {
        this->view_pool         = nullptr;
        this->view_layout       = nullptr;
        this->view_set          = nullptr;
        this->skeleton_pool     = nullptr;
        this->skeleton_layout   = nullptr;
        this->skeleton_set      = nullptr;
        this->texture_pool      = nullptr;
        this->texture_layout    = nullptr;
        /*this->light_pool        = nullptr;
        this->light_layout      = nullptr;
        this->light_set         = nullptr;*/
    }

    /**
     * Libération des ressources
     */
    void DescriptorSetManager::Clear()
    {
        if(this->sampler != nullptr) vkDestroySampler(Vulkan::GetDevice(), this->sampler, nullptr);
        if(this->view_layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->view_layout, nullptr);
        if(this->skeleton_layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->skeleton_layout, nullptr);
        if(this->texture_layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->texture_layout, nullptr);
        if(this->view_pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->view_pool, nullptr);
        if(this->skeleton_pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->skeleton_pool, nullptr);
        if(this->texture_pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->texture_pool, nullptr);

        this->view_pool             = nullptr;
        this->view_layout           = nullptr;
        this->view_set              = nullptr;
        this->skeleton_pool         = nullptr;
        this->skeleton_layout       = nullptr;
        this->skeleton_set          = nullptr;
        this->texture_pool          = nullptr;
        this->texture_layout        = nullptr;
        this->sampler               = nullptr;
    }

    std::vector<VkDescriptorSetLayout> const DescriptorSetManager::GetLayoutArray(uint16_t schema)
    {
        std::vector<VkDescriptorSetLayout> output = {this->view_layout};
        if(schema & Renderer::TEXTURE) output.push_back(this->texture_layout);
        if(schema & Renderer::SKELETON ||schema & Renderer::SINGLE_BONE) output.push_back(this->skeleton_layout);

        return output;
    }

    /*bool DescriptorSetManager::CreateLightDescriptorSet(VkDescriptorBufferInfo& light_buffer)
    {
        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutBinding descriptor_set_binding;
        descriptor_set_binding.binding             = 0;
        descriptor_set_binding.descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_set_binding.descriptorCount     = 1;
        descriptor_set_binding.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_binding.pImmutableSamplers  = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 1;
        descriptor_layout.pBindings = &descriptor_set_binding;

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->light_layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateLightDescriptorSet => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        VkDescriptorPoolSize pool_size;
        pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_size.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &pool_size;
        poolInfo.maxSets = 1;

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->light_pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateLightDescriptorSet => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->light_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->light_layout;

        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->light_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"CreateLightDescriptorSet => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &light_buffer;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = nullptr;
        write.dstSet = this->light_set;

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateLightDescriptorSet : Success" << std::endl;
        #endif

        return true;
    }*/

    bool DescriptorSetManager::CreateSkeletonDescriptorSet(VkDescriptorBufferInfo const& skeleton_buffer, VkDescriptorBufferInfo const& bone_offsets_buffer)
    {
        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutBinding descriptor_set_bindings[2];
        descriptor_set_bindings[0].binding             = 0;
        descriptor_set_bindings[0].descriptorType      = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        descriptor_set_bindings[0].descriptorCount     = 1;
        descriptor_set_bindings[0].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_bindings[0].pImmutableSamplers  = nullptr;

        descriptor_set_bindings[1].binding             = 1;
        descriptor_set_bindings[1].descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_bindings[1].descriptorCount     = 1;
        descriptor_set_bindings[1].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_bindings[1].pImmutableSamplers  = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 2;
        descriptor_layout.pBindings = descriptor_set_bindings;

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->skeleton_layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSkeletonDescriptorSet => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        VkDescriptorPoolSize pool_sizes[2];

		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		pool_sizes[0].descriptorCount = 1;

        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		pool_sizes[1].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = pool_sizes;
        poolInfo.maxSets = 1;

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->skeleton_pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSkeletonDescriptorSet => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->skeleton_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->skeleton_layout;

        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->skeleton_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"CreateSkeletonDescriptorSet => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        VkWriteDescriptorSet writes[2];

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].pNext = nullptr;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        writes[0].pBufferInfo = &skeleton_buffer;
        writes[0].pTexelBufferView = nullptr;
        writes[0].pImageInfo = nullptr;
        writes[0].dstSet = this->skeleton_set;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].pNext = nullptr;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        writes[1].pBufferInfo = &bone_offsets_buffer;
        writes[1].pTexelBufferView = nullptr;
        writes[1].pImageInfo = nullptr;
        writes[1].dstSet = this->skeleton_set;

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 2, writes, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateSkeletonDescriptorSet : Success" << std::endl;
        #endif

        return true;
    }

    bool DescriptorSetManager::CreateTextureDescriptorSet(VkImageView const view, std::string const& texture)
    {
        ////////////////////////
        // Création du layout //
        ////////////////////////

        if(this->texture_layout == nullptr) {
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

            VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->texture_layout);
            if(result != VK_SUCCESS) {
                #if defined(DISPLAY_LOGS)
                std::cout << "CreateTextureDescriptorSet => vkCreateDescriptorSetLayout : Failed" << std::endl;
                #endif
                return false;
            }
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        if(this->texture_pool == nullptr) {
            VkDescriptorPoolSize pool_size;
		    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		    pool_size.descriptorCount = TEXTURE_ALLOCATION_INCREMENT;

		    VkDescriptorPoolCreateInfo poolInfo = {};
		    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		    poolInfo.poolSizeCount = 1;
		    poolInfo.pPoolSizes = &pool_size;
            poolInfo.maxSets = TEXTURE_ALLOCATION_INCREMENT;

		    VkResult result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->texture_pool);
		    if(result != VK_SUCCESS) {
                #if defined(DISPLAY_LOGS)
                std::cout << "CreateTextureDescriptorSet => vkCreateDescriptorPool : Failed" << std::endl;
                #endif
                return false;
            }
        }

        ///////////////////////////////////
        // Allocation du Descriptor Set //
        ///////////////////////////////////

        if(texture.empty() || view == nullptr) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"CreateTextureDescriptorSet : Success" << std::endl;
            #endif
            return true;
        }

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->texture_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->texture_layout;

        VkResult result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->texture_sets[texture]);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateTextureDescriptorSet => vkAllocateDescriptorSets : Failed [";
            switch(result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY : std::cout << "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY : std::cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
                case VK_ERROR_FRAGMENTED_POOL : std::cout << "VK_ERROR_FRAGMENTED_POOL"; break;
                case VK_ERROR_OUT_OF_POOL_MEMORY : std::cout << "VK_ERROR_OUT_OF_POOL_MEMORY"; break;
            }
            std::cout << "]" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        if(this->sampler == nullptr) {
            if(!this->CreateSampler()) {
                #if defined(DISPLAY_LOGS)
                std::cout <<"CreateTextureDescriptorSet => CreateSampler : Failed" << std::endl;
                #endif
                return false;
            }
        }

        VkDescriptorImageInfo descriptor_image_info;
        descriptor_image_info.sampler = this->sampler;
        descriptor_image_info.imageView = view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pBufferInfo = nullptr;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = &descriptor_image_info;
        write.dstSet = this->texture_sets[texture];

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateTextureDescriptorSet : Success" << std::endl;
        #endif

        return true;
    }

    bool DescriptorSetManager::CreateViewDescriptorSet(VkDescriptorBufferInfo& camera_buffer, VkDescriptorBufferInfo& entity_buffer, bool enable_geometry_shader)
    {
        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutBinding descriptor_set_bindings[2];

        // Camera
        descriptor_set_bindings[0].binding             = 0;
        descriptor_set_bindings[0].descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_set_bindings[0].descriptorCount     = 1;
        descriptor_set_bindings[0].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_bindings[0].pImmutableSamplers  = nullptr;
        if(enable_geometry_shader) descriptor_set_bindings[0].stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

        // Entity
        descriptor_set_bindings[1].binding             = 1;
        descriptor_set_bindings[1].descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_bindings[1].descriptorCount     = 1;
        descriptor_set_bindings[1].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_bindings[1].pImmutableSamplers  = nullptr;
        if(enable_geometry_shader) descriptor_set_bindings[1].stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

        // Light
        /*descriptor_set_bindings[2].binding             = 2;
        descriptor_set_bindings[2].descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_set_bindings[2].descriptorCount     = 1;
        descriptor_set_bindings[2].stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_bindings[2].pImmutableSamplers  = nullptr;*/

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 2;
        descriptor_layout.pBindings = descriptor_set_bindings;

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->view_layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateViewDescriptorSet => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        VkDescriptorPoolSize pool_sizes[2];

        // Camera
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = 1;

        // Entity
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		pool_sizes[1].descriptorCount = 1;

        // Light
        /*pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[2].descriptorCount = 1;*/

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = pool_sizes;
        poolInfo.maxSets = 1;

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->view_pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateViewDescriptorSet => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->view_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->view_layout;

        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->view_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"CreateViewDescriptorSet => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        VkWriteDescriptorSet writes[2];

        // Camera
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].pNext = nullptr;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].pBufferInfo = &camera_buffer;
        writes[0].pTexelBufferView = nullptr;
        writes[0].pImageInfo = nullptr;
        writes[0].dstSet = this->view_set;

        // Entity
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].pNext = nullptr;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        writes[1].pBufferInfo = &entity_buffer;
        writes[1].pTexelBufferView = nullptr;
        writes[1].pImageInfo = nullptr;
        writes[1].dstSet = this->view_set;

        // Light
        /*writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].pNext = nullptr;
        writes[2].dstBinding = 2;
        writes[2].dstArrayElement = 0;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[2].pBufferInfo = &lights_buffer;
        writes[2].pTexelBufferView = nullptr;
        writes[2].pImageInfo = nullptr;
        writes[2].dstSet = this->view_set;*/

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 2, writes, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateViewDescriptorSet : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Création d'un sampler d'images
     */
    bool DescriptorSetManager::CreateSampler()
    {
        VkSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = nullptr;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = VK_FILTER_LINEAR;
        sampler_create_info.minFilter = VK_FILTER_LINEAR;
        sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
}