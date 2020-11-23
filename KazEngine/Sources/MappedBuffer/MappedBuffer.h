#pragma once

#include "../Vulkan/Vulkan.h"
#include "../Chunk/Chunk.h"

#define MULTI_USAGE_BUFFER_MASK VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT

namespace Engine
{
    class MappedBuffer
    {
        public :

            inline MappedBuffer() { this->Clear(); }
            MappedBuffer(size_t size, VkBufferUsageFlags usage);
            void Clear();
            inline void* Data() const { return this->buffer.pointer; }
            inline void* Data(size_t offset) const { return this->buffer.pointer + offset; }
            inline const std::shared_ptr<Chunk> GetChunk() const { return this->chunk; }
            inline VkDescriptorBufferInfo GetBufferInfos(std::shared_ptr<Chunk> chunk) const { return {this->buffer.handle, chunk->offset, chunk->range}; }
            inline void WriteData(const void* data, size_t size) { std::memcpy(this->buffer.pointer, data, size); }
            inline void WriteData(const void* data, size_t size, size_t offset) { std::memcpy(this->buffer.pointer + offset, data, size); }
            inline vk::MAPPED_BUFFER const& GetBuffer() const { return this->buffer; }
            void MoveData(size_t source_offset, size_t dest_offset, size_t size) { std::memcpy(this->buffer.pointer + dest_offset, this->buffer.pointer + source_offset, size); }
            void Flush();

        private :

            std::shared_ptr<Chunk> chunk;
            vk::MAPPED_BUFFER buffer;
    };
}