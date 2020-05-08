#include "Entity.h"

namespace Engine
{
    uint32_t Entity::next_id = 0;
    uint32_t Entity::ubo_size = 0;

    void Entity::Update(VkDeviceSize offset, uint32_t frame_index)
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

        DataBank::GetManagedBuffer().WriteData(&this->properties, Entity::ubo_size, offset + this->data_offset, frame_index);
    }

    Entity::Entity()
    {
        this->id = Entity::next_id;
        this->data_offset = this->id * Entity::ubo_size;
        this->properties.frame_id = 0;
        this->moving = false;
        this->base_move_speed = 5.0f / 1000.0f;

        Entity::next_id++;
    }

    Entity::~Entity()
    {
        if(this->meshes != nullptr) delete this->meshes;
    }

    void Entity::AddMesh(std::shared_ptr<Model::Mesh> mesh)
    {
        if(this->meshes == nullptr) this->meshes = new std::vector<std::shared_ptr<Model::Mesh>>;
        this->meshes->push_back(mesh);
    }

    void Entity::PlayAnimation(std::string animation, float speed, bool loop)
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

    void Entity::MoveTo(Maths::Vector3 destination, float speed)
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