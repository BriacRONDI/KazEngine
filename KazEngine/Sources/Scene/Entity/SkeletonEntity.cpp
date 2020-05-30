#include "SkeletonEntity.h"

namespace Engine
{
    std::shared_ptr<Chunk> SkeletonEntity::skeleton_instance_chunk;

    SkeletonEntity::SkeletonEntity() : Entity()
    {
        this->properties.frame_id = 0;
        this->moving = false;
        this->base_move_speed = 5.0f / 1000.0f;
    }

    bool SkeletonEntity::AddToScene()
    {
        if(this->chunk != nullptr) return true;
        this->chunk = SkeletonEntity::skeleton_instance_chunk->ReserveRange(this->properties.Size());
        if(this->chunk == nullptr) {
            bool relocated;
            if(!DataBank::GetManagedBuffer().ResizeChunk(SkeletonEntity::skeleton_instance_chunk, SkeletonEntity::skeleton_instance_chunk->range + this->properties.Size() * 15, relocated)) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Entity::SetupChunk : Not enough memory" << std::endl;
                #endif
            }
            this->chunk = SkeletonEntity::skeleton_instance_chunk->ReserveRange(this->properties.Size());
        }

        return this->chunk != nullptr;
    }

    void SkeletonEntity::Update(uint32_t frame_index)
    {
        if(!this->animation.empty()) {
            uint32_t last_frame_id = DataBank::GetAnimation(this->animation).frame_count - 1;
            float progression = this->animation_timer.GetProgression();
            if(this->loop_animation) {
                this->properties.frame_id = static_cast<uint32_t>(progression * last_frame_id) % last_frame_id;
            }else{
                this->properties.frame_id = last_frame_id;
                this->animation.clear();
            }
        }

        if(this->moving) {
            Maths::Vector3 new_position = this->move_origin + this->move_direction * this->move_speed * static_cast<float>(this->move_timer.GetDuration().count());
            if((new_position - this->move_origin).Length() >= this->move_length) {
                this->moving = false;
                this->properties.matrix.SetTranslation(this->move_destination);
            }else{
                this->properties.matrix.SetTranslation(new_position);
            }
        }

        if(this->last_state[frame_index] != this->properties) {
            this->properties.Write(SkeletonEntity::skeleton_instance_chunk->offset + this->chunk->offset, frame_index);
            this->last_state[frame_index] = this->properties;
        }
    }

    Entity& SkeletonEntity::operator=(Entity const& other)
    {
        if(&other != this) {

            Entity::operator=(other);
            SkeletonEntity* skull = dynamic_cast<SkeletonEntity*>(const_cast<Entity*>(&other));

            if(skull != nullptr) {
                this->animation = skull->animation;
                this->animation_speed = skull->animation_speed;
                this->animation_timer = skull->animation_timer;
                this->base_move_speed = skull->base_move_speed;
                this->loop_animation = skull->loop_animation;
                this->move_destination = skull->move_destination;
                this->move_direction = skull->move_direction;
                this->move_length = skull->move_length;
                this->move_origin = skull->move_origin;
                this->move_speed = skull->move_speed;
                this->move_timer = skull->move_timer;
                this->moving = skull->moving;
            }
        }

        return *this;
    }

    Entity& SkeletonEntity::operator=(Entity&& other)
    {
        if(&other != this) {

            Entity::operator=(other);
            SkeletonEntity* skull = dynamic_cast<SkeletonEntity*>(const_cast<Entity*>(&other));

            if(skull != nullptr) {
                this->animation = skull->animation;
                this->animation_speed = skull->animation_speed;
                this->animation_timer = skull->animation_timer;
                this->base_move_speed = skull->base_move_speed;
                this->loop_animation = skull->loop_animation;
                this->move_destination = skull->move_destination;
                this->move_direction = skull->move_direction;
                this->move_length = skull->move_length;
                this->move_origin = skull->move_origin;
                this->move_speed = skull->move_speed;
                this->move_timer = skull->move_timer;
                this->moving = skull->moving;
            }
        }

        return *this;
    }

    void SkeletonEntity::PlayAnimation(std::string animation, float speed, bool loop)
    {
        if(DataBank::HasAnimation(animation)) {
            this->animation = animation;
            this->properties.animation_id = DataBank::GetAnimation(animation).animation_id;

            if(speed == 1.0f) this->animation_timer.Start(DataBank::GetAnimation(animation).duration);
            else this->animation_timer.Start(std::chrono::milliseconds(static_cast<uint64_t>(DataBank::GetAnimation(animation).duration.count() / speed)));
            
            this->loop_animation = loop;
            this->animation_speed = speed;
        }
    }

    void SkeletonEntity::MoveTo(Maths::Vector3 destination, float speed)
    {
        this->move_origin = this->properties.matrix.GetTranslation();
        this->move_destination = destination;
        this->move_direction = this->move_destination - this->move_origin;
        this->move_length = this->move_direction.Length();
        this->move_direction = this->move_direction.Normalize();
        this->moving = true;
        this->move_timer.Start();
        this->move_speed = this->base_move_speed * speed;

        this->PlayAnimation("Armature|Walk", 1.0f, true);
    }
}