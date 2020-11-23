#pragma once

#include "VulkanTools.h"
#include "Vulkan.h"

namespace Engine { namespace vk
{
    void Destroy(VkCommandPool pool) { if(pool != nullptr) vkDestroyCommandPool(Vulkan::GetDevice(), pool, nullptr); }
    void Destroy(VkFence fence) { if(fence != nullptr) vkDestroyFence(Vulkan::GetDevice(), fence, nullptr); }
    void Destroy(VkSemaphore semaphore) { if(semaphore != nullptr) vkDestroySemaphore(Vulkan::GetDevice(), semaphore, nullptr); }
    void Destroy(VkFramebuffer frame_buffer) { if(frame_buffer != nullptr) vkDestroyFramebuffer(Vulkan::GetDevice(), frame_buffer, nullptr); }
    void Destroy(VkDescriptorSetLayout layout) { if(layout != nullptr) vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), layout, nullptr); }
    void Destroy(VkSampler sampler) { if(sampler != nullptr) vkDestroySampler(Vulkan::GetDevice(), sampler, nullptr); }
    void Destroy(VkDescriptorPool pool) { if(pool != nullptr) vkDestroyDescriptorPool(Vulkan::GetDevice(), pool, nullptr); }
    void Destroy(VkPipelineShaderStageCreateInfo stage) { if(stage.module != nullptr) vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr); }
    void Destroy(VkRenderPass render_pass) { if(render_pass != nullptr) vkDestroyRenderPass(Vulkan::GetDevice(), render_pass, nullptr); };
    void Destroy(VkImageView view) { if(view != nullptr) vkDestroyImageView(Vulkan::GetDevice(), view, nullptr); }

    void IMAGE_BUFFER::Clear()
    {
        if(this->view != nullptr) vkDestroyImageView(Vulkan::GetDevice(), this->view, nullptr);
        if(this->handle != nullptr) vkDestroyImage(Vulkan::GetDevice(), this->handle, nullptr);
        if(this->memory != nullptr) vkFreeMemory(Vulkan::GetDevice(), this->memory, nullptr);
        *this = {};
    }

    void DATA_BUFFER::Clear()
    {
        if(this->handle != nullptr) vkDestroyBuffer(Vulkan::GetDevice(), this->handle, nullptr);
        if(this->memory != nullptr) vkFreeMemory(Vulkan::GetDevice(), this->memory, nullptr); // vkUnmapMemory is implicit
        *this = {};
    }

    bool MAPPED_BUFFER::Map()
    {
        return vkMapMemory(Vulkan::GetDevice(), this->memory, 0, this->size, 0, (void**)&this->pointer) == VK_SUCCESS;
    }

    void PIPELINE::Clear() {
        if(this->handle != nullptr) vkDestroyPipeline(Vulkan::GetDevice(), this->handle, nullptr);
        if(this->layout != nullptr) vkDestroyPipelineLayout(Vulkan::GetDevice(), this->layout, nullptr);
        *this = {};
    }

    IMAGE_BUFFER CreateImageBuffer(VkImageUsageFlags usage, VkImageAspectFlags aspect, uint32_t width, uint32_t height, VkFormat format)
    {
        IMAGE_BUFFER image_buffer = {};
        image_buffer.format = format;
        image_buffer.aspect = aspect;

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = NULL;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = image_buffer.format;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = NULL;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.flags = 0;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

        VkResult result = vkCreateImage(Vulkan::GetDevice(), &image_info, NULL, &image_buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateImageBuffer() => vkCreateImage : Failed" << std::endl;
            #endif
            return image_buffer;
        }

        VkMemoryRequirements image_memory_requirements;
        vkGetImageMemoryRequirements(Vulkan::GetDevice(), image_buffer.handle, &image_memory_requirements);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = image_memory_requirements.size;
        
        if(!Vulkan::GetInstance()->MemoryTypeFromProperties(image_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_alloc.memoryTypeIndex)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateImageBuffer() => MemoryType VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : Not found" << std::endl;
            #endif
            return image_buffer;
        }

        result = vkAllocateMemory(Vulkan::GetDevice(), &mem_alloc, nullptr, &image_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateImageBuffer() => vkAllocateMemory : Failed" << std::endl;
            #endif
            return image_buffer;
        }

        result = vkBindImageMemory(Vulkan::GetDevice(), image_buffer.handle, image_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateImageBuffer() => vkBindImageMemory : Failed" << std::endl;
            #endif
            return image_buffer;
        }

        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.pNext = nullptr;
        view_info.image = VK_NULL_HANDLE;
        view_info.format = image_buffer.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = image_buffer.aspect; // VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.flags = 0;
        view_info.image = image_buffer.handle;

        result = vkCreateImageView(Vulkan::GetDevice(), &view_info, nullptr, &image_buffer.view);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateImageBuffer() => vkCreateImageView : Failed" << std::endl;
            #endif
            return image_buffer;
        }

        return image_buffer;
    }

    bool CreateDataBuffer(DATA_BUFFER& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkFlags requirement, std::vector<uint32_t> const& queue_families)
    {
        buffer.size = size;

        VkBufferCreateInfo buffer_create_info;
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = buffer.size;
        buffer_create_info.usage = usage; // VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size());
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.pQueueFamilyIndices = queue_families.data();
        if(queue_families.size() > 1) buffer_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;

        VkResult result = vkCreateBuffer(Vulkan::GetDevice(), &buffer_create_info, nullptr, &buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateDataBuffer() => vkCreateBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(Vulkan::GetDevice(), buffer.handle, &mem_reqs);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;
        mem_alloc.allocationSize = mem_reqs.size;
        
        if(!Vulkan::GetInstance()->MemoryTypeFromProperties(mem_reqs.memoryTypeBits, requirement, mem_alloc.memoryTypeIndex)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateDataBuffer() => requirement : " << requirement << " : Not found" << std::endl;
            #endif
            return false;
        }

        result = vkAllocateMemory(Vulkan::GetDevice(), &mem_alloc, nullptr, &buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateDataBuffer() => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindBufferMemory(Vulkan::GetDevice(), buffer.handle, buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"vk::CreateDataBuffer() => vkBindBufferMemory : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    bool CreateCommandPool(VkCommandPool& pool, uint32_t queue_family_index, VkCommandPoolCreateFlags flags)
    {
        VkCommandPoolCreateInfo cmd_pool_info;
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.queueFamilyIndex = queue_family_index;
        cmd_pool_info.flags = flags;

        return vkCreateCommandPool(Vulkan::GetDevice(), &cmd_pool_info, nullptr, &pool) == VK_SUCCESS;
    }

    bool CreateCommandBuffer(VkCommandPool pool, VkCommandBuffer& command_buffer, VkCommandBufferLevel level)
    {
        VkCommandBufferAllocateInfo cmd;
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = pool;
        cmd.level = level;
        cmd.commandBufferCount = 1;

        return vkAllocateCommandBuffers(Vulkan::GetDevice(), &cmd, &command_buffer) == VK_SUCCESS;
    }

    bool CreateFence(VkFence& fence, bool reset)
    {
        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = nullptr;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if(vkCreateFence(Vulkan::GetDevice(), &fence_create_info, nullptr, &fence) != VK_SUCCESS) return false;
        if(reset) return(vkResetFences(Vulkan::GetDevice(), 1, &fence) == VK_SUCCESS);
        else return true;
    }

    bool CreateSemaphore(VkSemaphore &semaphore)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = nullptr;
        semaphore_create_info.flags = 0;

        return vkCreateSemaphore(Vulkan::GetDevice(), &semaphore_create_info, nullptr, &semaphore) == VK_SUCCESS;
    }

    bool SubmitQueue(std::vector<VkCommandBuffer> command_buffers, std::vector<VkSemaphore> wait_semaphores,
                     std::vector<VkPipelineStageFlags> wait_stages, VkSemaphore signal_semaphore,
                     VkQueue queue, VkFence fence)
    {
        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;

        submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
        submit_info.pCommandBuffers = command_buffers.data();

        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &signal_semaphore;

        submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();

        return vkQueueSubmit(queue, 1, &submit_info, fence) == VK_SUCCESS;
    }

    bool SubmitQueue(std::vector<VkSubmitInfo> infos, VkQueue queue, VkFence fence)
    {
        return vkQueueSubmit(queue, static_cast<uint32_t>(infos.size()), infos.data(), fence) == VK_SUCCESS;
    }

    VkSubmitInfo PrepareQueue(std::vector<VkCommandBuffer> command_buffers, std::vector<VkSemaphore> wait_semaphores,
                     std::vector<VkPipelineStageFlags> wait_stages, VkSemaphore signal_semaphore,
                     VkQueue queue, VkFence fence)
    {
        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;

        submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
        submit_info.pCommandBuffers = command_buffers.data();

        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &signal_semaphore;

        submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();

        return submit_info;
    }

    bool WaitForFence(VkFence fence)
    {
        if(vkWaitForFences(Vulkan::GetDevice(), 1, &fence, VK_FALSE, 1000000000) != VK_SUCCESS) return false;
        vkResetFences(Vulkan::GetDevice(), 1, &fence);
        return true;
    }

    size_t SendImageToBuffer(IMAGE_BUFFER& buffer, Tools::IMAGE_MAP const& image, VkCommandBuffer command_buffer, VkFence fence, MAPPED_BUFFER staging)
    {
        VkResult result = vkWaitForFences(Vulkan::GetDevice(), 1, &fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SendToBuffer[image] => vkWaitForFences : Timeout" << std::endl;
            #endif
            return false;
        }
        vkResetFences(Vulkan::GetDevice(), 1, &fence);

        std::memcpy(staging.pointer, image.data.data(), image.data.size());

        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = staging.memory;
        flush_range.offset = 0;
        flush_range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(Vulkan::GetDevice(), 1, &flush_range);

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

        VkImageMemoryBarrier from_undefined_to_transfer_dst = {};
        from_undefined_to_transfer_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        from_undefined_to_transfer_dst.pNext = nullptr;
        from_undefined_to_transfer_dst.srcAccessMask = 0;
        from_undefined_to_transfer_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        from_undefined_to_transfer_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        from_undefined_to_transfer_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        from_undefined_to_transfer_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        from_undefined_to_transfer_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        from_undefined_to_transfer_dst.image = buffer.handle;
        from_undefined_to_transfer_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        from_undefined_to_transfer_dst.subresourceRange.baseMipLevel = 0;
        from_undefined_to_transfer_dst.subresourceRange.levelCount = 1;
        from_undefined_to_transfer_dst.subresourceRange.baseArrayLayer = 0;
        from_undefined_to_transfer_dst.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(command_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &from_undefined_to_transfer_dst);

        VkBufferImageCopy buffer_image_copy_info = {};
        buffer_image_copy_info.bufferOffset = 0;
        buffer_image_copy_info.bufferRowLength = 0;
        buffer_image_copy_info.bufferImageHeight = 0;
        buffer_image_copy_info.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_image_copy_info.imageSubresource.mipLevel = 0;
        buffer_image_copy_info.imageSubresource.baseArrayLayer = 0;
        buffer_image_copy_info.imageSubresource.layerCount = 1;
        buffer_image_copy_info.imageOffset.x = 0;
        buffer_image_copy_info.imageOffset.y = 0;
        buffer_image_copy_info.imageOffset.z = 0;
        buffer_image_copy_info.imageExtent.width = image.width;
        buffer_image_copy_info.imageExtent.height = image.height;
        buffer_image_copy_info.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(command_buffer, staging.handle, buffer.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy_info);


        /*if(Vulkan::GetGraphicsQueue().index != Vulkan::GetTransferQueue().index) {
            VkImageMemoryBarrier image_memory_barrier;
            image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_memory_barrier.pNext = nullptr;
            image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_memory_barrier.image = buffer.handle;
            image_memory_barrier.subresourceRange.aspectMask = buffer.aspect;
            image_memory_barrier.subresourceRange.baseMipLevel = 0;
            image_memory_barrier.subresourceRange.levelCount = 1;
            image_memory_barrier.subresourceRange.baseArrayLayer = 0;
            image_memory_barrier.subresourceRange.layerCount = 1;
            image_memory_barrier.srcQueueFamilyIndex = Vulkan::GetTransferQueue().index;
            image_memory_barrier.dstQueueFamilyIndex = Vulkan::GetGraphicsQueue().index;

            vkCmdPipelineBarrier(
                command_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1,
                &image_memory_barrier
            );
        }*/

        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(Vulkan::GetTransferQueue().handle, 1, &submit_info, fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SendToBuffer[image] => vkQueueSubmit : Failed" << std::endl;
            #endif
            return false;
        }

        /*if(!this->TransitionImageLayout(buffer,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SendToBuffer[image] => TransitionImageLayout : Failed" << std::endl;
            #endif
            return 0;
        }*/

        return true;
    }

    size_t SendDataToBuffer(DATA_BUFFER& buffer, VkCommandBuffer command_buffer, MAPPED_BUFFER staging_buffer,
                            std::vector<Chunk> chunks, VkQueue queue, VkSemaphore signal, VkSemaphore wait)
    {
        VkMappedMemoryRange flush_range;
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = staging_buffer.memory;
        flush_range.offset = 0;
        flush_range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(Vulkan::GetDevice(), 1, &flush_range);

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

        size_t bytes_sent = 0;
        for(auto& chunk : chunks) {
            VkBufferCopy buffer_copy_info;
            buffer_copy_info.srcOffset = chunk.offset;
            buffer_copy_info.dstOffset = chunk.offset;
            buffer_copy_info.size = chunk.range;
            vkCmdCopyBuffer(command_buffer, staging_buffer.handle, buffer.handle, 1, &buffer_copy_info);
            bytes_sent += chunk.range;
        }

        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        if(signal != nullptr) {
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &signal;
        }else{
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = nullptr;
        }

        if(wait) {
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &wait;
            VkPipelineStageFlags mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            submit_info.pWaitDstStageMask = &mask;
        }else{
            submit_info.waitSemaphoreCount = 0;
            submit_info.pWaitSemaphores = nullptr;
            submit_info.pWaitDstStageMask = nullptr;
        }

        if(!vk::SubmitQueue({submit_info}, queue, nullptr)) return 0;

        return bytes_sent;
    }

    bool CreateComputePipeline(VkPipelineShaderStageCreateInfo stage,
                               std::vector<VkDescriptorSetLayout> descriptor_set_layouts,
                               std::vector<VkPushConstantRange> push_constant_ranges,
                               PIPELINE& pipeline)
    {
        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
        pPipelineLayoutCreateInfo.pPushConstantRanges = push_constant_ranges.data();
        pPipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
        pPipelineLayoutCreateInfo.pSetLayouts = descriptor_set_layouts.data();

        VkResult result = vkCreatePipelineLayout(Vulkan::GetDevice(), &pPipelineLayoutCreateInfo, nullptr, &pipeline.layout);

        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk:CreateComputePipeline() => vkCreatePipelineLayout : Failed" << std::endl;
            #endif
            return false;
        }

        VkComputePipelineCreateInfo pipeline_create_infos;
        pipeline_create_infos.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_create_infos.pNext = nullptr;
        pipeline_create_infos.layout = pipeline.layout;
        pipeline_create_infos.flags = 0;
        pipeline_create_infos.basePipelineHandle = nullptr;
        pipeline_create_infos.basePipelineIndex = 0;
        pipeline_create_infos.stage = stage;

        result = vkCreateComputePipelines(Vulkan::GetDevice(), nullptr, 1, &pipeline_create_infos, nullptr, &pipeline.handle);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk:CreateComputePipeline() => vkCreateComputePipelines : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    bool CreateGraphicsPipeline(bool dynamic_viewport,
                                std::vector<VkDescriptorSetLayout> descriptor_set_layouts,
                                std::vector<VkPipelineShaderStageCreateInfo> shader_stages,
                                std::vector<VkVertexInputBindingDescription> vertex_binding_description,
                                std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions,
                                std::vector<VkPushConstantRange> push_constant_ranges,
                                PIPELINE& pipeline,
                                VkPolygonMode polygon_mode,
                                VkPrimitiveTopology topology,
                                bool blend_state)
    {
        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
        pPipelineLayoutCreateInfo.pPushConstantRanges = push_constant_ranges.data();
        pPipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
        pPipelineLayoutCreateInfo.pSetLayouts = descriptor_set_layouts.data();

        VkResult result = vkCreatePipelineLayout(Vulkan::GetDevice(), &pPipelineLayoutCreateInfo, nullptr, &pipeline.layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateGraphicsPipeline() => vkCreatePipelineLayout : Failed" << std::endl;
            #endif
            return false;
        }
        
        VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = nullptr;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = nullptr;
        viewport_state_create_info.pViewports = nullptr;

        VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState = {};
        VkViewport viewport = {};
        VkRect2D scissor = {};

        if(dynamic_viewport) {
            
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.pNext = nullptr;
            dynamicState.dynamicStateCount = 2;
            dynamicState.pDynamicStates = dynamic_states;
            
        }else{

            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(Vulkan::GetDrawSurface().width);
            viewport.height = static_cast<float>(Vulkan::GetDrawSurface().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            scissor.offset.x = 0;
            scissor.offset.y = 0;
            scissor.extent.width = Vulkan::GetDrawSurface().width;
            scissor.extent.height = Vulkan::GetDrawSurface().height;

            viewport_state_create_info.pScissors = &scissor;
            viewport_state_create_info.pViewports = &viewport;
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state;
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state.pNext = nullptr;
        vertex_input_state.flags = 0;
        vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_description.size());
        vertex_input_state.pVertexBindingDescriptions = vertex_binding_description.data();
        vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size());
        vertex_input_state.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.pNext = nullptr;
        input_assembly_state.flags = 0;
        input_assembly_state.primitiveRestartEnable = VK_FALSE;
        input_assembly_state.topology = topology;

        if(polygon_mode == VK_POLYGON_MODE_LINE) input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterization_state;
        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.pNext = nullptr;
        rasterization_state.flags = 0;
        rasterization_state.polygonMode = polygon_mode;
        rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state.depthClampEnable = VK_FALSE;
        rasterization_state.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state.depthBiasEnable = VK_FALSE;
        rasterization_state.depthBiasConstantFactor = 0.0f;
        rasterization_state.depthBiasClamp = 0.0f;
        rasterization_state.depthBiasSlopeFactor = 0.0f;
        rasterization_state.lineWidth = 1.0f;

        if(polygon_mode == VK_POLYGON_MODE_LINE) {
            rasterization_state.cullMode = VK_CULL_MODE_NONE;
            rasterization_state.lineWidth = 2.0f;
        }

        VkPipelineColorBlendAttachmentState blend_attachment;
        blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        if(blend_state) {
            blend_attachment.blendEnable = VK_TRUE;
		    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        }else{
            blend_attachment.blendEnable = VK_FALSE;
		    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        }

        VkPipelineColorBlendStateCreateInfo color_blend_state;
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.pNext = nullptr;
        color_blend_state.flags = 0;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &blend_attachment;
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.logicOp = VK_LOGIC_OP_COPY;
        color_blend_state.blendConstants[0] = 0.0f;
        color_blend_state.blendConstants[1] = 0.0f;
        color_blend_state.blendConstants[2] = 0.0f;
        color_blend_state.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = nullptr;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.minDepthBounds = 0.0f;
        depth_stencil_state.maxDepthBounds = 1.0f;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.back.failOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.passOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depth_stencil_state.back.compareMask = 0;
        depth_stencil_state.back.reference = 0;
        depth_stencil_state.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.writeMask = 0;
        depth_stencil_state.front = depth_stencil_state.back;

        VkPipelineMultisampleStateCreateInfo multi_samples_state;
        multi_samples_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multi_samples_state.pNext = nullptr;
        multi_samples_state.flags = 0;
        multi_samples_state.pSampleMask = nullptr;
        multi_samples_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multi_samples_state.sampleShadingEnable = VK_FALSE;
        multi_samples_state.alphaToCoverageEnable = VK_FALSE;
        multi_samples_state.alphaToOneEnable = VK_FALSE;
        multi_samples_state.minSampleShading = 1.0f;

        VkGraphicsPipelineCreateInfo pipeline_create_info;
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = nullptr;
        pipeline_create_info.layout = pipeline.layout;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state;
        pipeline_create_info.pRasterizationState = &rasterization_state;
        pipeline_create_info.pColorBlendState = &color_blend_state;
        pipeline_create_info.pTessellationState = nullptr;
        pipeline_create_info.pMultisampleState = &multi_samples_state;
        pipeline_create_info.pDynamicState = (dynamic_viewport) ? &dynamicState : nullptr;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pStages = shader_stages.data();
        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipeline_create_info.renderPass = Vulkan::GetRenderPass();
        pipeline_create_info.subpass = 0;

        result = vkCreateGraphicsPipelines(Vulkan::GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline.handle);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::CreateGraphicsPipeline() => vkCreateGraphicsPipelines : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "vk::CreateGraphicsPipeline() : Success" << std::endl;
        #endif
        return true;
    }

    VkPipelineShaderStageCreateInfo LoadShaderModule(std::string const& filename, VkShaderStageFlagBits type)
    {
        std::vector<char> code = Tools::GetBinaryFileContents(filename);
        if(!code.size()) {
            #if defined(DISPLAY_LOGS)
            std::cout << "vk::LoadShaderModule() => GetBinaryFileContents : Failed" << std::endl;
            #endif
            return {};
        }

        VkShaderModuleCreateInfo shader_module_create_info;
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.pNext = nullptr;
        shader_module_create_info.flags = 0;
        shader_module_create_info.codeSize = code.size();
        shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule module;
		VkResult result = vkCreateShaderModule(Vulkan::GetDevice(), &shader_module_create_info, nullptr, &module);
		if(result != VK_SUCCESS) {
			#if defined(DISPLAY_LOGS)
            std::cout << "vk::LoadShaderModule() => vkCreateShaderModule : Failed" << std::endl;
            #endif
			return {};
		}

        VkPipelineShaderStageCreateInfo shader_stage_create_infos;
        shader_stage_create_infos.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_infos.pNext = nullptr;
        shader_stage_create_infos.flags = 0;
        shader_stage_create_infos.stage = type;
        shader_stage_create_infos.module = module;
        shader_stage_create_infos.pName = "main";
        shader_stage_create_infos.pSpecializationInfo = nullptr;

        return shader_stage_create_infos;
    }

    std::vector<VkVertexInputAttributeDescription> CreateVertexInputDescription(
                std::vector<std::vector<VERTEX_BINDING_ATTRIBUTE>> attributes,
                std::vector<VkVertexInputBindingDescription>& descriptions)
    {
        std::vector<VkVertexInputAttributeDescription> output;
        uint32_t location = 0;
        descriptions.clear();

        for(int32_t i=0; i<attributes.size(); i++) {

            uint32_t offset = 0;
            for(auto type : attributes[i]) {

                VkVertexInputAttributeDescription attribute;
                attribute.offset = offset;
                attribute.location = location;
                attribute.binding = i;
                location++;

                switch(type) {

                    case VERTEX_BINDING_ATTRIBUTE::POSITION :
                    case VERTEX_BINDING_ATTRIBUTE::NORMAL :
                    case VERTEX_BINDING_ATTRIBUTE::COLOR :
                        attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
                        offset += sizeof(Maths::Vector3);
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::POSITION_2D :
                    case VERTEX_BINDING_ATTRIBUTE::UV :
                        attribute.format = VK_FORMAT_R32G32_SFLOAT;
                        offset += sizeof(Maths::Vector2);
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::BONE_WEIGHTS :
                        attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        offset += sizeof(Maths::Vector4);
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::BONE_IDS :
                        attribute.format = VK_FORMAT_R32G32B32A32_SINT;
                        offset += sizeof(int) * 4;
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::MATRIX :
                        attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        for(uint8_t i=0; i<3; i++) {
                            output.push_back(attribute);
                            attribute.offset += sizeof(Maths::Vector4);
                            attribute.location = location + i;
                        }
                        offset += sizeof(Maths::Matrix4x4);
                        location += 3;
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::UINT_ID :
                        attribute.format = VK_FORMAT_R32_UINT;
                        offset += sizeof(uint32_t);
                        break;

                    default :
                        attribute.format = VK_FORMAT_UNDEFINED;
                }

                output.push_back(attribute);
            }

            VkVertexInputBindingDescription description;
            description.binding = i;
            description.stride = offset;
            if(i == 0) description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            else description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
            descriptions.push_back(description);
        }

        return output;
    }

    bool TransitionImageLayout(
            IMAGE_BUFFER& buffer,
            VkAccessFlags source_mask, VkAccessFlags dest_mask,
            VkImageLayout old_layout, VkImageLayout new_layout,
            VkPipelineStageFlags source_stage, VkPipelineStageFlags dest_stage,
            VkCommandBuffer command_buffer, VkFence fence
    ) {
        if(!vk::WaitForFence(fence)) return false;

        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

        VkImageMemoryBarrier image_memory_barrier;
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = source_mask;
        image_memory_barrier.dstAccessMask = dest_mask;
        image_memory_barrier.oldLayout = old_layout;
        image_memory_barrier.newLayout = new_layout;
        image_memory_barrier.image = buffer.handle;
        image_memory_barrier.subresourceRange.aspectMask = buffer.aspect;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        vkCmdPipelineBarrier(
            command_buffer,
            source_stage, dest_stage,
            0, 0, nullptr, 0, nullptr, 1,
            &image_memory_barrier
        );

        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        return vk::SubmitQueue({submit_info}, Vulkan::GetGraphicsQueue().handle, fence);
    }

    bool GetVulkanVersion(std::string &version_string)
    {
        bool success = false;

        ///////////////////////////////
        //// Plateforme MS Windows ////
        ///////////////////////////////

        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        DWORD ver_handle;
        DWORD ver_size = GetFileVersionInfoSize(L"vulkan-1.dll", &ver_handle);
        if (ver_size > 0) {
            std::vector<char> ver_data(ver_size);
            if (GetFileVersionInfo(L"vulkan-1.dll", ver_handle, ver_size, ver_data.data())) {
                UINT size = 0;
                LPBYTE buffer = NULL;
                if (VerQueryValue(ver_data.data(), L"\\", (VOID FAR * FAR *)&buffer, &size)) {
                    if(size) {
                        VS_FIXEDFILEINFO *ver_info = (VS_FIXEDFILEINFO *)buffer;
                        if (ver_info->dwSignature == 0xfeef04bd) {
                            version_string = std::to_string((ver_info->dwFileVersionMS >> 16) & 0xffff);
                            version_string += ".";
                            version_string += std::to_string((ver_info->dwFileVersionMS >> 0) & 0xffff);
                            version_string += ".";
                            version_string += std::to_string((ver_info->dwFileVersionLS >> 16) & 0xffff);
                            success = true;
                        }
                    }
                }
            }
            ver_data.clear();
        }
        #endif

        return success;
    }
}}