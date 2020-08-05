#pragma once

#include <chrono>
#include "../../DescriptorSet/DescriptorSet.h"
#include "../../Camera/Camera.h"
#include "../Entity.h"

#define DESCRIPTOR_BIND_FRAME       static_cast<uint8_t>(0)
#define DESCRIPTOR_BIND_ANIMATION   static_cast<uint8_t>(1)
#define DESCRIPTOR_BIND_TIME        static_cast<uint8_t>(2)

namespace Engine
{
    class DynamicEntity : public Entity
    {
        public :

            DynamicEntity();
            virtual inline void SetMatrix(Maths::Matrix4x4 matrix);
            bool InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane);
            bool IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction);
            void PlayAnimation(std::string animation, float speed, bool loop);
            void StopAnimation();
            void SetAnimationFrame(std::string animation, uint32_t frame_id);

            static DynamicEntity* ToggleSelection(Point<uint32_t> mouse_position);
            static std::vector<DynamicEntity*> SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end);
            static void Clear();
            static bool Initialize();
            static inline DescriptorSet const& GetMatrixDescriptor() { return DynamicEntity::matrix_descriptor; }
            static inline DescriptorSet const& GetAnimationDescriptor() { return DynamicEntity::animation_descriptor; }
            static void UpdateAnimationTimer();
            static inline bool UpdateMatrixDescriptor(uint8_t instance_id) { return DynamicEntity::matrix_descriptor.Update(instance_id); }

        private :

            struct FRAME_DATA {
                uint32_t animation_id;
                uint32_t frame_id;
            };

            struct ANIMATION_DATA {
                uint32_t frame_count;
                uint32_t loop;
                uint32_t play;
                uint32_t duration;
                uint32_t start;
                float speed;
            };

            bool selected;
            std::shared_ptr<Chunk> frame_chunk;
            std::shared_ptr<Chunk> animation_chunk;
            ANIMATION_DATA animation;
            FRAME_DATA frame;

            static DescriptorSet matrix_descriptor;
            static DescriptorSet animation_descriptor;
            static std::vector<DynamicEntity*> entities;
            static std::chrono::system_clock::time_point animation_time;
    };
}