#pragma once

#include "../Vulkan/Vulkan.h"
#include "../Chunk/Chunk.h"

namespace Engine
{
    class InstancedBuffer
    {
        public :

            InstancedBuffer() { this->Clear(); }
            InstancedBuffer(size_t size, VkBufferUsageFlags usage, uint8_t instance_count, std::vector<uint32_t> const& queue_families = {});
            InstancedBuffer& operator=(InstancedBuffer const& other);
            InstancedBuffer& operator=(InstancedBuffer&& other);
            void Clear();
            const std::shared_ptr<Chunk> GetChunk() const { return this->chunk; }
            uint8_t GetInstanceCount() const { return this->instance_count; }
            VkDescriptorBufferInfo GetBufferInfos(std::shared_ptr<Chunk> chunk, uint8_t instance_id = 0) const { return {this->buffers[instance_id].handle, chunk->offset, chunk->range}; }
            VkDescriptorBufferInfo GetBufferInfos(uint8_t instance_id = 0) const { return {this->buffers[instance_id].handle, 0, this->buffers[instance_id].size}; }
            void WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset) { for(uint8_t i=0; i<this->instance_count; i++) this->WriteData(data, data_size, global_offset, i); }
            void WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset, uint8_t instance_id);
            vk::MAPPED_BUFFER const& GetBuffer(uint8_t instance_id = 0) const { return this->buffers[instance_id]; }
            void Flush(uint8_t instance_id = 0);
            void MoveData(size_t source_offset, size_t dest_offset, size_t size) { for(uint8_t i=0; i<this->buffers.size(); i++) std::memcpy(this->buffers[i].pointer + dest_offset, this->buffers[i].pointer + source_offset, size); }
            
        private :

            // std::vector<vk::DATA_BUFFER> buffers;
            // vk::DATA_BUFFER staging_buffer;
            // char* staging_pointer;
            std::vector<vk::MAPPED_BUFFER> buffers;
            std::shared_ptr<Chunk> chunk;
            uint8_t instance_count;
            std::vector<std::vector<Chunk>> write_chunks;

            // inline void UpdateFlushRange(size_t start_offset, size_t data_size, uint8_t instance_id);
    };
}
