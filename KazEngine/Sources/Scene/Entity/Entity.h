#pragma once

#include <thread>
#include <mutex>
#include <Maths.h>
#include <Model.h>
#include <EventEmitter.hpp>
#include "../../DataBank/DataBank.h"
#include "../../Platform/Common/Timer/Timer.h"
#include "../../DescriptorSet/DescriptorSet.h"
#include "../../Camera/Camera.h"
#include "IEntityListener.h"

namespace Engine
{
    class Entity : public Tools::StaticEventEmitter<IEntityListener>
    {
        public :
            
            Entity();
            inline Entity(bool skip_me) {};
            virtual ~Entity();
            inline Entity(Entity const& other) { *this = other; }
            inline Entity(Entity&& other) { *this = std::move(other); }
            virtual Entity& operator=(Entity const& other);
            virtual Entity& operator=(Entity&& other);
            void AddMesh(std::shared_ptr<Model::Mesh> mesh);
            inline std::vector<std::shared_ptr<Model::Mesh>> const* GetMeshes() { return this->meshes; }
            inline uint32_t const& GetId() { return this->id; }
            virtual inline void Update(uint32_t frame_index) {};
            bool InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane);
            bool IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction);
            virtual inline uint32_t GetInstanceId() { return static_cast<uint32_t>(this->static_instance_chunk->offset / sizeof(Maths::Matrix4x4)); }
            static bool Initialize();
            static inline DescriptorSet& GetDescriptor() { return *Entity::descriptor; }
            static void Clear();
            virtual inline uint32_t GetStaticDataOffset() { return static_cast<uint32_t>(this->static_instance_chunk->offset); }
            virtual std::vector<Chunk> UpdateData(uint8_t instance_id);
            static Entity* ToggleSelection(Point<uint32_t> mouse_position);
            static std::vector<Entity*> SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end);
            static void UpdateAll(uint32_t frame_index);
            void SetMatrix(Maths::Matrix4x4 matrix);

        protected :

            Maths::Matrix4x4 matrix;
            bool selected;

            uint32_t id;
            std::vector<std::shared_ptr<Model::Mesh>>* meshes;
            std::shared_ptr<Chunk> static_instance_chunk;

            static uint32_t next_id;
            static DescriptorSet* descriptor;
            static std::vector<Entity*> entities;

            virtual bool PickChunk();
    };
}