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

            struct ENTITY_UBO {
                Maths::Matrix4x4 matrix;
                uint32_t animation_id;
                uint32_t frame_id;
                uint32_t selected;
            };
            
            ENTITY_UBO properties;
            uint32_t data_offset;
            
            Entity();
            ~Entity();
            void AddMesh(std::shared_ptr<Model::Mesh> mesh);
            inline std::vector<std::shared_ptr<Model::Mesh>> const* GetMeshes() { return this->meshes; }
            inline uint32_t const& GetId() { return this->id; }
            void Update(VkDeviceSize offset, uint32_t frame_index);
            void PlayAnimation(std::string animation, float speed = 1.0f, bool loop = false);
            inline void StopAnimation() { this->animation.clear(); }
            void MoveTo(Maths::Vector3 destination, float speed = 1.0f);
            bool InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane);
            bool IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction);

            static inline void UpdateUboSize() { Entity::ubo_size = Vulkan::GetInstance().ComputeStorageBufferAlignment(sizeof(ENTITY_UBO)); }
            static inline uint32_t GetUboSize() { return Entity::ubo_size; }

        private :

            uint32_t id;
            std::vector<std::shared_ptr<Model::Mesh>>* meshes;
            Timer animation_timer;
            Timer move_timer;
            std::string animation;
            bool loop_animation;
            float animation_speed;

            bool moving;
            Maths::Vector3 move_origin;
            Maths::Vector3 move_destination;
            Maths::Vector3 move_direction;
            float base_move_speed;
            float move_speed;
            float move_length;

            static uint32_t next_id;
            static uint32_t ubo_size;
    };
}