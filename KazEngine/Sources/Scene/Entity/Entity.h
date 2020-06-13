#pragma once

#include <Maths.h>
#include <Model.h>
#include <EventEmitter.hpp>
#include "../../DataBank/DataBank.h"
#include "../../Platform/Common/Timer/Timer.h"
#include "../../DescriptorSet/DescriptorSet.h"
#include "IEntityListener.h"

namespace Engine
{
    class Entity : public Tools::StaticEventEmitter<IEntityListener>
    {
        public :
            
            Maths::Matrix4x4 matrix;
            bool selected;
            // static std::shared_ptr<Chunk> static_data_chunk;
            
            
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
            virtual inline uint32_t GetInstanceId() { return static_cast<uint32_t>(this->static_instance_chunk->offset / sizeof(Maths::Matrix4x4)); }
            // static bool InitilizeInstanceChunk(DescriptorSet* descriptor);
            static bool Initialize();
            static inline DescriptorSet& GetDescriptor() { return *Entity::descriptor; }
            static void Clear();

        protected :

            uint32_t id;
            std::vector<std::shared_ptr<Model::Mesh>>* meshes;
            std::shared_ptr<Chunk> static_instance_chunk;

            static uint32_t next_id;
            static DescriptorSet* descriptor;

            virtual bool PickChunk();
    };
}