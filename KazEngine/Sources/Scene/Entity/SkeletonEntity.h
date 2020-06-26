#pragma once

#include "Entity.h"

#define DESCRIPTOR_BIND_FRAME       static_cast<uint8_t>(0)
#define DESCRIPTOR_BIND_ANIMATION   static_cast<uint8_t>(1)
#define DESCRIPTOR_BIND_TIME        static_cast<uint8_t>(2)

namespace Engine
{
    class SkeletonEntity : public Entity
    {
        public :

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
            
            FRAME_DATA frame;
            static std::shared_ptr<Chunk> skeleton_data_chunk;
            static std::shared_ptr<Chunk> absolute_skeleton_data_chunk;
            static std::shared_ptr<Chunk> animation_data_chunk;
            
            SkeletonEntity();
            virtual Entity& operator=(Entity const& other);
            virtual Entity& operator=(Entity&& other);

            virtual void Update(uint32_t frame_index);
            void PlayAnimation(std::string animation, float speed = 1.0f, bool loop = false);
            inline void StopAnimation() { this->animation.clear(); }
            void MoveTo(Maths::Vector3 destination, float speed = 1.0f);
            static bool Initialize();
            virtual inline uint32_t GetStaticDataOffset() { return static_cast<uint32_t>(SkeletonEntity::skeleton_data_chunk->offset + this->static_instance_chunk->offset); }
            static inline DescriptorSet& GetDescriptor() { return *SkeletonEntity::animation_descriptor; }
            static void Clear();
            static void UpdateAnimationTimer();

        private :

            std::shared_ptr<Chunk> frame_chunk;
            std::shared_ptr<Chunk> animation_chunk;
            Timer move_timer;
            std::string animation;
            bool moving;
            Maths::Vector3 move_origin;
            Maths::Vector3 move_destination;
            Maths::Vector3 move_direction;
            float base_move_speed;
            float move_speed;
            float move_length;
            ANIMATION_DATA animation_instance;

            static DescriptorSet* animation_descriptor;
            static std::chrono::system_clock::time_point animation_time;
            virtual bool PickChunk();
    };
}