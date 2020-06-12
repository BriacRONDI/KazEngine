#include "Entity.h"
#include "SkeletonEntity.h"

namespace Engine
{
    uint32_t Entity::next_id = 0;
    std::shared_ptr<Chunk> Entity::static_data_chunk;

    Entity::Entity(bool pick_chunk)
    {
        this->id = Entity::next_id;
        this->selected = false;
        this->meshes = nullptr;
        if(pick_chunk) this->PickChunk();
        Entity::next_id++;
    }

    bool Entity::InitilizeInstanceChunk()
    {
        if(Entity::static_data_chunk == nullptr) {
            Entity::static_data_chunk = DataBank::GetManagedBuffer().ReserveChunk(0, Vulkan::SboAlignment());
            if(Entity::static_data_chunk == nullptr) return false;
        }
        return true;
    }

    bool Entity::PickChunk()
    {
        if(this->static_instance_chunk != nullptr) return true;
        this->static_instance_chunk = Entity::static_data_chunk->ReserveRange(sizeof(Maths::Matrix4x4), Vulkan::SboAlignment());
        if(this->static_instance_chunk == nullptr) {
            bool relocated;
            if(!DataBank::GetManagedBuffer().ResizeChunk(Entity::static_data_chunk,
                                                         Entity::static_data_chunk->range + sizeof(Maths::Matrix4x4),
                                                         relocated, Vulkan::SboAlignment())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Entity::SetupChunk : Not enough memory" << std::endl;
                #endif
                return false;
            }

            if(relocated) {
                SkeletonEntity::absolute_skeleton_data_chunk->offset = Entity::static_data_chunk->offset
                                                                     + SkeletonEntity::skeleton_data_chunk->offset;
            }

            this->static_instance_chunk = Entity::static_data_chunk->ReserveRange(sizeof(Maths::Matrix4x4), Vulkan::SboAlignment());

            for(auto& listener : this->Listeners)
                listener->StaticBufferUpdated();
        }

        return this->static_instance_chunk != nullptr;
    }

    Entity::~Entity()
    {
        if(this->meshes != nullptr)
            delete this->meshes;
    }

    void Entity::Update(uint32_t frame_index)
    {
        DataBank::GetManagedBuffer().WriteData(&this->matrix, sizeof(Maths::Matrix4x4),
                                               Entity::static_data_chunk->offset + this->static_instance_chunk->offset,
                                               frame_index);
    }

    Entity& Entity::operator=(Entity const& other)
    {
        if(&other != this) {
            this->static_instance_chunk = other.static_instance_chunk;
            this->id = other.id;
            this->matrix = other.matrix;
            this->selected = other.selected;

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
            this->static_instance_chunk = std::move(other.static_instance_chunk);
            this->id = other.id;
            this->matrix = std::move(other.matrix);
            this->meshes = std::move(other.meshes);
            this->selected = other.selected;
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
                Maths::Vector3 box_min = this->matrix * mesh->hit_box->near_left_bottom_point;
                Maths::Vector3 box_max = this->matrix * mesh->hit_box->far_right_top_point;
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
                                                 this->matrix * mesh->hit_box->near_left_bottom_point,
                                                 this->matrix * mesh->hit_box->far_right_top_point,
                                                 -2000.0f, 2000.0f))
                    return true;
            }
        }

        return false;
    }
}