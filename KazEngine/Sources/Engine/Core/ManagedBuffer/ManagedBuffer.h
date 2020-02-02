#pragma once

#include <unordered_map>
#include <set>
#include "../../Graphics/Vulkan/Vulkan.h"

namespace Engine
{
    class ManagedBuffer
    {
        public :

            inline ~ManagedBuffer() { this->Reset(); }
            void Reset();
            inline Vulkan::DATA_BUFFER& GetBuffer() { return this->buffer; }
            void SetBuffer(Vulkan::DATA_BUFFER& buffer);
            inline VkDescriptorBufferInfo CreateSubBuffer(uint8_t id, VkDeviceSize offset, VkDeviceSize range) { this->sub_buffer[id] = {offset, range}; return this->GetSubBuffer(id); }
            void WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset);
            void WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize relative_offset, uint8_t sub_buffer_id);
            bool Flush();
            inline VkDescriptorBufferInfo GetSubBuffer(uint8_t id) { return {this->buffer.handle, this->sub_buffer[id].offset, this->sub_buffer[id].range}; }
            inline void SetChunkAlignment(VkDeviceSize alignment) { this->chunck_alignment = alignment; }
            bool ReserveChunk(VkDeviceSize& offset, size_t size);
            bool ReserveChunk(uint32_t& offset, size_t size, uint8_t sub_buffer_id);

            std::unique_ptr<char> data;

        private :

            struct SUB_BUFFER {
                VkDeviceSize offset;
                VkDeviceSize range;
            };

            struct CHUNK {
                VkDeviceSize offset;
                VkDeviceSize range;
            };
            
            Vulkan::DATA_BUFFER buffer;
            // std::unique_ptr<char> data;
            std::unordered_map<uint8_t, SUB_BUFFER> sub_buffer;
            bool need_flush;
            VkDeviceSize flush_range_start;
            VkDeviceSize flush_range_end;
            std::vector<CHUNK> free_chunks;
            VkDeviceSize chunck_alignment;

            inline void UpdateFlushRange(size_t start_offset, size_t end_offset);
    };
}