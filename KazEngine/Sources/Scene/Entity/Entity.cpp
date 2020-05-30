#include "Entity.h"

namespace Engine
{
    uint32_t Entity::next_id = 0;
    std::shared_ptr<Chunk> Entity::static_instance_chunk;

    Entity::Entity()
    {
        this->id = Entity::next_id;
        this->properties.selected = 0;
        this->meshes = nullptr;
        this->last_state.resize(Vulkan::GetConcurrentFrameCount());
        for(auto& state : this->last_state) state.selected = true;

        Entity::next_id++;
    }

    bool Entity::AddToScene()
    {
        if(this->chunk != nullptr) return true;
        this->chunk = Entity::static_instance_chunk->ReserveRange(this->properties.Size());
        if(this->chunk == nullptr) {
            bool relocated;
            if(!DataBank::GetManagedBuffer().ResizeChunk(Entity::static_instance_chunk, Entity::static_instance_chunk->range + this->properties.Size() * 15, relocated)) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Entity::SetupChunk : Not enough memory" << std::endl;
                #endif
            }
            this->chunk = Entity::static_instance_chunk->ReserveRange(this->properties.Size());
        }

        return this->chunk != nullptr;
    }

    Entity::~Entity()
    {
        if(this->meshes != nullptr)
            delete this->meshes;
    }

    void Entity::Update(uint32_t frame_index)
    {
        if(this->last_state[frame_index] != this->properties) {
            this->properties.Write(Entity::static_instance_chunk->offset + this->chunk->offset, frame_index);
            this->last_state[frame_index] = this->properties;
        }
    }

    Entity& Entity::operator=(Entity const& other)
    {
        if(&other != this) {
            this->chunk = other.chunk;
            this->id = other.id;
            this->last_state = other.last_state;
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
            this->chunk = other.chunk;
            this->id = other.id;
            this->last_state = other.last_state;
            this->properties = other.properties;
            this->meshes = other.meshes;

            other.meshes = nullptr;
            other.chunk = nullptr;
        }

        return *this;
    }

    void Entity::AddMesh(std::shared_ptr<Model::Mesh> mesh)
    {
        if(this->meshes == nullptr) this->meshes = new std::vector<std::shared_ptr<Model::Mesh>>;
        this->meshes->push_back(mesh);
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