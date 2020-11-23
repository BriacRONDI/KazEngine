#include "MappedBuffer.h"

namespace Engine
{
    MappedBuffer::MappedBuffer(size_t size, VkBufferUsageFlags usage)
    {
        if(!vk::CreateDataBuffer(this->buffer, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) return;

        if(!this->buffer.Map()) {
            vk::Destroy(this->buffer);
            return;
        }

        std::memset(this->buffer.pointer, '\0', size);
        this->chunk = std::shared_ptr<Chunk>(new Chunk(0, size));
    }

    void MappedBuffer::Clear()
    {
        this->chunk.reset();
        vk::Destroy(this->buffer);
    }

    void MappedBuffer::Flush()
    {
        VkMappedMemoryRange flush_range;
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->buffer.memory;
        flush_range.offset = 0;
        flush_range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(Vulkan::GetDevice(), 1, &flush_range);
    }
}