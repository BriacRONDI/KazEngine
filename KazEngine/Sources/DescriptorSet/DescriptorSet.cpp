#include "DescriptorSet.h"

namespace Engine
{
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

    VkDescriptorSet Create(std::vector<VkDescriptorSetLayoutBinding> layout_bindings)
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
}