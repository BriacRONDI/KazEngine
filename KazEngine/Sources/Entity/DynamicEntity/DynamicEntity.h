#pragma once

#include <chrono>
#include "../../DescriptorSet/DescriptorSet.h"
#include "../../Camera/Camera.h"
#include "../Entity.h"
#include "../../Platform/Common/Timer/Timer.h"

#define DESCRIPTOR_BIND_FRAME       static_cast<uint8_t>(0)
#define DESCRIPTOR_BIND_ANIMATION   static_cast<uint8_t>(1)
#define DESCRIPTOR_BIND_TIME        static_cast<uint8_t>(2)

#define DESCRIPTOR_BIND_PLANES      static_cast<uint8_t>(1)

namespace Engine
{
    class DynamicEntity : public Entity
    {
        public :

            DynamicEntity(Maths::Matrix4x4 matrix = IDENTITY_MATRIX);
            virtual inline void SetMatrix(Maths::Matrix4x4 matrix) { DynamicEntity::matrix_descriptor.WriteData(&matrix, sizeof(Maths::Matrix4x4), this->matrix_chunk->offset); }
            virtual inline Maths::Matrix4x4 const& GetMatrix() const { return DynamicEntity::matrix_array[this->GetInstanceId()]; }
            bool InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane);
            bool IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction);
            void PlayAnimation(std::string animation, float speed, bool loop);
            void StopAnimation();
            void SetAnimationFrame(std::string animation, uint32_t frame_id);
            inline bool IsSelected() { return this->selected; }
            inline void SetMovementChunk(std::shared_ptr<Chunk> chunk) { this->movement_chunk = chunk; }
            inline std::shared_ptr<Chunk> GetMovementChunk() { return this->movement_chunk; }
            /*inline void SetMovement(MOVEMENT_DATA movement) { DynamicEntity::movement_descriptor.WriteData(&movement, sizeof(MOVEMENT_DATA), this->movement_chunk->offset); }
            inline MOVEMENT_DATA const& GetMovement() const { return DynamicEntity::movement_array[this->movement_chunk->offset / sizeof(MOVEMENT_DATA)]; }*/

            static DynamicEntity* ToggleSelection(Point<uint32_t> mouse_position);
            static std::vector<DynamicEntity*> SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end);
            static void Clear();
            static bool Initialize();
            static inline DescriptorSet const& GetMatrixDescriptor() { return DynamicEntity::matrix_descriptor; }
            static inline DescriptorSet const& GetAnimationDescriptor() { return DynamicEntity::animation_descriptor; }
            // static inline DescriptorSet const& GetMovementDescriptor() { return DynamicEntity::movement_descriptor; }
            static inline DescriptorSet const& GetSelectionDescriptor() { return DynamicEntity::selection_descriptor; }
            static inline bool UpdateMatrixDescriptor(uint8_t instance_id) { return DynamicEntity::matrix_descriptor.Update(instance_id); }
            static inline bool UpdateAnimationDescriptor(uint8_t instance_id) { return DynamicEntity::animation_descriptor.Update(instance_id); }
            // static inline bool UpdateMovementDescriptor(uint8_t instance_id) { return DynamicEntity::movement_descriptor.Update(instance_id); }
            static inline bool UpdateSelectionDescriptor(uint8_t instance_id) { return DynamicEntity::selection_descriptor.Update(instance_id); }
            static void Update(uint8_t instance_id);
            static inline void ReadMatrix() { DynamicEntity::matrix_descriptor.ReadData(); }
            // static inline void ReadMovement() { DynamicEntity::movement_descriptor.ReadData(); DynamicEntity::movement_descriptor.ReadData(DESCRIPTOR_BIND_GROUPS); }
            // static void MoveToPosition(Maths::Vector3 destination);
            static void UpdateSelection(std::vector<DynamicEntity*> entities);
            static inline std::vector<DynamicEntity*> GetEntities() { return DynamicEntity::entities; }

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
            std::shared_ptr<Chunk> movement_chunk;
            ANIMATION_DATA animation;
            FRAME_DATA frame;

            bool moving;
            std::chrono::system_clock::time_point move_start;
            Maths::Vector3 move_initial_position;
            Maths::Vector3 move_destination;
            Maths::Vector3 move_direction;
            float move_speed;
            float move_length;

            //static uint32_t movement_group_count;
            static Maths::Matrix4x4* matrix_array;
            /*static MOVEMENT_GROUP* groups_array;
            static MOVEMENT_DATA* movement_array;*/
            static DescriptorSet matrix_descriptor;
            static DescriptorSet animation_descriptor;
            static DescriptorSet selection_descriptor;
            static std::vector<DynamicEntity*> entities;
    };
}