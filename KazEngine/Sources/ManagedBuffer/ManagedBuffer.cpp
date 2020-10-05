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
        this->write_chunks.clear();
        this->read_chunks.clear();

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
        this->write_chunks.resize(instance_count);
        this->read_chunks.resize(instance_count);

        for(uint8_t i=0; i<instance_count; i++) {
            if(allocate_staging_buffer && !ManagedBuffer::AllocateStagingBuffer(this->staging_buffers[i], queue_families, size)) {
                this->Clear();
                return false;
            }

            if(!Vulkan::GetInstance().CreateDataBuffer(this->buffers[i], size, usage, requirement, queue_families)) {
                this->Clear();
                return false;
            }
        }

        return true;
    }

    bool ManagedBuffer::AllocateStagingBuffer(Vulkan::STAGING_BUFFER& staging_buffer, std::vector<uint32_t> queue_families, VkDeviceSize size)
    {
        if(!Vulkan::GetInstance().CreateDataBuffer(staging_buffer, size,
                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
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
    }

    void ManagedBuffer::ReadData(std::shared_ptr<Chunk> chunk, uint8_t instance_id)
    {
        /*std::shared_ptr<Chunk> rchunk = std::shared_ptr<Chunk>(new Chunk(chunk->offset, chunk->range));
        std::vector<Chunk>& chunks = this->write_chunks[instance_id];
        size_t rchunk_end = chunk->offset + chunk->range;
        for(uint32_t i=0; i<chunks.size(); i++) {
            size_t wchunk_end = chunks[i].offset + chunks[i].range;
            if(chunks[i].offset <= rchunk->offset && wchunk_end >= rchunk_end) return;
            else if(chunks[i].offset >= rchunk_end) break;
            else if(chunks[i].offset <= rchunk->offset && wchunk_end >= rchunk->offset) {
                rchunk->offset = wchunk_end;
                rchunk->range = rchunk_end - wchunk_end;
            }else if(chunks[i].offset >= rchunk->offset && wchunk_end <= rchunk_end) {
                rchunk->range = chunks[i].offset - rchunk->offset;
                this->read_chunks[instance_id].push_back(rchunk);
                rchunk = std::shared_ptr<Chunk>(new Chunk(wchunk_end, rchunk_end - wchunk_end));
            }else if(chunks[i].offset <= rchunk->offset && wchunk_end >= rchunk_end) {
                rchunk->range = chunks[i].offset - rchunk->offset;
                break;
            }
        }

        if(rchunk->range > 0) this->read_chunks[instance_id].push_back(rchunk);*/
        this->read_chunks[instance_id].push_back({chunk->offset, chunk->range});
    }

    void ManagedBuffer::WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset, uint8_t instance_id)
    {
        size_t plane_start = 3328;
        size_t plane_end = 3328 + 128;
        size_t write_end = global_offset + data_size;
        if(global_offset >= plane_start && global_offset <= plane_end
            || write_end >= plane_start && write_end <= plane_end
            || global_offset <= plane_start && global_offset >= plane_end)
                int a = 0;
        std::memcpy(this->staging_buffers[instance_id].pointer + global_offset, data, data_size);
        this->UpdateFlushRange(global_offset, data_size, static_cast<uint8_t>(instance_id));
    }

    bool ManagedBuffer::FlushWrite(Vulkan::COMMAND_BUFFER const& command_buffer)
    {
        for(uint8_t i=0; i<this->instance_count; i++)
            if(!this->FlushWrite(command_buffer, i)) return false;

        return true;
    }

    bool ManagedBuffer::FlushWrite(Vulkan::COMMAND_BUFFER const& command_buffer, uint8_t instance_id, VkSemaphore signal, VkSemaphore wait)
    {
        if(!this->write_chunks[instance_id].empty()) {
            size_t bytes_sent = Vulkan::GetInstance().SynchronizeBuffer(
                this->buffers[instance_id], command_buffer, this->staging_buffers[instance_id],
                this->write_chunks[instance_id], Vulkan::GetGraphicsQueue().handle,
                signal, wait, true
            );

            if(!bytes_sent) return false;
            this->write_chunks[instance_id].clear();
        }else{
            return false;
        }
        
        return true;
    }

    bool ManagedBuffer::FlushRead(Vulkan::COMMAND_BUFFER const& command_buffer, uint8_t instance_id, VkSemaphore signal, VkSemaphore wait)
    {
        if(!this->read_chunks[instance_id].empty()) {
            size_t bytes_sent = Vulkan::GetInstance().SynchronizeBuffer(
                this->buffers[instance_id], command_buffer, this->staging_buffers[instance_id],
                this->read_chunks[instance_id], Vulkan::GetGraphicsQueue().handle,
                signal, wait, false
            );

            if(!bytes_sent) return false;
            this->read_chunks[instance_id].clear();
        }else{
            return false;
        }
        
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