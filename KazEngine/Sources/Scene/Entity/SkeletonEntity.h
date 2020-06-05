#pragma once

#include "Entity.h"

namespace Engine
{
    class SkeletonEntity : public Entity
    {
        public :

            struct SKELETON_DATA {
                uint32_t animation_id;
                uint32_t frame_id;

                inline bool operator !=(SKELETON_DATA other) { return this->animation_id != other.animation_id || this->frame_id != other.frame_id; }

                /*inline void Write(size_t offset, uint8_t instance_id)
                {
                    ENTITY_DATA::Write(offset, instance_id);
                    DataBank::GetManagedBuffer().WriteData(this, SKELETON_DATA::Size(), offset + ENTITY_DATA::Size(), instance_id);
                }*/
            
            };
            
            SKELETON_DATA skeleton_properties;
            static std::shared_ptr<Chunk> skeleton_data_chunk;
            
            SkeletonEntity();
            virtual Entity& operator=(Entity const& other);
            virtual Entity& operator=(Entity&& other);

            virtual void Update(uint32_t frame_index);
            void PlayAnimation(std::string animation, float speed = 1.0f, bool loop = false);
            inline void StopAnimation() { this->animation.clear(); }
            void MoveTo(Maths::Vector3 destination, float speed = 1.0f);
            inline uint32_t GetSkeletonInstanceId() { return static_cast<uint32_t>(this->skeleton_instance_chunk->offset / sizeof(SKELETON_DATA)); }
            static bool InitilizeInstanceChunk();

        private :

            std::shared_ptr<Chunk> skeleton_instance_chunk;
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
            std::vector<SKELETON_DATA> last_skeleton_state;

            virtual bool PickChunk() override;
    };
}