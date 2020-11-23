#include "InstancedBuffer.h"

namespace Engine
{
    void InstancedBuffer::Clear()
    {
        for(auto& buffer : this->buffers) vk::Destroy(buffer);
        // vk::Destroy(this->staging_buffer);

        this->instance_count = 0;
        this->buffers.clear();
        this->write_chunks.clear();
        // this->staging_pointer = nullptr;
        this->chunk.reset();
    }

    InstancedBuffer::InstancedBuffer(size_t size, VkBufferUsageFlags usage, uint8_t instance_count, std::vector<uint32_t> const& queue_families)
    {
        this->instance_count = instance_count;

        /*if(!vk::CreateDataBuffer(this->staging_buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, queue_families)) {
            this->Clear();
            return;
        }

        if(vkMapMemory(Vulkan::GetDevice(), this->staging_buffer.memory, 0, size, 0, (void**)&this->staging_pointer) != VK_SUCCESS) {
            this->Clear();
            return;
        }*/

        this->buffers.resize(instance_count);
        for(uint8_t i=0; i<instance_count; i++) {
            // if(!vk::CreateDataBuffer(this->buffers[i], size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, queue_families)) {
            if(!vk::CreateDataBuffer(this->buffers[i], size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, queue_families)) {
                this->Clear();
                return;
            }

            if(!this->buffers[i].Map()) {
                this->Clear();
                return;
            }

            std::memset(this->buffers[i].pointer, '\0', this->buffers[i].size);
        }

        // std::memset(this->staging_pointer, '\0', this->staging_buffer.size);
        this->write_chunks.resize(instance_count);
        this->chunk = std::shared_ptr<Chunk>(new Chunk(0, size));
    }

    InstancedBuffer& InstancedBuffer::operator=(InstancedBuffer const& other)
    {
        if(&other != this) {
            this->buffers = other.buffers;
            // this->staging_buffer = other.staging_buffer;
            this->chunk = other.chunk;
            this->write_chunks = other.write_chunks;
            this->instance_count = other.instance_count;
            // this->staging_pointer = other.staging_pointer;
        }

        return *this;
    }

    InstancedBuffer& InstancedBuffer::operator=(InstancedBuffer&& other)
    {
        if(&other != this) {
            this->buffers = std::move(other.buffers);
            // this->staging_buffer = std::move(other.staging_buffer);
            this->chunk = std::move(other.chunk);
            this->write_chunks = std::move(other.write_chunks);
            this->instance_count = other.instance_count;
            // this->staging_pointer = other.staging_pointer;

            other.instance_count = 0;
            // other.staging_pointer = nullptr;
        }

        return *this;
    }

    void InstancedBuffer::WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset, uint8_t instance_id)
    {
        // std::memcpy(this->staging_pointer + global_offset, data, data_size);
        std::memcpy(this->buffers[instance_id].pointer + global_offset, data, data_size);
        // this->UpdateFlushRange(global_offset, data_size, static_cast<uint8_t>(instance_id));
    }

    /*void InstancedBuffer::UpdateFlushRange(size_t start_offset, size_t data_size, uint8_t instance_id)
    {
        if(!data_size) return;

        std::vector<Chunk>& chunks = this->write_chunks[instance_id];
        size_t end_offset = start_offset + data_size;
        
        uint32_t overlap_start = 0;
        uint32_t overlap_count = 0;
        uint32_t insert_pos = 0;
        for(uint32_t i=0; i<chunks.size(); i++) {
            
            size_t flush_chunk_end = chunks[i].offset + chunks[i].range;

            if(end_offset < chunks[i].offset) {
                insert_pos = i;
                break;
            }else if(flush_chunk_end >= start_offset) {
                if(!overlap_count) overlap_start = i;
                overlap_count++;
                if(chunks[i].offset < start_offset) start_offset = chunks[i].offset;
                if(flush_chunk_end > end_offset) end_offset = flush_chunk_end;
            }else{
                insert_pos++;
            }
        }

        if(!overlap_count) chunks.insert(chunks.begin() + insert_pos, {start_offset, end_offset - start_offset});
        else {
            chunks[overlap_start].offset = start_offset;
            chunks[overlap_start].range = end_offset - start_offset;
            if(overlap_count == 2) chunks.erase(chunks.begin() + overlap_start + 1);
            else if(overlap_count > 2) chunks.erase(chunks.begin() + overlap_start + 1, chunks.begin() + overlap_start + overlap_count - 1);
        }
    }*/

    void InstancedBuffer::Flush(uint8_t instance_id)
    {
        VkMappedMemoryRange flush_range;
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->buffers[instance_id].memory;
        flush_range.offset = 0;
        flush_range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(Vulkan::GetDevice(), 1, &flush_range);
    }
}