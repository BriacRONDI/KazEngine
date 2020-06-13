#include "DescriptorSet.h"

namespace Engine
{
    DescriptorSet::DescriptorSet()
    {
        this->sets.clear();
        this->image_buffers.clear();
        this->bindings.clear();

        this->layout            = nullptr;
        this->pool              = nullptr;
        this->sampler           = nullptr;
    }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other)
    {
        if(&other != this) {
            this->pool = other.pool;
            this->layout = other.layout;
            this->sets = std::move(other.sets);
            this->sampler = other.sampler;
            this->image_buffers = std::move(other.image_buffers);
            this->layout_bindings = std::move(other.layout_bindings);
            this->bindings = std::move(other.bindings);

            other.pool = nullptr;
            other.layout = nullptr;
            other.sampler = nullptr;
        }

        return *this;
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

        for(auto& binding : this->bindings)
            DataBank::GetInstance().GetManagedBuffer().FreeChunk(binding.chunk);

        this->sets.clear();
        this->image_buffers.clear();
        this->bindings.clear();

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

    /*std::vector<DescriptorSet::DESCRIPTOR_SET_BINDING> DescriptorSet::CreateBindings(std::vector<BINDING_INFOS> infos)
    {
        std::vector<DESCRIPTOR_SET_BINDING> return_value;

        for(uint32_t i=0; i<infos.size(); i++) {
            
            VkDescriptorSetLayoutBinding binding;
            binding.binding = i;
            binding.descriptorType = infos[i].type;
            binding.stageFlags = infos[i].stage;
            binding.descriptorCount = 1;
            binding.pImmutableSamplers = nullptr;

            Vulkan::DATA_CHUNK chunk = DataBank::GetInstance().GetManagedBuffer().ReserveChunk(infos[i].size);

            return_value.push_back({binding, chunk});
        }

        return return_value;
    }*/

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

    bool DescriptorSet::Create(std::vector<BINDING_INFOS> infos, uint32_t instance_count, std::vector<std::shared_ptr<Chunk>> external_chunk)
    {
        this->Clear();
        this->bindings.resize(infos.size());

        std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
        for(uint32_t i=0; i<infos.size(); i++) {
            
            this->bindings[i].layout.binding = i;
            this->bindings[i].layout.descriptorType = infos[i].type;
            this->bindings[i].layout.stageFlags = infos[i].stage;
            this->bindings[i].layout.descriptorCount = 1;
            this->bindings[i].layout.pImmutableSamplers = nullptr;

            VkDeviceSize alignment = 0;
            switch(this->bindings[i].layout.descriptorType) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
                    alignment = Vulkan::UboAlignment();
                    break;

                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC :
                    alignment = Vulkan::SboAlignment();
                    break;
            }

            if(external_chunk.size() == infos.size() && external_chunk[i] != nullptr) this->bindings[i].chunk = external_chunk[i];
            else this->bindings[i].chunk = DataBank::GetManagedBuffer().ReserveChunk(infos[i].size, alignment);

            layout_bindings.push_back(this->bindings[i].layout);

            this->bindings[i].awaiting_update.resize(instance_count, false);
        }

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
            std::cout << "DescriptorSet::PrepareDescriptor => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////
        // Création du pool //
        //////////////////////

        std::vector<VkDescriptorPoolSize> pool_sizes(layout_bindings.size());
        for(uint8_t i=0; i<pool_sizes.size(); i++) {
            pool_sizes[i].type = layout_bindings[i].descriptorType;
            pool_sizes[i].descriptorCount = layout_bindings[i].descriptorCount * instance_count;
        }

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = instance_count;

		result = vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &this->pool);
		if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::PrepareDescriptor => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        std::vector<VkDescriptorSetLayout> layouts(instance_count, this->layout);
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->pool;
        alloc_info.descriptorSetCount = instance_count;
        alloc_info.pSetLayouts = layouts.data();

        this->sets.resize(instance_count);
        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, this->sets.data());
        if(result != VK_SUCCESS) {
            this->Clear();
            #if defined(DISPLAY_LOGS)
            std::cout <<"DescriptorSet::Create => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Mise à jour du Descriptor Set //
        ///////////////////////////////////

        std::vector<VkWriteDescriptorSet> writes;
        std::vector<VkDescriptorBufferInfo> buffer_infos(bindings.size() * instance_count);

        for(uint8_t i=0; i<bindings.size(); i++) {

            VkWriteDescriptorSet write;
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.pNext = nullptr;
            write.dstBinding = this->bindings[i].layout.binding;
            write.dstArrayElement = 0;
            write.descriptorCount = this->bindings[i].layout.descriptorCount;
            write.descriptorType = this->bindings[i].layout.descriptorType;
            write.pTexelBufferView = nullptr;
            write.pImageInfo = nullptr;

            for(uint32_t j=0; j<instance_count; j++) {
                buffer_infos[i * instance_count + j] = DataBank::GetInstance().GetManagedBuffer().GetBufferInfos(this->bindings[i].chunk, j);
                if(buffer_infos[i * bindings.size() + j].range > 0) {
                    write.dstSet = this->sets[j];
                    write.pBufferInfo = &buffer_infos[i * bindings.size() + j];
                    writes.push_back(write);
                }
            }
        }
        
        // Mise à jour
        vkUpdateDescriptorSets(Vulkan::GetDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout <<"DescriptorSet::Create : Success" << std::endl;
        #endif

        return true;
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

    /*std::vector<Chunk>::iterator const DescriptorSet::ReserveRange(size_t size, uint8_t binding)
    {
        Chunk chunk = this->bindings[binding].chunk->ReserveRange(size);
        return chunk;
    }*/

    std::shared_ptr<Chunk> DescriptorSet::ReserveRange(size_t size, size_t alignment, uint8_t binding)
    {
        std::shared_ptr<Chunk> chunk = this->bindings[binding].chunk->ReserveRange(size, alignment);

        if(chunk == nullptr) {

            VkDeviceSize binding_alignment = 0;
            switch(this->bindings[binding].layout.descriptorType) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
                    binding_alignment = Vulkan::UboAlignment();
                    break;

                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC :
                    binding_alignment = Vulkan::SboAlignment();
                    break;
            }

            bool relocated;
            if(DataBank::GetManagedBuffer().ResizeChunk(this->bindings[binding].chunk,
                                                        this->bindings[binding].chunk->range + size,
                                                        relocated, binding_alignment)) {

                this->AwaitUpdate(binding);
                return this->bindings[binding].chunk->ReserveRange(size, alignment);
            }else{
                return nullptr;
            }
        }

        return chunk;
    }

    bool DescriptorSet::ResizeChunk(size_t size, uint8_t binding, VkDeviceSize alignment)
    {
        VkDeviceSize binding_alignment = 0;
        switch(this->bindings[binding].layout.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
                binding_alignment = Vulkan::UboAlignment();
                break;

            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC :
                binding_alignment = Vulkan::SboAlignment();
                break;
        }

        bool relocated;
        if(DataBank::GetManagedBuffer().ResizeChunk(this->bindings[binding].chunk, size, relocated, binding_alignment)) {
            this->AwaitUpdate(binding);
            return true;
        }else{
            return false;
        }
    }

    bool DescriptorSet::ResizeChunk(std::shared_ptr<Chunk> chunk, size_t size, uint8_t binding, VkDeviceSize alignment)
    {
        bool relocated;
        if(!this->bindings[binding].chunk->ResizeChild(chunk, size, relocated, alignment)) {

            VkDeviceSize binding_alignment = 0;
            switch(this->bindings[binding].layout.descriptorType) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
                    binding_alignment = Vulkan::UboAlignment();
                    break;

                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC :
                    binding_alignment = Vulkan::SboAlignment();
                    break;
            }

            size_t extension = size - chunk->range;
            if(DataBank::GetManagedBuffer().ResizeChunk(this->bindings[binding].chunk,
                                                        this->bindings[binding].chunk->range + extension,
                                                        relocated, binding_alignment)) {
                this->AwaitUpdate(binding);
                if(!this->bindings[binding].chunk->ResizeChild(chunk, size, relocated, alignment)) {
                    std::vector<Chunk::DEFRAG_CHUNK> defrag;
                    if(this->bindings[binding].chunk->Defragment(alignment, defrag, chunk, extension)) {
                        std::vector<std::vector<char>> resized_chunk_data(this->sets.size());
                        for(auto& fragment : defrag) {
                            if(fragment.chunk == chunk) {
                                for(uint8_t i=0; i<resized_chunk_data.size(); i++) {
                                    resized_chunk_data[i].resize(chunk->range - extension);
                                    DataBank::GetManagedBuffer().ReadStagingBuffer(
                                        resized_chunk_data[i], this->bindings[binding].chunk->offset + fragment.old_offset, i);
                                }
                            }else{
                                DataBank::GetManagedBuffer().MoveData(this->bindings[binding].chunk->offset + fragment.old_offset,
                                                                      this->bindings[binding].chunk->offset + fragment.chunk->offset,
                                                                      fragment.chunk->range);
                            }
                        }

                        for(uint8_t i=0; i<resized_chunk_data.size(); i++)
                            this->WriteData(resized_chunk_data[i].data(), resized_chunk_data[i].size(), binding, static_cast<uint32_t>(chunk->offset));

                        return true;
                    }else{
                        return false;
                    }
                }
                return true;
            }else{
                return false;
            }
        }else{
            this->AwaitUpdate(binding);
            return true;
        }
    }

    void DescriptorSet::UpdateDescriptorSet(uint8_t binding, uint8_t instance_id)
    {
        VkDescriptorBufferInfo buffer_infos = DataBank::GetInstance().GetManagedBuffer().GetBufferInfos(this->bindings[binding].chunk, instance_id);

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = this->bindings[binding].layout.binding;
        write.dstArrayElement = 0;
        write.descriptorCount = this->bindings[binding].layout.descriptorCount;
        write.descriptorType = this->bindings[binding].layout.descriptorType;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = nullptr;
        write.dstSet = this->sets[instance_id];
        write.pBufferInfo = &buffer_infos;

        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);
        this->bindings[binding].awaiting_update[instance_id] = false;
    }

    bool DescriptorSet::Update(uint8_t instance_id)
    {
        bool updated = false;
        for(uint8_t i=0; i<this->bindings.size(); i++) {
            if(this->bindings[i].awaiting_update[instance_id]) {
                this->UpdateDescriptorSet(i, instance_id);
                updated = true;
            }
        }

        return updated;
    }

    bool DescriptorSet::NeedUpdate(uint8_t instance_id)
    {
        for(auto& binding : this->bindings)
            if(binding.awaiting_update[instance_id])
                return true;
        return false;
    }
}