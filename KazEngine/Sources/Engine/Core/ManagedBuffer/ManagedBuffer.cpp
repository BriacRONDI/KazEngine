#include "ManagedBuffer.h"

namespace Engine
{
    void ManagedBuffer::Clear()
    {
        this->data.reset();
        this->sub_buffer.clear();
        this->free_chunks.clear();
        this->buffer = {};
        this->need_flush = false;
        this->flush_range_start = 0;
        this->flush_range_end = 0;
        this->chunck_alignment = 0;
    }

    void ManagedBuffer::SetBuffer(Vulkan::DATA_BUFFER& buffer)
    {
        this->buffer = buffer;
        this->data.reset();
        this->data = std::unique_ptr<char>(new char[buffer.size]);
        this->free_chunks.push_back({0, buffer.size});
    }

    inline void ManagedBuffer::UpdateFlushRange(size_t start_offset, size_t data_size)
    {
        size_t end_offset = start_offset + data_size;
        this->need_flush = true;
        if(start_offset < this->flush_range_start) this->flush_range_start = start_offset;
        if(end_offset > this->flush_range_end) this->flush_range_end = end_offset;
    }

    void ManagedBuffer::WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize global_offset)
    {
        std::memcpy(this->data.get() + global_offset, data, data_size);
        this->UpdateFlushRange(global_offset, data_size);
    }

    void ManagedBuffer::WriteData(const void* data, VkDeviceSize data_size, VkDeviceSize relative_offset, uint8_t sub_buffer_id)
    {
        size_t start_offset = relative_offset + this->sub_buffer[sub_buffer_id].offset;
        std::memcpy(this->data.get() + start_offset, data, data_size);
        this->UpdateFlushRange(start_offset, data_size);
    }

    bool ManagedBuffer::Flush()
    {
        if(this->need_flush) {
            // VIRER CETTE LIGNE
            std::vector<char> debug_data(this->data.get() + this->flush_range_start, this->data.get() + this->flush_range_end);
            size_t bytes_sent = Vulkan::GetInstance().SendToBuffer(this->buffer, this->data.get() + this->flush_range_start,
                                                                   this->flush_range_end - this->flush_range_start, this->flush_range_start);

            if(!bytes_sent) {
                this->need_flush = false;
                this->flush_range_start = 0;
                this->flush_range_end = 0;
                return false;
            }
        }

        this->need_flush = false;
        this->flush_range_start = 0;
        this->flush_range_end = 0;
        return true;
    }

    bool ManagedBuffer::ReserveChunk(VkDeviceSize& offset, size_t size)
    {
        if(!this->free_chunks.size()) return false;

        VkDeviceSize claimed_range = size;
        if(this->chunck_alignment > 0) claimed_range = static_cast<uint32_t>((size + this->chunck_alignment - 1) & ~(this->chunck_alignment - 1));

        for(auto chunk = this->free_chunks.begin(); chunk<this->free_chunks.end(); chunk++) {
            if(chunk->range > claimed_range) {
                offset = chunk->offset;
                chunk->offset += static_cast<uint32_t>(claimed_range);
                chunk->range -= claimed_range;
                return true;
            }else if(chunk->range == claimed_range) {
                offset = chunk->offset;
                this->free_chunks.erase(chunk);
                return true;
            }
        }

        return false;
    }

    bool ManagedBuffer::ReserveChunk(uint32_t& offset, size_t size, uint8_t sub_buffer_id)
    {
        if(!this->free_chunks.size()) return false;

        VkDeviceSize claimed_range = size;
        if(this->chunck_alignment > 0) claimed_range = static_cast<uint32_t>((size + this->chunck_alignment - 1) & ~(this->chunck_alignment - 1));

        for(auto chunk = this->free_chunks.begin(); chunk<this->free_chunks.end(); chunk++) {

            VkDeviceSize available_range;
            if(chunk->offset + chunk->range < this->sub_buffer[sub_buffer_id].offset
            || chunk->offset > this->sub_buffer[sub_buffer_id].offset + this->sub_buffer[sub_buffer_id].range) {
                // Le chunk ne se trouve pas dans la zone du sub-buffer, on passe au suivant
                continue;
            }else{

                // Taille du segment disponible à l'allocation
                available_range = chunk->range;

                // Le chunk démarre avant le sub-buffer
                if(chunk->offset < this->sub_buffer[sub_buffer_id].offset)
                    available_range -= this->sub_buffer[sub_buffer_id].offset - chunk->offset;

                // Le chunk se termine après le sub-buffer
                if(chunk->offset + chunk->range > this->sub_buffer[sub_buffer_id].offset + this->sub_buffer[sub_buffer_id].range)
                    available_range -= (chunk->offset + chunk->range) - (this->sub_buffer[sub_buffer_id].offset + this->sub_buffer[sub_buffer_id].range);
            }

            if(available_range >= claimed_range) {
                
                bool split_chunk = false;
                CHUNK new_chunk;

                // Le chunk démarre avant le sub-buffer
                if(chunk->offset < this->sub_buffer[sub_buffer_id].offset) {
                    new_chunk.offset = chunk->offset;
                    new_chunk.range = this->sub_buffer[sub_buffer_id].offset - chunk->offset;
                    chunk->offset = this->sub_buffer[sub_buffer_id].offset;
                    chunk->range -= new_chunk.range;
                    split_chunk = true;
                }

                offset = static_cast<uint32_t>(chunk->offset - this->sub_buffer[sub_buffer_id].offset);
                if(chunk->range == claimed_range) {

                    // Le chunk est occupé en totalité, il est détruit
                    this->free_chunks.erase(chunk);
                }else{

                    // Le chunk est réduit en taille
                    chunk->offset += static_cast<uint32_t>(claimed_range);
                    chunk->range -= claimed_range;
                }

                // Le chunk a été séparé en deux
                if(split_chunk) this->free_chunks.push_back(new_chunk);

                // Le chunk est réservé
                return true;
            }
        }

        return false;
    }
}