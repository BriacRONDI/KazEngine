#include "Chunk.h"

namespace Engine
{
    Chunk Chunk::ReserveRange(size_t size)
    {
        if(size > this->range) return {};

        if(this->free_ranges == nullptr)
            this->free_ranges = new std::vector<Chunk>(1, Chunk(0, this->range));

        if(!this->free_ranges->size()) return {};

        size_t claimed_range = size;

        for(auto iter = this->free_ranges->begin(); iter != this->free_ranges->end(); iter++) {
            Chunk& chunk = *iter;
            if(chunk.range > claimed_range) {
                Chunk result = {chunk.offset, size};
                chunk.offset += static_cast<uint32_t>(claimed_range);
                chunk.range -= claimed_range;
                return result;
            }else if(chunk.range == claimed_range) {
                Chunk result = {chunk.offset, size};
                this->free_ranges->erase(iter);
                return result;
            }
        }

        return {};
    }

    Chunk Chunk::ReserveRange(size_t size, size_t alignment)
    {
        if(size > this->range) return {};

        if(this->free_ranges == nullptr)
            this->free_ranges = new std::vector<Chunk>(1, Chunk(0, this->range));

        if(!this->free_ranges->size()) return {};

        size_t claimed_range = (size + alignment - 1) & ~(alignment - 1);

        for(auto iter = this->free_ranges->begin(); iter != this->free_ranges->end(); iter++) {
            Chunk& chunk = *iter;

            size_t aligned_offset = (chunk.offset + alignment - 1) & ~(alignment - 1);
            if(aligned_offset == chunk.offset) {
                if(chunk.range > claimed_range) {
                    Chunk result = {chunk.offset, size};
                    chunk.offset += static_cast<uint32_t>(claimed_range);
                    chunk.range -= claimed_range;
                    return result;
                }else if(chunk.range == claimed_range) {
                    Chunk result = {chunk.offset, size};
                    this->free_ranges->erase(iter);
                    return result;
                }
            }else{
                size_t new_range = aligned_offset - chunk.offset;
                size_t available_range = chunk.range - new_range;
                if(available_range >= claimed_range) {
                    
                    Chunk result = {aligned_offset, claimed_range};
                    chunk.range = new_range;
                    if(available_range != claimed_range) this->free_ranges->push_back({aligned_offset + claimed_range, available_range - claimed_range});
                    return result;
                }
            }
        }

        return {};
    }

    void Chunk::FreeRange(Chunk chunk)
    {
        if(!chunk.range) return;
        if(this->free_ranges == nullptr) return;
        auto contiguous_before = this->free_ranges->end();
        auto contiguous_after = this->free_ranges->end();

        for(auto chunk_it = this->free_ranges->begin(); chunk_it != this->free_ranges->end(); chunk_it++) {

            if(chunk_it->offset + chunk_it->range == chunk.offset) contiguous_before = chunk_it;
            if(chunk_it->offset == chunk.offset + chunk.range) contiguous_after = chunk_it;
            if(contiguous_before != this->free_ranges->end() && contiguous_after != this->free_ranges->end()) break;
        }

        if(contiguous_before != this->free_ranges->end() && contiguous_after != this->free_ranges->end()) {
            contiguous_before->range += chunk.range + contiguous_after->range;
            this->free_ranges->erase(contiguous_after);
        } else if(contiguous_before != this->free_ranges->end()) {
            contiguous_before->range += chunk.range;
        } else if(contiguous_after != this->free_ranges->end()) {
            contiguous_after->range += chunk.range;
            contiguous_after->offset -= chunk.range;
        } else {
            this->free_ranges->push_back(chunk);
        }
    }
}