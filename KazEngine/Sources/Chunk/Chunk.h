#pragma once

#include <vector>
#include <algorithm>
#include <memory>

namespace Engine
{
    class Chunk
    {
        public :

            size_t offset;
            size_t range;

            struct DEFRAG_CHUNK {
                std::shared_ptr<Chunk> chunk;
                size_t old_offset;
            };

            void Clear();
            inline Chunk() : offset(0), range(0), free_ranges(nullptr), children(nullptr) {}
            inline Chunk(Chunk const& other) { *this = other; }
            inline Chunk(Chunk&& other) { *this = std::move(other); }
            inline Chunk(size_t offset, size_t range, uint32_t alignment = 0) : offset(offset), range(range), free_ranges(nullptr), children(nullptr) {}
            Chunk& operator=(Chunk const& other);
            Chunk& operator=(Chunk&& other);
            inline ~Chunk() { this->Clear(); }
            inline size_t TotalFree() {  if(!this->free_ranges) return this->range; if(this->free_ranges->empty()) return 0; size_t total = 0; for(auto& free : *this->free_ranges) total += free.range; return total; }
            inline std::shared_ptr<Chunk> ReserveRange(size_t size) { return this->ReserveRange(size, nullptr); }
            inline std::shared_ptr<Chunk> ReserveRange(size_t size, size_t alignment) { return this->ReserveRange(size, alignment, nullptr); }
            bool ResizeChild(std::shared_ptr<Chunk> child, size_t size, bool& relocated, size_t alignment = 0);
            void FreeChild(std::shared_ptr<Chunk> child);
            bool Defragment(size_t alignment, std::vector<DEFRAG_CHUNK>& defrag, std::shared_ptr<Chunk> extend_me = nullptr, size_t extension = 0);
            static inline bool CompareOffsets(std::shared_ptr<Chunk> a, std::shared_ptr<Chunk> b) { return (a->offset < b->offset); }

        private :

            std::vector<Chunk>* free_ranges;
            std::vector<std::shared_ptr<Chunk>>* children;

            bool ExtendChild(std::shared_ptr<Chunk> child, size_t extension);
            void FreeRange(size_t offset, size_t range);
            std::shared_ptr<Chunk> ReserveRange(size_t size, std::shared_ptr<Chunk> child);
            std::shared_ptr<Chunk> ReserveRange(size_t size, size_t alignment, std::shared_ptr<Chunk> child);
            inline std::shared_ptr<Chunk> MoveOrAppendChild(size_t offset, size_t range, std::shared_ptr<Chunk> child);
            
    };
}
