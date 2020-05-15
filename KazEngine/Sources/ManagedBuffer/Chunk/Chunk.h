#pragma once

#include <vector>

namespace Engine
{
    class Chunk
    {
        public :

            size_t offset;
            size_t range;

            inline Chunk() : offset(0), range(0), free_ranges(nullptr) {}
            inline Chunk(size_t offset, size_t range) : offset(offset), range(range), free_ranges(nullptr) {}
            inline ~Chunk() { if(this->free_ranges != nullptr) delete free_ranges; }

            Chunk ReserveRange(size_t size);
            Chunk ReserveRange(size_t size, size_t alignment);
            void FreeRange(Chunk chunk);

        private :

            std::vector<Chunk>* free_ranges;
    };
}
