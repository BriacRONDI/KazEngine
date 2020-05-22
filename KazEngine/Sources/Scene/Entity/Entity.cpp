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

        if(this->last_ubo[frame_index] != this->properties) {
            DataBank::GetManagedBuffer().WriteData(&this->properties, Entity::ubo_size, offset + this->data_offset, frame_index);
            this->last_ubo[frame_index] = this->properties;
        }
    }

    Entity::Entity()
    {
        this->id = Entity::next_id;
        this->data_offset = this->id * Entity::ubo_size;
        this->properties.frame_id = 0;
        this->properties.selected = 0;
        this->moving = false;
        this->base_move_speed = 5.0f / 1000.0f;
        this->meshes = nullptr;
        this->last_ubo.resize(Vulkan::GetConcurrentFrameCount());
        for(auto& ubo : this->last_ubo) ubo.selected = true;

        Entity::next_id++;
    }

    Entity::~Entity()
    {
        if(this->meshes != nullptr)
            delete this->meshes;
    }

    Entity& Entity::operator=(Entity const& other)
    {
        if(&other != this) {
            this->animation = other.animation;
            this->animation_speed = other.animation_speed;
            this->animation_timer = other.animation_timer;
            this->base_move_speed = other.base_move_speed;
            this->data_offset = other.data_offset;
            this->id = other.id;
            this->loop_animation = other.loop_animation;
            this->move_destination = other.move_destination;
            this->move_direction = other.move_direction;
            this->move_length = other.move_length;
            this->move_origin = other.move_origin;
            this->move_speed = other.move_speed;
            this->move_timer = other.move_timer;
            this->moving = other.moving;
            this->properties = other.properties;

            if(other.meshes != nullptr) {
                this->meshes = new std::vector<std::shared_ptr<Model::Mesh>>;
                *this->meshes = *other.meshes;
            }else{
                this->meshes = nullptr;
            }
        }

        return *this;
    }

    Entity& Entity::operator=(Entity&& other)
    {
        if(&other != this) {
            this->animation = other.animation;
            this->animation_speed = other.animation_speed;
            this->animation_timer = other.animation_timer;
            this->base_move_speed = other.base_move_speed;
            this->data_offset = other.data_offset;
            this->id = other.id;
            this->loop_animation = other.loop_animation;
            this->move_destination = other.move_destination;
            this->move_direction = other.move_direction;
            this->move_length = other.move_length;
            this->move_origin = other.move_origin;
            this->move_speed = other.move_speed;
            this->move_timer = other.move_timer;
            this->moving = other.moving;
            this->properties = other.properties;
            this->meshes = other.meshes;
            other = {};
        }

        return *this;
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

    bool Entity::InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane)
    {
        if(this->meshes == nullptr) return false;

        for(auto& mesh : *this->meshes) {
            if(mesh->hit_box != nullptr) {
                Maths::Vector3 box_min = this->properties.matrix * mesh->hit_box->near_left_bottom_point;
                Maths::Vector3 box_max = this->properties.matrix * mesh->hit_box->far_right_top_point;
                if(Maths::aabb_inside_half_space(left_plane, box_min, box_max) && Maths::aabb_inside_half_space(right_plane, box_min, box_max)
                && Maths::aabb_inside_half_space(top_plane, box_min, box_max) && Maths::aabb_inside_half_space(bottom_plane, box_min, box_max))
                    return true;
            }
        }

        return false;
    }

    bool Entity::IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction)
    {
        if(this->meshes == nullptr) return false;

        for(auto& mesh : *this->meshes) {
            if(mesh->hit_box != nullptr) {
                if(Maths::ray_box_aabb_intersect(ray_origin, ray_direction,
                                                 this->properties.matrix * mesh->hit_box->near_left_bottom_point,
                                                 this->properties.matrix * mesh->hit_box->far_right_top_point,
                                                 -2000.0f, 2000.0f))
                    return true;
            }
        }

        return false;
    }
}