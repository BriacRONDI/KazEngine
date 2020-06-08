#pragma once

#include <Maths.h>
#include <Model.h>
#include "../../DataBank/DataBank.h"
#include "../../Platform/Common/Timer/Timer.h"

namespace Engine
{
    class Entity
    {
        public :

            struct ENTITY_DATA {
                Maths::Matrix4x4 matrix;
                uint32_t selected;

                inline bool operator !=(ENTITY_DATA other) { return this->matrix != other.matrix || this->selected != other.selected; }
                // static inline uint32_t Size() { return sizeof(Maths::Matrix4x4) + sizeof(uint32_t); }
                // virtual inline void Write(size_t offset, uint8_t instance_id) { DataBank::GetManagedBuffer().WriteData(&this->matrix, this->Size(), offset, instance_id); }
            };
            
            ENTITY_DATA properties;
            static std::shared_ptr<Chunk> static_data_chunk;
            
            Entity(bool pick_chunk = true);
            virtual ~Entity();
            inline Entity(Entity const& other) { *this = other; }
            inline Entity(Entity&& other) { *this = std::move(other); }
            virtual Entity& operator=(Entity const& other);
            virtual Entity& operator=(Entity&& other);
            void AddMesh(std::shared_ptr<Model::Mesh> mesh);
            inline std::vector<std::shared_ptr<Model::Mesh>> const* GetMeshes() { return this->meshes; }
            inline uint32_t const& GetId() { return this->id; }
            virtual void Update(uint32_t frame_index);
            bool InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane);
            bool IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction);
            virtual inline uint32_t GetInstanceId() { return static_cast<uint32_t>(this->static_instance_chunk->offset / sizeof(ENTITY_DATA)); }
            static bool InitilizeInstanceChunk();

        protected :

            uint32_t id;
            std::vector<std::shared_ptr<Model::Mesh>>* meshes;
            std::vector<ENTITY_DATA> last_static_state;
            std::shared_ptr<Chunk> static_instance_chunk;

            static uint32_t next_id;

            virtual bool PickChunk();
    };
}