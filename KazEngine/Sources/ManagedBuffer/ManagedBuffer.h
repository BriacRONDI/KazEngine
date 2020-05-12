#pragma once

#include <unordered_map>
#include <set>
#include "../Vulkan/Vulkan.h"

#define SIZE_MEGABYTE(mb) 1024 * 1024 * mb
#define SIZE_KILOBYTE(kb) 1024 * kb

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
            inline VkDescriptorBufferInfo GetBufferInfos(Vulkan::DATA_CHUNK const& chunk, uint8_t instance_id = 0) const { return {this->buffers[instance_id].handle, chunk.offset, chunk.range}; }
            inline VkDescriptorBufferInfo GetBufferInfos(uint8_t instance_id = 0) const { return {this->buffers[instance_id].handle, 0, this->buffers[instance_id].size}; }
            inline void WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset) { for(uint8_t i=0; i<this->instance_count; i++) this->WriteData(data, data_size, global_offset, i); }
            void WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset, uint8_t instance_id);
            bool Flush(Vulkan::COMMAND_BUFFER const& command_buffer);
            bool Flush(Vulkan::COMMAND_BUFFER const& command_buffer, uint8_t instance_id);
            Vulkan::DATA_CHUNK ReserveChunk(size_t size);
            Vulkan::DATA_CHUNK ReserveChunk(size_t size, VkDeviceSize alignment);
            void FreeChunk(Vulkan::DATA_CHUNK freed);
            inline uint8_t GetInstanceCount() const { return this->instance_count; }

        private :

            struct FLUSH_RANGE {
                VkDeviceSize start;
                VkDeviceSize end;
            };
            
            uint8_t instance_count;
            std::vector<Vulkan::STAGING_BUFFER> staging_buffers;
            std::vector<Vulkan::DATA_BUFFER> buffers;
            std::vector<Vulkan::DATA_CHUNK> free_chunks;
            uint32_t next_free_chunk_id;
            std::vector<std::vector<Vulkan::DATA_CHUNK>> flush_chunks;

            inline void UpdateFlushRange(size_t start_offset, size_t end_offset, uint8_t instance_id);
            static bool AllocateStagingBuffer(Vulkan::STAGING_BUFFER& staging_buffer, std::vector<uint32_t> queue_families, VkDeviceSize size);
    };
}