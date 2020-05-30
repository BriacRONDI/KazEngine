#pragma once

#include "Entity.h"

namespace Engine
{
    class SkeletonEntity : public Entity
    {
        public :

            struct ENTITY_SKELETON_DATA : ENTITY_DATA {
                uint32_t animation_id;
                uint32_t frame_id;

                virtual inline bool operator !=(ENTITY_DATA other)
                {
                    ENTITY_SKELETON_DATA* skull = dynamic_cast<ENTITY_SKELETON_DATA*>(&other);
                    return this->matrix != other.matrix
                           || this->animation_id != skull->animation_id
                           || this->frame_id != skull->frame_id
                           || this->selected != skull->selected;
                }

                virtual inline uint32_t Size() { return ENTITY_DATA::Size() + sizeof(uint32_t) * 2; }
                virtual inline void Write(size_t offset, uint8_t instance_id)
                {
                    ENTITY_DATA::Write(offset, instance_id);
                    DataBank::GetManagedBuffer().WriteData(&this->animation_id, this->Size(), offset + ENTITY_DATA::Size(), instance_id);
                }
            
            };
            
            ENTITY_SKELETON_DATA properties;
            static std::shared_ptr<Chunk> skeleton_instance_chunk;
            
            SkeletonEntity();
            virtual Entity& operator=(Entity const& other);
            virtual Entity& operator=(Entity&& other);

            virtual void Update(uint32_t frame_index);
            void PlayAnimation(std::string animation, float speed = 1.0f, bool loop = false);
            inline void StopAnimation() { this->animation.clear(); }
            void MoveTo(Maths::Vector3 destination, float speed = 1.0f);
            virtual bool AddToScene();

        private :

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
    };
}