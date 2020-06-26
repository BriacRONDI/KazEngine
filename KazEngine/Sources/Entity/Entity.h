#pragma once

#include <Maths.h>
#include <Model.h>
#include <EventEmitter.hpp>
#include "IEntityListener.h"
#include "../ManagedBuffer/Chunk/Chunk.h"

namespace Engine
{
    class Entity : public Tools::StaticEventEmitter<IEntityListener>
    {
        public :

            Entity() { this->id = Entity::next_id; Entity::next_id++; }
            virtual void SetMatrix(Maths::Matrix4x4 matrix) = 0;
            virtual inline Maths::Matrix4x4 const& GetMatrix() const { return this->matrix; }
            virtual inline std::vector<std::shared_ptr<Model::Mesh>> const& GetMeshes() const { return this->meshes; }
            virtual inline void AddMesh(std::shared_ptr<Model::Mesh> mesh) { this->meshes.push_back(mesh); for(auto& listener : Entity::Listeners) listener->AddMesh(*this, mesh); }
            virtual inline uint32_t GetInstanceId() { return static_cast<uint32_t>(this->matrix_chunk->offset / sizeof(Maths::Matrix4x4)); }
            virtual inline uint32_t GetId() const { return this->id; }

        protected :

            Maths::Matrix4x4 matrix;
            std::vector<std::shared_ptr<Model::Mesh>> meshes;
            std::shared_ptr<Chunk> matrix_chunk;
            uint32_t id;

            static uint32_t next_id;
    };
}