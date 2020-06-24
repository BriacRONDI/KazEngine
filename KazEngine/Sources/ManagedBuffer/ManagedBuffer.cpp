#include "ManagedBuffer.h"

namespace Engine
{
    void ManagedBuffer::Clear()
    {
        if(Vulkan::HasInstance()) {
            for(uint8_t i=0; i<this->instance_count; i++) {
                this->buffers[i].Clear();
                this->staging_buffers[i].Clear();
            }
        }

        this->staging_buffers.clear();
        this->buffers.clear();
        this->flush_chunks.clear();
        this->mutex.clear();

        this->instance_count = 0;
    }

    bool ManagedBuffer::Allocate(VkDeviceSize size, VkBufferUsageFlags usage, VkFlags requirement, uint8_t instance_count,
                                 bool allocate_staging_buffer, std::vector<uint32_t> const& queue_families)
    {
        this->Clear();

        this->instance_count = instance_count;
        this->chunk = {0, size};

        this->staging_buffers.resize(instance_count);
        this->buffers.resize(instance_count);
        this->flush_chunks.resize(instance_count);
        this->mutex.resize(instance_count);

        for(uint8_t i=0; i<instance_count; i++) {
            if(allocate_staging_buffer && !ManagedBuffer::AllocateStagingBuffer(this->staging_buffers[i], queue_families, size)) {
                this->Clear();
                return false;
            }

            if(!Vulkan::GetInstance().CreateDataBuffer(this->buffers[i], size, usage, requirement, queue_families)) {
                this->Clear();
                return false;
            }

            this->mutex[i] = std::unique_ptr<std::mutex>(new std::mutex);
        }

        return true;
    }

    bool ManagedBuffer::AllocateStagingBuffer(Vulkan::STAGING_BUFFER& staging_buffer, std::vector<uint32_t> queue_families, VkDeviceSize size)
    {
        if(!Vulkan::GetInstance().CreateDataBuffer(staging_buffer, size,
                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                   queue_families)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "ManagedBuffer::AllocateStagingBuffer => CreateDataBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        VkResult result = vkMapMemory(Vulkan::GetDevice(), staging_buffer.memory, 0, size, 0, (void**)&staging_buffer.pointer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "ManagedBuffer::AllocateStagingBuffer => vkMapMemory : Failed" << std::endl;
            #endif
            return false;
        }

        std::memset(staging_buffer.pointer, '\0', staging_buffer.size);

        return true;
    }

    inline void ManagedBuffer::UpdateFlushRange(size_t start_offset, size_t data_size, uint8_t instance_id)
    {
        if(!data_size) return;

        std::unique_lock<std::mutex> lock(*this->mutex[instance_id]);
        std::vector<Chunk>& flush = this->flush_chunks[instance_id];
        size_t end_offset = start_offset + data_size;
        
        uint32_t overlap_start = 0;
        uint32_t overlap_count = 0;
        uint32_t insert_pos = 0;
        for(uint32_t i=0; i<flush.size(); i++) {
            
            size_t flush_chunk_end = flush[i].offset + flush[i].range;

            if(end_offset < flush[i].offset) {
                insert_pos = i;
                break;
            }else if(flush_chunk_end >= start_offset) {
                if(!overlap_count) overlap_start = i;
                overlap_count++;
                if(flush[i].offset < start_offset) start_offset = flush[i].offset;
                if(flush_chunk_end > end_offset) end_offset = flush_chunk_end;
            }else{
                insert_pos++;
            }
        }

        if(!overlap_count) flush.insert(flush.begin() + insert_pos, {start_offset, end_offset - start_offset});
        else {
            flush[overlap_start].offset = start_offset;
            flush[overlap_start].range = end_offset - start_offset;
            if(overlap_count == 2) flush.erase(flush.begin() + overlap_start + 1);
            else if(overlap_count > 2) flush.erase(flush.begin() + overlap_start + 1, flush.begin() + overlap_start + overlap_count - 1);
        }
        lock.unlock();
    }

    void ManagedBuffer::WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset, uint8_t instance_id)
    {
        std::memcpy(this->staging_buffers[instance_id].pointer + global_offset, data, data_size);
        this->UpdateFlushRange(global_offset, data_size, static_cast<uint8_t>(instance_id));
    }

    bool ManagedBuffer::Flush(Vulkan::COMMAND_BUFFER const& command_buffer)
    {
        for(uint8_t i=0; i<this->instance_count; i++)
            if(!this->Flush(command_buffer, i)) return false;

        return true;
    }

    bool ManagedBuffer::Flush(Vulkan::COMMAND_BUFFER const& command_buffer, uint8_t instance_id)
    {
        std::unique_lock<std::mutex> lock(*this->mutex[instance_id]);
        if(!this->flush_chunks[instance_id].empty()) {
            size_t bytes_sent = Vulkan::GetInstance().SendToBuffer(this->buffers[instance_id], command_buffer, this->staging_buffers[instance_id],
                                                                   this->flush_chunks[instance_id], Vulkan::GetGraphicsQueue().handle);
            if(!bytes_sent) {
                lock.unlock();
                return false;
            }
            this->flush_chunks[instance_id].clear();
        }
        lock.unlock();
        
        return true;
    }

    bool ManagedBuffer::ResizeChunk(std::shared_ptr<Chunk> chunk, size_t size, bool& relocated, VkDeviceSize alignment)
    {
        size_t mem_offset = chunk->offset;
        size_t mem_range = chunk->range;
        bool resized = this->chunk.ResizeChild(chunk, size, relocated, alignment);
        if(!resized) return false;

        if(relocated)
            for(uint8_t i=0; i<this->instance_count; i++)
                this->WriteData(this->staging_buffers[i].pointer + mem_offset, mem_range, chunk->offset, i);

        return true;
    }
}