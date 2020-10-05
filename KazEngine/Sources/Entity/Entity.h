#pragma once

#include <Maths.h>
#include <EventEmitter.hpp>
// #include "IEntityListener.h"
#include "../ManagedBuffer/Chunk/Chunk.h"
#include "../LOD/LOD.h"

namespace Engine
{
    class IEntityListener;
    class Entity : public Tools::StaticEventEmitter<IEntityListener>
    {
        public :

            Entity() { this->id = Entity::next_id; Entity::next_id++; }
            virtual void SetMatrix(Maths::Matrix4x4 matrix) = 0;
            virtual Maths::Matrix4x4 const& GetMatrix() const = 0;
            virtual inline std::vector<std::shared_ptr<LODGroup>> const& GetLODs() const { return this->lods; }
            virtual void AddLOD(std::shared_ptr<LODGroup> lod);
            virtual inline uint32_t GetInstanceId() const { return static_cast<uint32_t>(this->matrix_chunk->offset / sizeof(Maths::Matrix4x4)); }
            virtual inline uint32_t GetId() const { return this->id; }

        protected :

            std::vector<std::shared_ptr<LODGroup>> lods;
            std::shared_ptr<Chunk> matrix_chunk;
            uint32_t id;

            static uint32_t next_id;
    };
}