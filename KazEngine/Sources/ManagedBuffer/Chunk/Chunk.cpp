#include "Chunk.h"

namespace Engine
{
    void Chunk::Clear()
    {
        if(this->free_ranges != nullptr) delete free_ranges;
        if(this->children != nullptr) delete children;
        this->free_ranges = nullptr;
        this->children = nullptr;
        this->offset = 0;
        this->range = 0;
    }

    Chunk& Chunk::operator=(Chunk const& other)
    {
        if(&other != this) {
            this->offset = other.offset;
            this->range = other.range;
            if(other.free_ranges != nullptr) this->free_ranges = new std::vector<Chunk>(*other.free_ranges);
            else this->free_ranges = nullptr;
            if(other.children != nullptr) this->children = new std::vector<std::shared_ptr<Chunk>>(*other.children);
            else this->children = nullptr;
        }

        return *this;
    }

    Chunk& Chunk::operator=(Chunk&& other)
    {
        if(&other != this) {
            this->offset = other.offset;
            this->range = other.range;
            this->free_ranges = other.free_ranges;
            this->children = other.children;
            other.offset = 0;
            other.range = 0;
            other.free_ranges = nullptr;
            other.children = nullptr;
        }

        return *this;
    }

    inline std::shared_ptr<Chunk> Chunk::MoveOrAppendChild(size_t offset, size_t range, std::shared_ptr<Chunk> child)
    {
        if(child != nullptr) {
            size_t free_offset = child->range;
            child->offset = offset;
            child->range = range;
            child->FreeRange(free_offset, child->range - free_offset);
            return child;
        }else{
            auto new_child = std::shared_ptr<Chunk>(new Chunk(offset, range));
            if(this->children == nullptr) this->children = new std::vector<std::shared_ptr<Chunk>>;
            this->children->push_back(new_child);
            return new_child;
        }
    }

    std::shared_ptr<Chunk> Chunk::ReserveRange(size_t size, std::shared_ptr<Chunk> child)
    {
        if(size > this->range) return nullptr;

        if(this->free_ranges == nullptr)
            this->free_ranges = new std::vector<Chunk>(1, Chunk(0, this->range));

        if(!this->free_ranges->size()) return nullptr;

        for(auto iter = this->free_ranges->begin(); iter != this->free_ranges->end(); iter++) {
            Chunk& chunk = *iter;
            if(chunk.range > size) {
                std::shared_ptr<Chunk> result = this->MoveOrAppendChild(chunk.offset, size, child);
                chunk.offset += static_cast<uint32_t>(size);
                chunk.range -= size;
                return result;
            }else if(chunk.range == size) {
                std::shared_ptr<Chunk> result = this->MoveOrAppendChild(chunk.offset, size, child);
                this->free_ranges->erase(iter);
                return result;
            }
        }

        return nullptr;
    }

    std::shared_ptr<Chunk> Chunk::ReserveRange(size_t size, size_t alignment, std::shared_ptr<Chunk> child)
    {
        if(size > this->range) return nullptr;

        if(!alignment) return this->ReserveRange(size, child);

        if(this->free_ranges == nullptr)
            this->free_ranges = new std::vector<Chunk>(1, Chunk(0, this->range));

        if(!this->free_ranges->size()) return nullptr;

        size_t claimed_range = (size + alignment - 1) & ~(alignment - 1);
        for(auto iter = this->free_ranges->begin(); iter != this->free_ranges->end(); iter++) {
            Chunk& chunk = *iter;

            size_t aligned_offset = (chunk.offset + alignment - 1) & ~(alignment - 1);
            if(aligned_offset == chunk.offset) {
                if(chunk.range > claimed_range) {
                    std::shared_ptr<Chunk> result = this->MoveOrAppendChild(chunk.offset, claimed_range, child);
                    chunk.offset += claimed_range;
                    chunk.range -= claimed_range;
                    return result;
                }else if(chunk.range == claimed_range) {
                    std::shared_ptr<Chunk> result = this->MoveOrAppendChild(chunk.offset, claimed_range, child);
                    this->free_ranges->erase(iter);
                    return result;
                }
            }else{
                size_t new_range = aligned_offset - chunk.offset;
                size_t available_range = chunk.range - new_range;
                if(available_range >= claimed_range) {
                    std::shared_ptr<Chunk> result = this->MoveOrAppendChild(aligned_offset, claimed_range, child);
                    chunk.range = new_range;
                    if(available_range != claimed_range) this->free_ranges->push_back({aligned_offset + claimed_range, available_range - claimed_range});
                    return result;
                }
            }
        }

        return nullptr;
    }

    void Chunk::FreeRange(size_t offset, size_t range)
    {
        if(!range) return;
        if(this->free_ranges == nullptr) return;
        /*if(this->free_ranges == nullptr) {
            this->free_ranges = new std::vector<Chunk>(1, chunk);
            return;
        }*/

        auto contiguous_before = this->free_ranges->end();
        auto contiguous_after = this->free_ranges->end();

        for(auto chunk_it = this->free_ranges->begin(); chunk_it != this->free_ranges->end(); chunk_it++) {

            if(chunk_it->offset + chunk_it->range == offset) contiguous_before = chunk_it;
            if(chunk_it->offset == offset + range) contiguous_after = chunk_it;
            if(contiguous_before != this->free_ranges->end() && contiguous_after != this->free_ranges->end()) break;
        }

        if(contiguous_before != this->free_ranges->end() && contiguous_after != this->free_ranges->end()) {
            contiguous_before->range += range + contiguous_after->range;
            this->free_ranges->erase(contiguous_after);
        } else if(contiguous_before != this->free_ranges->end()) {
            contiguous_before->range += range;
        } else if(contiguous_after != this->free_ranges->end()) {
            contiguous_after->range += range;
            contiguous_after->offset -= range;
        } else {
            this->free_ranges->push_back({offset, range});
        }
    }

    void Chunk::FreeChild(std::shared_ptr<Chunk> child)
    {
        if(this->children != nullptr) {
            for(auto it = this->children->begin(); it != this->children->end(); it++) {
                if(*it == child) {
                    this->FreeRange(child->offset, child->range);
                    this->children->erase(it);
                    return;
                }
            }
        }
    }

    /* bool Chunk::ReserveRange(Chunk& chunk, size_t alignment)
    {
        size_t claimed_range = (alignment > 0) ? (chunk.range + alignment - 1) & ~(alignment - 1) : chunk.range;

        if(chunk.range > this->range) return false;

        if(this->free_ranges == nullptr)
            this->free_ranges = new std::vector<Chunk>(1, Chunk(0, this->range));

        if(!this->free_ranges->size()) return false;
        
        for(auto free_it = this->free_ranges->begin(); free_it != this->free_ranges->end(); free_it++) {
            Chunk& free = *free_it;
            if(free.offset <= chunk.offset && free.range + free.offset >= claimed_range + chunk.offset) {
                if(free.offset == chunk.offset && free.range == claimed_range) {
                    this->free_ranges->erase(free_it);
                }else if(free.offset == chunk.offset) {
                    free.offset += claimed_range;
                    free.range -= claimed_range;
                }else if(free.range + free.offset == claimed_range + chunk.offset) {
                    free.range -= claimed_range;
                }else{
                    this->free_ranges->push_back({claimed_range + chunk.offset, free.offset + free.range - chunk.offset});
                    free.range = chunk.offset - free.offset;
                }

                if(alignment > 0) chunk.range = claimed_range;
                return true;
            }
        }

        return false;
    }*/

    bool Chunk::ExtendChild(std::shared_ptr<Chunk> child, size_t extension)
    {
        if(extension > this->range) return false;

        if(this->free_ranges == nullptr)
            this->free_ranges = new std::vector<Chunk>(1, Chunk(0, this->range));

        if(!this->free_ranges->size()) return false;
        
        size_t extension_offset = child->offset + child->range;
        for(auto free_it = this->free_ranges->begin(); free_it != this->free_ranges->end(); free_it++) {
            Chunk& free = *free_it;
            if(free.offset == extension_offset) {
                if(free.range == extension) {
                    this->free_ranges->erase(free_it);
                    size_t free_offset = child->range;
                    child->range += extension;
                    child->FreeRange(free_offset, extension);
                    return true;
                }else if(free.range > extension) {
                    size_t free_offset = child->range;
                    free.range -= extension;
                    free.offset += extension;
                    child->range += extension;
                    child->FreeRange(free_offset, extension);
                    return true;
                }
            }
        }

        return false;
    }

    bool Chunk::ResizeChild(std::shared_ptr<Chunk> child, size_t size, bool& relocated, size_t alignment)
    {
        if(size == child->range) return true;
        if(size < child->range) {
            size_t claimed_range = (alignment > 0) ? (size + alignment - 1) & ~(alignment - 1) : size;
            if(claimed_range == child->range) return true;
            if(claimed_range > child->range) return this->ResizeChild(child, claimed_range, relocated, alignment);
            // Chunk free = {chunk->offset + claimed_range, chunk->range - claimed_range};
            this->FreeRange(child->offset + claimed_range, child->range - claimed_range);
            child->range = claimed_range;
            relocated = false;
            return true;
        }else{
            size_t extension_range = size - child->range;
            if(alignment > 0) extension_range = (extension_range + alignment - 1) & ~(alignment - 1);
            if(this->ExtendChild(child, extension_range)) {
                relocated = false;
                return true;
            }else{
                if(this->ReserveRange(size, alignment, child) == nullptr) return false;
                relocated = true;
                return true;
            }
        }
    }

    bool Chunk::Defragment(size_t alignment, std::vector<DEFRAG_CHUNK>& defrag, std::shared_ptr<Chunk> extend_me, size_t extension)
    {
        if(this->children == nullptr) return extend_me == nullptr;
        if(defrag.size() > 0) return false;

        size_t total_free = this->TotalFree();
        if(total_free == 0) return extension == 0;
        if(total_free < extension) return false;

        std::sort(this->children->begin(), this->children->end(), Chunk::CompareOffsets);

        std::vector<Chunk> new_free_ranges;

        size_t free_offset = 0;
        for(auto& chunk : *this->children) {
            
            if(free_offset != chunk->offset || chunk == extend_me) defrag.push_back({chunk, free_offset});
            if(chunk == extend_me) continue;

            free_offset += chunk->range;
            size_t mem_offset = free_offset;
            if(alignment > 0) free_offset = (free_offset + alignment - 1) & ~(alignment - 1);
            if(free_offset > mem_offset) new_free_ranges.push_back({mem_offset, free_offset - mem_offset});
        }

        if(extend_me != nullptr) {
            size_t free_range = this->range - free_offset - extend_me->range;
            if(free_range < extension) {
                defrag.clear();
                return false;
            }else if(free_range > extension) {
                new_free_ranges.push_back({free_offset + extend_me->range + extension, free_range - extension});
            }
        }else{
            if(this->range > free_offset) new_free_ranges.push_back({free_offset, this->range - free_offset});
        }
        *this->free_ranges = new_free_ranges;

        for(auto& fragment : defrag) {
            if(fragment.chunk == extend_me) {
                size_t extension_offset = fragment.chunk->range;
                fragment.chunk->range += extension;
                fragment.chunk->offset = free_offset;
                fragment.chunk->FreeRange(extension_offset, extension);
            }else{
                size_t old_offset = fragment.chunk->offset;
                fragment.chunk->offset = fragment.old_offset;
                fragment.old_offset = old_offset;
            }
        }

        return true;
    }

    /*bool Chunk::ResizeChunk(std::vector<Chunk>::iterator const chunk, size_t size, bool& relocated, size_t alignment)
    {
        if(size == chunk->range) return true;
        if(size < chunk->range) {
            size_t claimed_range = (alignment > 0) ? (size + alignment - 1) & ~(alignment - 1) : size;
            if(claimed_range == chunk->range) return true;
            if(claimed_range > chunk->range) return this->ResizeChunk(chunk, claimed_range, relocated, alignment);
            Chunk free = {chunk->offset + claimed_range, chunk->range - claimed_range};
            this->FreeRange(free);
            chunk->range = claimed_range;
            relocated = false;
            return true;
        }else{
            size_t extension_range = size - chunk->range;
            if(alignment > 0) extension_range = (extension_range + alignment - 1) & ~(alignment - 1);
            Chunk extension = {chunk->offset + chunk->range, extension_range};
            if(this->ReserveRange(extension, alignment)) {
                Chunk free_space = {chunk->range, extension_range};
                chunk->range += extension_range;
                chunk->FreeRange(free_space);
                relocated = false;
                return true;
            }else{
                Chunk relocated_chunk = this->ReserveRange(size, alignment);
                if(!relocated_chunk.range) return false;
                Chunk free_space = {chunk->offset, chunk->range};
                this->FreeRange(free_space);
                free_space = {chunk->range, extension_range};
                chunk->offset = relocated_chunk.offset;
                chunk->range = relocated_chunk.range;
                chunk->FreeRange(free_space);
                relocated = true;
                return true;
            }
        }
    }*/
}