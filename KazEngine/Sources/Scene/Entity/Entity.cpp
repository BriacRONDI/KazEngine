#include "Entity.h"
#include "SkeletonEntity.h"

namespace Engine
{
    uint32_t Entity::next_id = 0;
    DescriptorSet* Entity::descriptor = nullptr;
    std::vector<Entity*> Entity::entities;

    Entity::Entity()
    {

        this->id = Entity::next_id;
        this->selected = false;
        this->meshes = nullptr;
        this->PickChunk();
        Entity::next_id++;
        Entity::entities.push_back(this);

        for(auto& listener : Entity::Listeners)
            listener->NewEntity(*this);
    }

    void Entity::Clear()
    {
        Entity::entities.clear();

        if(Entity::descriptor != nullptr) {
            delete Entity::descriptor;
            Entity::descriptor = nullptr;
        }

        Entity::next_id = 0;
    }

    bool Entity::Initialize()
    {
        if(Entity::descriptor == nullptr) {
            Entity::descriptor = new DescriptorSet;
            if(!Entity::descriptor->Create({
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Maths::Matrix4x4) * 10}
            }, DataBank::GetManagedBuffer().GetInstanceCount())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Entity::Initialize() : Not enough memory" << std::endl;
                #endif
                return false;
            }
        }

        return true;
    }

    bool Entity::PickChunk()
    {
        if(this->static_instance_chunk != nullptr) return true;
        this->static_instance_chunk = Entity::descriptor->ReserveRange(sizeof(Maths::Matrix4x4), Vulkan::SboAlignment());
        if(this->static_instance_chunk == nullptr) {
            if(!Entity::descriptor->ResizeChunk(this->static_instance_chunk,
                                                this->static_instance_chunk->range + sizeof(Maths::Matrix4x4),
                                                0, Vulkan::SboAlignment())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Entity::SetupChunk : Not enough memory" << std::endl;
                #endif
                return false;
            }

            SkeletonEntity::absolute_skeleton_data_chunk->offset = Entity::descriptor->GetChunk()->offset
                                                                 + SkeletonEntity::skeleton_data_chunk->offset;

            this->static_instance_chunk = Entity::descriptor->ReserveRange(sizeof(Maths::Matrix4x4), Vulkan::SboAlignment());
            Entity::descriptor->WriteData(&this->matrix, sizeof(Maths::Matrix4x4), 0, this->GetStaticDataOffset());
        }

        return this->static_instance_chunk != nullptr;
    }

    Entity::~Entity()
    {
        if(this->meshes != nullptr)
            delete this->meshes;

        for(auto it = Entity::entities.begin(); it!=Entity::entities.end(); it++) {
            if(*it == this) {
                Entity::entities.erase(it);
                break;
            }
        }
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

        for(auto& listener : Entity::Listeners)
            listener->AddMesh(*this, mesh);
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

    Entity* Entity::ToggleSelection(Point<uint32_t> mouse_position)
    {
        auto& camera = Engine::Camera::GetInstance();
        Area<float> const& near_plane_size = camera.GetFrustum().GetNearPlaneSize();
        Maths::Vector3 camera_position = -camera.GetUniformBuffer().position;
        Surface& draw_surface = Engine::Vulkan::GetDrawSurface();
        Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

        Point<float> real_mouse = {
            static_cast<float>(mouse_position.X) - float_draw_surface.Width / 2.0f,
            static_cast<float>(mouse_position.Y) - float_draw_surface.Height / 2.0f
        };

        real_mouse.X /= float_draw_surface.Width / 2.0f;
        real_mouse.Y /= float_draw_surface.Height / 2.0f;

        Maths::Vector3 mouse_world_position = camera_position + camera.GetFrontVector() * camera.GetNearClipDistance() + camera.GetRightVector()
                                            * near_plane_size.Width * real_mouse.X - camera.GetUpVector() * near_plane_size.Height * real_mouse.Y;
        Maths::Vector3 mouse_ray = mouse_world_position - camera_position;
        mouse_ray = mouse_ray.Normalize();

        Entity* selected_entity = nullptr;
        for(Entity* entity : Entity::entities) {
            if(selected_entity != nullptr) {
                entity->selected = VK_FALSE;
            }else if(entity->IntersectRay(camera_position, mouse_ray)) {
                selected_entity = entity;
                entity->selected = VK_TRUE;
            }else {
                entity->selected = VK_FALSE;
            }
        }

        return selected_entity;
    }

    std::vector<Entity*> Entity::SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end)
    {
        auto& camera = Engine::Camera::GetInstance();
        Area<float> const& near_plane_size = camera.GetFrustum().GetNearPlaneSize();
        Maths::Vector3 camera_position = -camera.GetUniformBuffer().position;
        Surface& draw_surface = Engine::Vulkan::GetDrawSurface();
        Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

        float x1 = static_cast<float>(box_start.X) - float_draw_surface.Width / 2.0f;
        float x2 = static_cast<float>(box_end.X) - float_draw_surface.Width / 2.0f;

        float y1 = static_cast<float>(box_start.Y) - float_draw_surface.Height / 2.0f;
        float y2 = static_cast<float>(box_end.Y) - float_draw_surface.Height / 2.0f;

        x1 /= float_draw_surface.Width / 2.0f;
        x2 /= float_draw_surface.Width / 2.0f;
        y1 /= float_draw_surface.Height / 2.0f;
        y2 /= float_draw_surface.Height / 2.0f;

        float left_x = std::min<float>(x1, x2);
        float right_x = std::max<float>(x1, x2);
        float top_y = std::min<float>(y1, y2);
        float bottom_y = std::max<float>(y1, y2);

        Maths::Vector3 base_near = camera_position + camera.GetFrontVector() * camera.GetNearClipDistance();
        Maths::Vector3 base_with = camera.GetRightVector() * near_plane_size.Width;
        Maths::Vector3 base_height = camera.GetUpVector() * near_plane_size.Height;

        Maths::Vector3 top_left_position = base_near + base_with * left_x - base_height * top_y;
        Maths::Vector3 bottom_right_position = base_near + base_with * right_x - base_height * bottom_y;

        Maths::Vector3 top_left_ray = (top_left_position - camera_position).Normalize();
        Maths::Vector3 bottom_right_ray = (bottom_right_position - camera_position).Normalize();

        Maths::Plane left_plane = {top_left_position, top_left_ray.Cross(-camera.GetUpVector())};
        Maths::Plane right_plane = {bottom_right_position, bottom_right_ray.Cross(camera.GetUpVector())};
        Maths::Plane top_plane = {top_left_position, top_left_ray.Cross(-camera.GetRightVector())};
        Maths::Plane bottom_plane = {bottom_right_position, bottom_right_ray.Cross(camera.GetRightVector())};

        std::vector<Entity*> return_value;
        for(Entity* entity : Entity::entities) {
            if(entity->InSelectBox(left_plane, right_plane, top_plane, bottom_plane)) {
                entity->selected = VK_TRUE;
                return_value.push_back(entity);
            }else{
                entity->selected = VK_FALSE;
            }
        }

        return return_value;
    }

    std::vector<Chunk> Entity::UpdateData(uint8_t instance_id)
    {
        Chunk chunk = {
            Entity::descriptor->GetChunk()->offset + this->GetStaticDataOffset(),
            sizeof(Maths::Matrix4x4)
        };

        Maths::Matrix4x4* last_state = reinterpret_cast<Maths::Matrix4x4*>(DataBank::GetManagedBuffer().GetStagingBuffer(instance_id).pointer + chunk.offset);
        if(*last_state != this->matrix) {
            *last_state = this->matrix;
            return {chunk};
        }else{
            return {};
        }
    }

    void Entity::SetMatrix(Maths::Matrix4x4 matrix)
    {
        this->matrix = matrix;
        Entity::descriptor->WriteData(&this->matrix, sizeof(Maths::Matrix4x4), 0, this->GetStaticDataOffset());
    }

    void Entity::UpdateAll(uint32_t frame_index)
    {
        std::vector<Chunk> chunks;
        for(auto entity : Entity::entities) {
            entity->Update(frame_index);
            auto update = entity->UpdateData(frame_index);
            if(!update.empty()) chunks.insert(chunks.end(), update.begin(), update.end());
        }

        std::sort(chunks.begin(), chunks.end(), [](Chunk a, Chunk b){ return a.offset < b.offset; });

        auto current = chunks.begin();
        while(current != chunks.end()) {
            auto next = current + 1;
            if(next != chunks.end()) {
                if(current->offset + current->range == next->offset) {
                    current->range += next->range;
                    chunks.erase(next);
                }else{
                    current++;
                }
            }else{
                break;
            }
        }

        for(auto chunk : chunks)
            DataBank::GetManagedBuffer().UpdateFlushRange(chunk.offset, chunk.range, frame_index);
    }
}