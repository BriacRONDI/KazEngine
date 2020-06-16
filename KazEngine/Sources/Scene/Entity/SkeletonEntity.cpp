#include "SkeletonEntity.h"

namespace Engine
{
    std::shared_ptr<Chunk> SkeletonEntity::skeleton_data_chunk;
    std::shared_ptr<Chunk> SkeletonEntity::absolute_skeleton_data_chunk;
    std::shared_ptr<Chunk> SkeletonEntity::animation_data_chunk;

    SkeletonEntity::SkeletonEntity() : Entity(false)
    {
        this->animation_properties.frame_id = 0;
        this->animation_properties.animation_id = 0;
        this->moving = false;
        this->base_move_speed = 5.0f / 1000.0f;
        this->PickChunk();

        this->last_animation_state.resize(Vulkan::GetConcurrentFrameCount());
        for(auto& state : this->last_animation_state) state.animation_id = UINT32_MAX;
    }

    bool SkeletonEntity::InitilizeInstanceChunk()
    {
        //Entity::InitilizeInstanceChunk();
        if(SkeletonEntity::skeleton_data_chunk == nullptr) {
            SkeletonEntity::skeleton_data_chunk = Entity::descriptor->ReserveRange(0, Vulkan::SboAlignment());
            if(SkeletonEntity::skeleton_data_chunk == nullptr) return false;
        }

        if(SkeletonEntity::animation_data_chunk == nullptr) {
            SkeletonEntity::animation_data_chunk = DataBank::GetManagedBuffer().ReserveChunk(0);
            if(SkeletonEntity::animation_data_chunk == nullptr) return false;
        }

        if(SkeletonEntity::absolute_skeleton_data_chunk == nullptr) {
            SkeletonEntity::absolute_skeleton_data_chunk = std::shared_ptr<Chunk>(new Chunk);
        }

        return true;
    }

    bool SkeletonEntity::PickChunk()
    {
        if(this->static_instance_chunk == nullptr) {
            this->static_instance_chunk = SkeletonEntity::skeleton_data_chunk->ReserveRange(sizeof(Maths::Matrix4x4), Vulkan::SboAlignment());
            if(this->static_instance_chunk == nullptr) {
                if(!Entity::descriptor->ResizeChunk(SkeletonEntity::skeleton_data_chunk,
                                                    SkeletonEntity::skeleton_data_chunk->range + sizeof(Maths::Matrix4x4),
                                                    0, Vulkan::SboAlignment())) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "SkeletonEntity::PickChunk() => Not enough memory" << std::endl;
                    return false;
                    #endif
                    /*if(!DataBank::GetManagedBuffer().ResizeChunk(Entity::static_data_chunk,
                                                                 Entity::static_data_chunk->range + sizeof(Maths::Matrix4x4),
                                                                 relocated, Vulkan::SboAlignment())) {
                        #if defined(DISPLAY_LOGS)
                        std::cout << "SkeletonEntity::PickChunk() => Not enough memory" << std::endl;
                        return false;
                        #endif
                    }else{
                        Entity::static_data_chunk->ResizeChild(SkeletonEntity::skeleton_data_chunk,
                                                                SkeletonEntity::skeleton_data_chunk->range + sizeof(Maths::Matrix4x4),
                                                                relocated, Vulkan::SboAlignment());
                    }*/
                }
                this->static_instance_chunk = SkeletonEntity::skeleton_data_chunk->ReserveRange(sizeof(Maths::Matrix4x4), Vulkan::SboAlignment());
            }
            SkeletonEntity::absolute_skeleton_data_chunk->offset = Entity::descriptor->GetChunk()->offset + SkeletonEntity::skeleton_data_chunk->offset;
            SkeletonEntity::absolute_skeleton_data_chunk->range = SkeletonEntity::skeleton_data_chunk->range;

            /*for(auto& listener : this->Listeners)
                listener->StaticBufferUpdated();*/
        }

        if(this->animation_instance_chunk == nullptr) {
            this->animation_instance_chunk = SkeletonEntity::animation_data_chunk->ReserveRange(sizeof(SKELETON_DATA));
            if(this->animation_instance_chunk == nullptr) {
                bool relocated;
                if(!DataBank::GetManagedBuffer().ResizeChunk(SkeletonEntity::animation_data_chunk,
                                                             SkeletonEntity::animation_data_chunk->range + sizeof(SKELETON_DATA),
                                                             relocated)) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "Entity::PickChunk() : Not enough memory" << std::endl;
                    #endif
                }
                this->animation_instance_chunk = SkeletonEntity::animation_data_chunk->ReserveRange(sizeof(SKELETON_DATA));
            }
        }

        return this->static_instance_chunk != nullptr && this->animation_instance_chunk != nullptr;
    }

    void SkeletonEntity::Update(uint32_t frame_index)
    {
        // Entity::Update(frame_index);

        if(!this->animation.empty()) {
            uint32_t last_frame_id = DataBank::GetAnimation(this->animation).frame_count - 1;
            float progression = this->animation_timer.GetProgression();
            if(this->loop_animation) {
                this->animation_properties.frame_id = static_cast<uint32_t>(progression * last_frame_id) % last_frame_id;
            }else{
                this->animation_properties.frame_id = last_frame_id;
                this->animation.clear();
            }
        }

        if(this->moving) {
            Maths::Vector3 new_position = this->move_origin + this->move_direction * this->move_speed * static_cast<float>(this->move_timer.GetDuration().count());
            if((new_position - this->move_origin).Length() >= this->move_length) {
                this->moving = false;
                this->matrix.SetTranslation(this->move_destination);
            }else{
                this->matrix.SetTranslation(new_position);
            }
        }

        Entity::descriptor->WriteData(&this->matrix, sizeof(Maths::Matrix4x4), 0, frame_index,
            static_cast<uint32_t>(SkeletonEntity::skeleton_data_chunk->offset + this->static_instance_chunk->offset));

        if(this->last_animation_state[frame_index] != this->animation_properties) {
            DataBank::GetManagedBuffer().WriteData(&this->animation_properties, sizeof(SKELETON_DATA),
                                                   SkeletonEntity::animation_data_chunk->offset
                                                        + this->animation_instance_chunk->offset,
                                                   frame_index);
            this->last_animation_state[frame_index] = this->animation_properties;
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

                this->animation_instance_chunk = skull->animation_instance_chunk;
                this->last_animation_state = skull->last_animation_state;
                this->animation_properties = skull->animation_properties;
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
                this->animation = std::move(skull->animation);
                this->animation_speed = skull->animation_speed;
                this->animation_timer = skull->animation_timer;
                this->base_move_speed = skull->base_move_speed;
                this->loop_animation = skull->loop_animation;
                this->move_destination = std::move(skull->move_destination);
                this->move_direction = std::move(skull->move_direction);
                this->move_length = skull->move_length;
                this->move_origin = std::move(skull->move_origin);
                this->move_speed = skull->move_speed;
                this->move_timer = skull->move_timer;
                this->moving = skull->moving;

                this->animation_instance_chunk = std::move(skull->animation_instance_chunk);
                this->last_animation_state = std::move(skull->last_animation_state);
                this->animation_properties = std::move(skull->animation_properties);
            }
        }

        return *this;
    }

    void SkeletonEntity::PlayAnimation(std::string animation, float speed, bool loop)
    {
        if(DataBank::HasAnimation(animation)) {
            this->animation = animation;
            this->animation_properties.animation_id = DataBank::GetAnimation(animation).animation_id;

            if(speed == 1.0f) this->animation_timer.Start(DataBank::GetAnimation(animation).duration);
            else this->animation_timer.Start(std::chrono::milliseconds(static_cast<uint64_t>(DataBank::GetAnimation(animation).duration.count() / speed)));
            
            this->loop_animation = loop;
            this->animation_speed = speed;
        }
    }

    void SkeletonEntity::MoveTo(Maths::Vector3 destination, float speed)
    {
        this->move_origin = this->matrix.GetTranslation();
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