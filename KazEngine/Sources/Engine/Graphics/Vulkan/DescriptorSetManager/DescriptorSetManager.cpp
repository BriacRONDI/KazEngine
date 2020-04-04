#include "DescriptorSetManager.h"

namespace Engine
{
    // Instance du singleton
    DescriptorSetManager* DescriptorSetManager::instance = nullptr;

    void DescriptorSetManager::DestroyInstance()
    {
        if(DescriptorSetManager::instance == nullptr) return;
        delete DescriptorSetManager::instance;
        DescriptorSetManager::instance = nullptr;
    }

    /**
     * Initialisation
     */
    DescriptorSetManager::DescriptorSetManager()
    {
        this->camera_pool       = nullptr;
        this->camera_layout     = nullptr;
        this->camera_set        = nullptr;
        this->entity_pool       = nullptr;
        this->entity_layout     = nullptr;
        this->entity_set        = nullptr;
        this->skeleton_pool     = nullptr;
        this->skeleton_layout   = nullptr;
        this->skeleton_set      = nullptr;
        this->texture_pool      = nullptr;
        this->texture_layout    = nullptr;
    }

    /**
     * Libération des ressources
     */
    void DescriptorSetManager::Clear()
    {
        if(Vulkan::HasInstance()) {

            // On attend que le GPU arrête de travailler
            vkDeviceWaitIdle(Vulkan::GetDevice());

            for(auto& set : this->texture_sets) {
                if(set.second.buffer.view != VK_NULL_HANDLE) vkDestroyImageView(Vulkan::GetDevice(), set.second.buffer.view, nullptr);
                if(set.second.buffer.handle != VK_NULL_HANDLE) vkDestroyImage(Vulkan::GetDevice(), set.second.buffer.handle, nullptr);
                if(set.second.buffer.memory != VK_NULL_HANDLE) vkFreeMemory(Vulkan::GetDevice(), set.second.buffer.memory, nullptr);
            }

            if(this->sampler != nullptr) vkDestroySampler(Vulkan::GetDevice(), this->sampler, nullptr);
            if(this->camera_layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->camera_layout, nullptr);
            if(this->entity_layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->entity_layout, nullptr);
            if(this->skeleton_layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->skeleton_layout, nullptr);
            if(this->texture_layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->texture_layout, nullptr);
            if(this->map_layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->map_layout, nullptr);
            if(this->camera_pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->camera_pool, nullptr);
            if(this->entity_pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->entity_pool, nullptr);
            if(this->skeleton_pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->skeleton_pool, nullptr);
            if(this->texture_pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->texture_pool, nullptr);
            if(this->map_pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->map_pool, nullptr);
        }

        this->texture_sets.clear();
        this->camera_pool           = nullptr;
        this->camera_layout         = nullptr;
        this->camera_set            = nullptr;
        this->entity_pool           = nullptr;
        this->entity_layout         = nullptr;
        this->entity_set            = nullptr;
        this->skeleton_pool         = nullptr;
        this->skeleton_layout       = nullptr;
        this->skeleton_set          = nullptr;
        this->texture_pool          = nullptr;
        this->texture_layout        = nullptr;
        this->map_set               = nullptr;
        this->map_pool              = nullptr;
        this->map_layout            = nullptr;
        this->sampler               = nullptr;
    }

    std::vector<VkDescriptorSetLayout> const DescriptorSetManager::GetLayoutArray(uint16_t schema)
    {
        std::vector<VkDescriptorSetLayout> output = {this->camera_layout};
        if(schema & Renderer::DYNAMIC_MODEL) output.push_back(this->entity_layout);
        if(schema & Renderer::TEXTURE) output.push_back(this->texture_layout);
        if(schema & Renderer::SKELETON ||schema & Renderer::SINGLE_BONE) output.push_back(this->skeleton_layout);
        if(schema & Renderer::MAP_PROPERTIES) output.push_back(this->map_layout);

        return output;
    }

    bool DescriptorSetManager::CreateSkeletonDescriptorSet(VkDescriptorBufferInfo const& meta_skeleton_buffer, VkDescriptorBufferInfo const& skeleton_buffer, VkDescriptorBufferInfo const& bone_offsets_buffer)
    {
        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutBinding descriptor_set_bindings[3];

        descriptor_set_bindings[0].binding             = 0;
        descriptor_set_bindings[0].descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_set_bindings[0].descriptorCount     = 1;
        descriptor_set_bindings[0].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_bindings[0].pImmutableSamplers  = nullptr;

        descriptor_set_bindings[1].binding             = 1;
        descriptor_set_bindings[1].descriptorType      = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_set_bindings[1].descriptorCount     = 1;
        descriptor_set_bindings[1].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_bindings[1].pImmutableSamplers  = nullptr;

        descriptor_set_bindings[2].binding             = 2;
        descriptor_set_bindings[2].descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_bindings[2].descriptorCount     = 1;
        descriptor_set_bindings[2].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_bindings[2].pImmutableSamplers  = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 3;
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

        VkDescriptorPoolSize pool_sizes[3];

		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = 1;
        
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_sizes[1].descriptorCount = 1;

        pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		pool_sizes[2].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 3;
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

        VkWriteDescriptorSet writes[3];

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].pNext = nullptr;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].pBufferInfo = &meta_skeleton_buffer;
        writes[0].pTexelBufferView = nullptr;
        writes[0].pImageInfo = nullptr;
        writes[0].dstSet = this->skeleton_set;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].pNext = nullptr;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &skeleton_buffer;
        writes[1].pTexelBufferView = nullptr;
        writes[1].pImageInfo = nullptr;
        writes[1].dstSet = this->skeleton_set;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].pNext = nullptr;
        writes[2].dstBinding = 2;
        writes[2].dstArrayElement = 0;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        writes[2].pBufferInfo = &bone_offsets_buffer;
        writes[2].pTexelBufferView = nullptr;
        writes[2].pImageInfo = nullptr;
        writes[2].dstSet = this->skeleton_set;

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 3, writes, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateSkeletonDescriptorSet : Success" << std::endl;
        #endif

        return true;
    }

    bool DescriptorSetManager::InitializeTextureLayout()
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
                std::cout << "InitializeTextureLayout => vkCreateDescriptorSetLayout : Failed" << std::endl;
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
                std::cout << "InitializeTextureLayout => vkCreateDescriptorPool : Failed" << std::endl;
                #endif
                return false;
            }
        }

        #if defined(DISPLAY_LOGS)
        std::cout <<"InitializeTextureLayout : Success" << std::endl;
        #endif
        return true;
    }

    bool DescriptorSetManager::CreateTextureDescriptorSet(Tools::IMAGE_MAP const& image, std::string const& texture)
    {
        // Un descriptor set existe déjà pour cette texture
        if(this->texture_sets.count(texture)) return true;

        ////////////////////////////////////////
        // Création du buffer pour accueillir //
        //   l'image dans la carte graphique  //
        ////////////////////////////////////////

        // Création du buffer
        this->texture_sets[texture].buffer = Vulkan::GetInstance().CreateImageBuffer(
                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                             VK_IMAGE_ASPECT_COLOR_BIT, image.width, image.height);

        // En cas d'échec, on renvoie false
        if(this->texture_sets[texture].buffer.handle == nullptr) return false;

        // Envoi de l'image dans la carte graphique
        if(!Vulkan::GetInstance().SendToBuffer(this->texture_sets[texture].buffer, image.data.data(), image.data.size(), image.width, image.height)) return false;

        ///////////////////////////////////
        // Allocation du Descriptor Set //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->texture_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->texture_layout;

        VkResult result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->texture_sets[texture].descriptor);
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
        descriptor_image_info.imageView = this->texture_sets[texture].buffer.view;
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
        write.dstSet = this->texture_sets[texture].descriptor;

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateTextureDescriptorSet : Success" << std::endl;
        #endif

        return true;
    }

    bool DescriptorSetManager::CreateCameraDescriptorSet(VkDescriptorBufferInfo& camera_buffer, bool enable_geometry_shader)
    {
        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutBinding descriptor_set_binding;
        descriptor_set_binding.binding             = 0;
        descriptor_set_binding.descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_set_binding.descriptorCount     = 1;
        descriptor_set_binding.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_binding.pImmutableSamplers  = nullptr;
        if(enable_geometry_shader) descriptor_set_binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 1;
        descriptor_layout.pBindings = &descriptor_set_binding;

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->camera_layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateCameraDescriptorSet => vkCreateDescriptorSetLayout : Failed" << std::endl;
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

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->camera_pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateCameraDescriptorSet => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////////////////
        // Allocation du Descriptor Set //
        //////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->camera_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->camera_layout;

        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->camera_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"CreateCameraDescriptorSet => vkAllocateDescriptorSets : Failed" << std::endl;
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
        write.pBufferInfo = &camera_buffer;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = nullptr;
        write.dstSet = this->camera_set;

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateCameraDescriptorSet : Success" << std::endl;
        #endif

        return true;
    }

    bool DescriptorSetManager::CreateEntityDescriptorSet(VkDescriptorBufferInfo& entity_buffer, bool enable_geometry_shader)
    {
        ////////////////////////
        // Création du layout //
        ////////////////////////

        VkDescriptorSetLayoutBinding descriptor_set_binding;
        descriptor_set_binding.binding             = 0;
        descriptor_set_binding.descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_binding.descriptorCount     = 1;
        descriptor_set_binding.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_binding.pImmutableSamplers  = nullptr;
        if(enable_geometry_shader) descriptor_set_binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 1;
        descriptor_layout.pBindings = &descriptor_set_binding;

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->entity_layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateEntityDescriptorSet => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        VkDescriptorPoolSize pool_size;
        pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		pool_size.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &pool_size;
        poolInfo.maxSets = 1;

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->entity_pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateEntityDescriptorSet => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////////////////
        // Allocation du Descriptor Set //
        //////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->entity_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->entity_layout;

        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->entity_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"CreateEntityDescriptorSet => vkAllocateDescriptorSets : Failed" << std::endl;
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
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.pBufferInfo = &entity_buffer;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = nullptr;
        write.dstSet = this->entity_set;

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateEntityDescriptorSet : Success" << std::endl;
        #endif

        return true;
    }

    bool DescriptorSetManager::CreateMapDescriptorSet(VkDescriptorBufferInfo& map_buffer)
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

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->map_layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateMapDescriptorSet => vkCreateDescriptorSetLayout : Failed" << std::endl;
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

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->map_pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateMapDescriptorSet => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////////////////
        // Allocation du Descriptor Set //
        //////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->map_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->map_layout;

        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->map_set);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"CreateMapDescriptorSet => vkAllocateDescriptorSets : Failed" << std::endl;
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
        write.pBufferInfo = &map_buffer;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = nullptr;
        write.dstSet = this->map_set;

        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"CreateMapDescriptorSet : Success" << std::endl;
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
}