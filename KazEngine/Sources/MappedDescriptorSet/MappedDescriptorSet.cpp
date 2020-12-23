#include "MappedDescriptorSet.h"
#include "../GlobalData/GlobalData.h"
#include "IMappedDescriptorListener.h"

namespace Engine
{
    MappedDescriptorSet::MappedDescriptorSet()
    {
        this->bindings.clear();

        this->layout            = nullptr;
        this->pool              = nullptr;
    }

    MappedDescriptorSet& MappedDescriptorSet::operator=(MappedDescriptorSet&& other)
    {
        if(&other != this) {
            this->pool = other.pool;
            this->layout = other.layout;
            this->set = std::move(other.set);
            this->layout_bindings = std::move(other.layout_bindings);
            this->bindings = std::move(other.bindings);

            other.pool = nullptr;
            other.layout = nullptr;
        }

        return *this;
    }

    void MappedDescriptorSet::Clear()
    {
        vkDeviceWaitIdle(Vulkan::GetDevice());

        if(this->layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), this->layout, nullptr);
        if(this->pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), this->pool, nullptr);

        // TODO : FreeChunk

        this->bindings.clear();

        this->layout            = nullptr;
        this->pool              = nullptr;
    }

    void MappedDescriptorSet::WriteData(const void* data, size_t size, size_t offset, uint8_t binding)
    {
        GlobalData::GetInstance()->mapped_buffer.WriteData(data, size, this->bindings[binding].chunk->offset + offset);
    }

    void* MappedDescriptorSet::AccessData(size_t offset, uint8_t binding)
    {
        return GlobalData::GetInstance()->mapped_buffer.Data(this->bindings[binding].chunk->offset + offset);
    }

    VkDescriptorSetLayoutBinding MappedDescriptorSet::CreateSimpleBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags)
    {
        VkDescriptorSetLayoutBinding layout_binding;
        layout_binding.binding = binding;
        layout_binding.descriptorType = type;
        layout_binding.stageFlags = stage_flags;
        layout_binding.descriptorCount = 1;
        layout_binding.pImmutableSamplers = nullptr;
        return layout_binding;
    }

    bool MappedDescriptorSet::Create(std::vector<BINDING_INFOS> infos)
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

            size_t alignment = this->GetBindingAlignment(i);
            if(infos[i].size > 0)
                this->bindings[i].chunk = GlobalData::GetInstance()->mapped_buffer.GetChunk()->ReserveRange(infos[i].size, alignment);
            layout_bindings.push_back(this->bindings[i].layout);
        }

        ///////////////////
        // Create layout //
        ///////////////////

        VkDescriptorSetLayoutCreateInfo descriptor_layout;
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.flags = 0;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = static_cast<uint32_t>(layout_bindings.size());
        descriptor_layout.pBindings = layout_bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &descriptor_layout, nullptr, &this->layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DescriptorSet::Create() => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }

        ////////////////////
        // Create du pool //
        ////////////////////

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
            std::cout << "DescriptorSet::Create() => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Allocation du Descriptor Sets //
        ///////////////////////////////////

        std::vector<VkDescriptorSetLayout> layouts = {this->layout};
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = layouts.data();

        result = vkAllocateDescriptorSets(Vulkan::GetDevice(), &alloc_info, &this->set);
        if(result != VK_SUCCESS) {
            this->Clear();
            #if defined(DISPLAY_LOGS)
            std::cout <<"DescriptorSet::Create() => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////
        // Update Descriptor Set //
        ///////////////////////////

        std::vector<VkWriteDescriptorSet> writes;
        std::vector<VkDescriptorBufferInfo> buffer_infos(bindings.size());

        for(uint8_t i=0; i<bindings.size(); i++) {

            if(this->bindings[i].chunk == nullptr) continue;

            VkWriteDescriptorSet write;
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.pNext = nullptr;
            write.dstBinding = this->bindings[i].layout.binding;
            write.dstArrayElement = 0;
            write.descriptorCount = this->bindings[i].layout.descriptorCount;
            write.descriptorType = this->bindings[i].layout.descriptorType;
            write.pTexelBufferView = nullptr;
            write.pImageInfo = nullptr;

            buffer_infos[i] = GlobalData::GetInstance()->mapped_buffer.GetBufferInfos(this->bindings[i].chunk);
            write.dstSet = this->set;
            write.pBufferInfo = &buffer_infos[i];
            writes.push_back(write);
        }
        
        vkUpdateDescriptorSets(Vulkan::GetDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        return true;
    }

    std::shared_ptr<Chunk> MappedDescriptorSet::ReserveRange(size_t size, uint8_t binding)
    {
        if(this->bindings[binding].chunk == nullptr) {
            size_t alignment = this->GetBindingAlignment(binding);
            this->bindings[binding].chunk = GlobalData::GetInstance()->mapped_buffer.GetChunk()->ReserveRange(size, alignment);
            if(this->bindings[binding].chunk != nullptr) {
                std::shared_ptr<Chunk> chunk = this->bindings[binding].chunk->ReserveRange(size);
                for(auto listener : this->Listeners) listener->MappedDescriptorSetUpdated(this, binding);
                return chunk;
            }else{
                return nullptr;
            }
        }

        std::shared_ptr<Chunk> chunk = this->bindings[binding].chunk->ReserveRange(size);

        if(chunk == nullptr) {

            bool relocated;
            size_t old_offset = this->bindings[binding].chunk->offset;
            size_t old_size = this->bindings[binding].chunk->range;
            if(GlobalData::GetInstance()->mapped_buffer.GetChunk()->ResizeChild(
                this->bindings[binding].chunk,
                this->bindings[binding].chunk->range + size,
                relocated, this->GetBindingAlignment(binding)))
            {
                if(relocated) GlobalData::GetInstance()->mapped_buffer.MoveData(old_offset, this->bindings[binding].chunk->offset, old_size);
                chunk = this->bindings[binding].chunk->ReserveRange(size);
                for(auto listener : this->Listeners) listener->MappedDescriptorSetUpdated(this, binding);
            }else{
                return nullptr;
            }
        }

        return chunk;
    }

    void MappedDescriptorSet::Update(uint8_t binding)
    {
        VkDescriptorBufferInfo buffer_infos = GlobalData::GetInstance()->mapped_buffer.GetBufferInfos(this->bindings[binding].chunk);

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = this->bindings[binding].layout.binding;
        write.dstArrayElement = 0;
        write.descriptorCount = this->bindings[binding].layout.descriptorCount;
        write.descriptorType = this->bindings[binding].layout.descriptorType;
        write.pTexelBufferView = nullptr;
        write.pImageInfo = nullptr;
        write.dstSet = this->set;
        write.pBufferInfo = &buffer_infos;

        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &write, 0, nullptr);
    }

    size_t MappedDescriptorSet::GetBindingAlignment(uint8_t binding)
    {
        switch(this->bindings[binding].layout.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
                return Vulkan::UboAlignment();

            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC :
                return Vulkan::SboAlignment();

            default :
                return 0;
        }
    }
}