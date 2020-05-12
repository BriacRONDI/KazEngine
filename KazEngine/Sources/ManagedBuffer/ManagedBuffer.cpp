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

        this->free_chunks.clear();
        this->staging_buffers.clear();
        this->buffers.clear();
        this->flush_chunks.clear();

        this->next_free_chunk_id    = 0;
        this->instance_count        = 0;
    }

    bool ManagedBuffer::Allocate(VkDeviceSize size, VkBufferUsageFlags usage, VkFlags requirement, uint8_t instance_count,
                                 bool allocate_staging_buffer, std::vector<uint32_t> const& queue_families)
    {
        this->Clear();

        this->instance_count = instance_count;
        this->free_chunks.push_back({0, size});

        this->staging_buffers.resize(instance_count);
        this->buffers.resize(instance_count);
        this->flush_chunks.resize(instance_count);

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
        std::vector<Vulkan::DATA_CHUNK>& chunks = this->flush_chunks[instance_id];

        auto contiguous_before = chunks.end();
        auto contiguous_after = chunks.end();

        for(auto chunk_it = chunks.begin(); chunk_it != chunks.end(); chunk_it++) {

            if(chunk_it->offset + chunk_it->range == start_offset) contiguous_before = chunk_it;
            if(chunk_it->offset == start_offset + data_size) contiguous_after = chunk_it;
            if(contiguous_before != chunks.end() && contiguous_after != chunks.end()) break;
        }

        if(contiguous_before != chunks.end() && contiguous_after != chunks.end()) {
            contiguous_before->range += data_size + contiguous_after->range;
            chunks.erase(contiguous_after);
        } else if(contiguous_before != chunks.end()) {
            contiguous_before->range += data_size;
        } else if(contiguous_after != chunks.end()) {
            contiguous_after->range += data_size;
            contiguous_after->offset -= data_size;
        } else {
            chunks.push_back({start_offset, data_size});
        }
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
        if(!this->flush_chunks[instance_id].empty()) {
            size_t bytes_sent = Vulkan::GetInstance().SendToBuffer(this->buffers[instance_id], command_buffer, this->staging_buffers[instance_id],
                                                                   this->flush_chunks[instance_id]);
            if(!bytes_sent) return false;
            this->flush_chunks[instance_id].clear();
        }
        
        return true;
    }

    Vulkan::DATA_CHUNK ManagedBuffer::ReserveChunk(size_t size)
    {
        if(!this->free_chunks.size()) return {};

        VkDeviceSize claimed_range = size;
        // if(this->chunck_alignment > 0) claimed_range = static_cast<uint32_t>((size + this->chunck_alignment - 1) & ~(this->chunck_alignment - 1));

        for(auto iter = this->free_chunks.begin(); iter != this->free_chunks.end(); iter++) {
            Vulkan::DATA_CHUNK& chunk = *iter;
            if(chunk.range > claimed_range) {
                Vulkan::DATA_CHUNK result = {chunk.offset, size};
                chunk.offset += static_cast<uint32_t>(claimed_range);
                chunk.range -= claimed_range;
                return result;
            }else if(chunk.range == claimed_range) {
                Vulkan::DATA_CHUNK result = {chunk.offset, size};
                this->free_chunks.erase(iter);
                return result;
            }
        }

        return {};
    }

    Vulkan::DATA_CHUNK ManagedBuffer::ReserveChunk(size_t size, VkDeviceSize alignment)
    {
        if(!this->free_chunks.size()) return {};

        VkDeviceSize claimed_range = (size + alignment - 1) & ~(alignment - 1);

        for(auto iter = this->free_chunks.begin(); iter != this->free_chunks.end(); iter++) {
            Vulkan::DATA_CHUNK& chunk = *iter;

            VkDeviceSize aligned_offset = (chunk.offset + alignment - 1) & ~(alignment - 1);
            if(aligned_offset == chunk.offset) {
                if(chunk.range > claimed_range) {
                    Vulkan::DATA_CHUNK result = {chunk.offset, size};
                    chunk.offset += static_cast<uint32_t>(claimed_range);
                    chunk.range -= claimed_range;
                    return result;
                }else if(chunk.range == claimed_range) {
                    Vulkan::DATA_CHUNK result = {chunk.offset, size};
                    this->free_chunks.erase(iter);
                    return result;
                }
            }else{
                VkDeviceSize new_range = aligned_offset - chunk.offset;
                VkDeviceSize available_range = chunk.range - new_range;
                if(available_range >= claimed_range) {
                    
                    Vulkan::DATA_CHUNK result = {aligned_offset, claimed_range};
                    chunk.range = new_range;
                    if(available_range != claimed_range) this->free_chunks.push_back({aligned_offset + claimed_range, available_range - claimed_range});
                    return result;
                }
            }
        }

        return {};
    }

    void ManagedBuffer::FreeChunk(Vulkan::DATA_CHUNK freed)
    {
        if(!freed.range) return;
        auto contiguous_before = this->free_chunks.end();
        auto contiguous_after = this->free_chunks.end();

        for(auto chunk_it = this->free_chunks.begin(); chunk_it != this->free_chunks.end(); chunk_it++) {

            if(chunk_it->offset + chunk_it->range == freed.offset) contiguous_before = chunk_it;
            if(chunk_it->offset == freed.offset + freed.range) contiguous_after = chunk_it;
            if(contiguous_before != this->free_chunks.end() && contiguous_after != this->free_chunks.end()) break;
        }

        if(contiguous_before != this->free_chunks.end() && contiguous_after != this->free_chunks.end()) {
            contiguous_before->range += freed.range + contiguous_after->range;
            this->free_chunks.erase(contiguous_after);
        } else if(contiguous_before != this->free_chunks.end()) {
            contiguous_before->range += freed.range;
        } else if(contiguous_after != this->free_chunks.end()) {
            contiguous_after->range += freed.range;
            contiguous_after->offset -= freed.range;
        } else {
            this->free_chunks.push_back(freed);
        }
    }
}