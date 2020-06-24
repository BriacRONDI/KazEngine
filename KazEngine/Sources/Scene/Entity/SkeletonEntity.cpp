#include "SkeletonEntity.h"

namespace Engine
{
    std::shared_ptr<Chunk> SkeletonEntity::skeleton_data_chunk;
    std::shared_ptr<Chunk> SkeletonEntity::absolute_skeleton_data_chunk;
    std::shared_ptr<Chunk> SkeletonEntity::animation_data_chunk;
    DescriptorSet* SkeletonEntity::animation_descriptor;
    std::chrono::system_clock::time_point SkeletonEntity::animation_time = std::chrono::system_clock::now();

    SkeletonEntity::SkeletonEntity() : Entity(true)
    {
        this->id = Entity::next_id;
        this->selected = false;
        this->meshes = nullptr;

        this->frame.frame_id = 0;
        this->frame.animation_id = 0;

        this->animation_instance.loop = 0;
        this->animation_instance.play = 0;
        this->animation_instance.speed = 0.0f;
        this->animation_instance.duration = 0;
        this->animation_instance.start = 0;
        this->animation_instance.frame_count = 0;

        this->moving = false;
        this->base_move_speed = 5.0f / 1000.0f;
        this->PickChunk();

        Entity::next_id++;
        Entity::entities.push_back(this);

        for(auto& listener : Entity::Listeners)
            listener->NewEntity(*this);
    }

    void SkeletonEntity::UpdateAnimationTimer()
    {
        uint32_t time = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - SkeletonEntity::animation_time).count()
        );

        SkeletonEntity::animation_descriptor->WriteData(&time, sizeof(uint32_t), DESCRIPTOR_BIND_TIME);
    }

    bool SkeletonEntity::Initialize()
    {
        if(SkeletonEntity::animation_descriptor == nullptr) {
            SkeletonEntity::animation_descriptor = new DescriptorSet;
            if(!SkeletonEntity::animation_descriptor->Create({
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(FRAME_DATA) * 10},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(ANIMATION_DATA) * 10},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t)}
            }, DataBank::GetManagedBuffer().GetInstanceCount())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "SkeletonEntity::Initialize() : Not enough memory" << std::endl;
                #endif
                return false;
            }
        }

        if(SkeletonEntity::skeleton_data_chunk == nullptr) {
            SkeletonEntity::skeleton_data_chunk = Entity::descriptor->ReserveRange(0, Vulkan::SboAlignment());
            if(SkeletonEntity::skeleton_data_chunk == nullptr) return false;
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

            #if defined(DISPLAY_LOGS)
            size_t mem_offset = SkeletonEntity::absolute_skeleton_data_chunk->offset;
            #endif

            if(this->static_instance_chunk == nullptr) {

                if(!Entity::descriptor->ResizeChunk(SkeletonEntity::skeleton_data_chunk,
                                                    SkeletonEntity::skeleton_data_chunk->range + sizeof(Maths::Matrix4x4),
                                                    0, Vulkan::SboAlignment())) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "SkeletonEntity::PickChunk() => Not enough memory" << std::endl;
                    return false;
                    #endif
                }

                this->static_instance_chunk = SkeletonEntity::skeleton_data_chunk->ReserveRange(sizeof(Maths::Matrix4x4), Vulkan::SboAlignment());
            }
            SkeletonEntity::absolute_skeleton_data_chunk->offset = Entity::descriptor->GetChunk()->offset + SkeletonEntity::skeleton_data_chunk->offset;
            SkeletonEntity::absolute_skeleton_data_chunk->range = SkeletonEntity::skeleton_data_chunk->range;

            Entity::descriptor->WriteData(&this->matrix, sizeof(Maths::Matrix4x4), 0, this->GetStaticDataOffset());
        }

        if(this->frame_chunk == nullptr) {
            this->frame_chunk = SkeletonEntity::animation_descriptor->ReserveRange(sizeof(FRAME_DATA), DESCRIPTOR_BIND_FRAME);
            if(this->frame_chunk == nullptr) {
                if(!SkeletonEntity::animation_descriptor->ResizeChunk(SkeletonEntity::animation_descriptor->GetChunk(DESCRIPTOR_BIND_FRAME)->range
                                                                                + sizeof(FRAME_DATA),
                                                                      DESCRIPTOR_BIND_FRAME, Vulkan::SboAlignment())) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "SkeletonEntity::PickChunk() : Not enough memory" << std::endl;
                    #endif
                }
                this->frame_chunk = SkeletonEntity::animation_descriptor->ReserveRange(sizeof(FRAME_DATA), DESCRIPTOR_BIND_FRAME);
            }

            SkeletonEntity::animation_descriptor->WriteData(&this->frame, sizeof(FRAME_DATA), DESCRIPTOR_BIND_FRAME,
                                                            static_cast<uint32_t>(this->frame_chunk->offset));
        }

        if(this->animation_chunk == nullptr) {
            this->animation_chunk = SkeletonEntity::animation_descriptor->ReserveRange(sizeof(ANIMATION_DATA), DESCRIPTOR_BIND_ANIMATION);
            if(this->animation_chunk == nullptr) {
                if(!SkeletonEntity::animation_descriptor->ResizeChunk(SkeletonEntity::animation_descriptor->GetChunk(DESCRIPTOR_BIND_ANIMATION)->range
                                                                                + sizeof(ANIMATION_DATA),
                                                                      DESCRIPTOR_BIND_ANIMATION, Vulkan::SboAlignment())) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "SkeletonEntity::PickChunk() : Not enough memory" << std::endl;
                    #endif
                }
                this->animation_chunk = SkeletonEntity::animation_descriptor->ReserveRange(sizeof(ANIMATION_DATA), DESCRIPTOR_BIND_ANIMATION);
            }

            SkeletonEntity::animation_descriptor->WriteData(&this->animation_instance, sizeof(ANIMATION_DATA), DESCRIPTOR_BIND_ANIMATION,
                                                            static_cast<uint32_t>(this->animation_chunk->offset));
        }

        return this->static_instance_chunk != nullptr && this->frame_chunk != nullptr;
    }

    void SkeletonEntity::Clear()
    {
        if(SkeletonEntity::animation_descriptor != nullptr) {
            delete SkeletonEntity::animation_descriptor;
            SkeletonEntity::animation_descriptor = nullptr;
        }
    }

    void SkeletonEntity::Update(uint32_t frame_index)
    {
        if(this->moving) {
            Maths::Vector3 new_position = this->move_origin + this->move_direction * this->move_speed * static_cast<float>(this->move_timer.GetDuration().count());
            if((new_position - this->move_origin).Length() >= this->move_length) {
                this->moving = false;
                this->matrix.SetTranslation(this->move_destination);
            }else{
                this->matrix.SetTranslation(new_position);
            }
        }

        Entity::Update(frame_index);
    }

    Entity& SkeletonEntity::operator=(Entity const& other)
    {
        if(&other != this) {

            Entity::operator=(other);
            SkeletonEntity* skull = dynamic_cast<SkeletonEntity*>(const_cast<Entity*>(&other));

            if(skull != nullptr) {
                this->animation = skull->animation;
                this->base_move_speed = skull->base_move_speed;
                this->move_destination = skull->move_destination;
                this->move_direction = skull->move_direction;
                this->move_length = skull->move_length;
                this->move_origin = skull->move_origin;
                this->move_speed = skull->move_speed;
                this->move_timer = skull->move_timer;
                this->moving = skull->moving;

                this->frame_chunk = skull->frame_chunk;
                this->animation_chunk = skull->animation_chunk;
                this->frame = skull->frame;
                this->animation_instance = skull->animation_instance;
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
                this->base_move_speed = skull->base_move_speed;
                this->move_destination = std::move(skull->move_destination);
                this->move_direction = std::move(skull->move_direction);
                this->move_length = skull->move_length;
                this->move_origin = std::move(skull->move_origin);
                this->move_speed = skull->move_speed;
                this->move_timer = skull->move_timer;
                this->moving = skull->moving;

                this->frame_chunk = std::move(skull->frame_chunk);
                this->animation_chunk = std::move(skull->animation_chunk);
                this->frame = std::move(skull->frame);
                this->animation_instance = std::move(skull->animation_instance);
            }
        }

        return *this;
    }

    void SkeletonEntity::PlayAnimation(std::string animation, float speed, bool loop)
    {
        if(DataBank::HasAnimation(animation)) {
            this->animation = animation;
            this->animation_instance.speed = speed;

            this->frame.frame_id = 0;
            this->frame.animation_id = DataBank::GetAnimation(animation).animation_id;
            this->animation_instance.frame_count = DataBank::GetAnimation(this->animation).frame_count;
            this->animation_instance.duration = static_cast<uint32_t>(DataBank::GetAnimation(animation).duration.count());

            this->animation_instance.loop = loop ? 1 : 0;
            this->animation_instance.play = 1;

            this->animation_instance.start = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - SkeletonEntity::animation_time).count()
            );

            SkeletonEntity::animation_descriptor->WriteData(&this->animation_instance, sizeof(ANIMATION_DATA), DESCRIPTOR_BIND_ANIMATION,
                                                            static_cast<uint32_t>(this->animation_chunk->offset));
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