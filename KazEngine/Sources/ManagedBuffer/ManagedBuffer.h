#pragma once

#include <mutex>
#include <unordered_map>
#include <set>
#include "../Vulkan/Vulkan.h"
#include "./Chunk/Chunk.h"

namespace Engine
{
    class ManagedBuffer
    {
        public :

            bool Allocate(VkDeviceSize size, VkBufferUsageFlags usage, VkFlags requirement = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          uint8_t instance_count = 1, bool allocate_staging_buffer = true, std::vector<uint32_t> const& queue_families = {});
            inline ~ManagedBuffer() { this->Clear(); }
            void Clear();
            inline Vulkan::DATA_BUFFER const& GetBuffer(uint8_t instance_id = 0) const { return this->buffers[instance_id]; }
            inline Vulkan::DATA_BUFFER& GetBuffer(uint8_t instance_id = 0) { return this->buffers[instance_id]; }
            inline VkDescriptorBufferInfo GetBufferInfos(std::shared_ptr<Chunk> chunk, uint8_t instance_id = 0) const { return {this->buffers[instance_id].handle, chunk->offset, chunk->range}; }
            inline VkDescriptorBufferInfo GetBufferInfos(uint8_t instance_id = 0) const { return {this->buffers[instance_id].handle, 0, this->buffers[instance_id].size}; }
            inline void WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset) { for(uint8_t i=0; i<this->instance_count; i++) this->WriteData(data, data_size, global_offset, i); }
            inline void MoveData(VkDeviceSize source_offset, VkDeviceSize dest_offset, VkDeviceSize size) { for(uint8_t i=0; i<this->instance_count; i++) this->MoveData(source_offset, dest_offset, size, i); }
            inline void MoveData(VkDeviceSize source_offset, VkDeviceSize dest_offset, VkDeviceSize size, uint8_t instance_id)  { this->WriteData(this->staging_buffers[instance_id].pointer + source_offset, size, dest_offset, instance_id); }
            void WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset, uint8_t instance_id);
            bool Flush(Vulkan::COMMAND_BUFFER const& command_buffer);
            bool Flush(Vulkan::COMMAND_BUFFER const& command_buffer, uint8_t instance_id);
            inline std::shared_ptr<Chunk> ReserveChunk(size_t size) { return this->chunk.ReserveRange(size); }
            inline std::shared_ptr<Chunk> ReserveChunk(size_t size, VkDeviceSize alignment) { return this->chunk.ReserveRange(size, alignment); }
            inline void FreeChunk(std::shared_ptr<Chunk> chunk) { this->chunk.FreeChild(chunk); }
            inline uint8_t GetInstanceCount() const { return this->instance_count; }
            bool ResizeChunk(std::shared_ptr<Chunk> chunk, size_t size, bool& relocated, VkDeviceSize alignment = 0);
            inline void ReadStagingBuffer(std::vector<char>& output, VkDeviceSize offset, uint8_t instance_id) { std::memcpy(output.data(), this->staging_buffers[instance_id].pointer + offset, output.size()); }
            inline Vulkan::STAGING_BUFFER& GetStagingBuffer(uint8_t instance_id) { return this->staging_buffers[instance_id]; };
            inline void UpdateFlushRange(size_t start_offset, size_t data_size, uint8_t instance_id);

        private :

            struct FLUSH_RANGE {
                VkDeviceSize start;
                VkDeviceSize end;
            };
            
            uint8_t instance_count;
            std::vector<Vulkan::STAGING_BUFFER> staging_buffers;
            std::vector<Vulkan::DATA_BUFFER> buffers;
            Chunk chunk;
            std::vector<std::vector<Chunk>> flush_chunks;
            std::vector<std::unique_ptr<std::mutex>> mutex;
            
            static bool AllocateStagingBuffer(Vulkan::STAGING_BUFFER& staging_buffer, std::vector<uint32_t> queue_families, VkDeviceSize size);
    };
}