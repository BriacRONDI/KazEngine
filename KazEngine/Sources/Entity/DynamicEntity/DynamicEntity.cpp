#include "DynamicEntity.h"

namespace Engine
{
    DescriptorSet DynamicEntity::matrix_descriptor;
    DescriptorSet DynamicEntity::animation_descriptor;
    std::vector<DynamicEntity> DynamicEntity::entities;
    std::chrono::system_clock::time_point DynamicEntity::animation_time;

    bool DynamicEntity::Initialize()
    {
        if(!DynamicEntity::matrix_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Maths::Matrix4x4) * 10}
        }, Vulkan::GetConcurrentFrameCount())) return false;

        if(!DynamicEntity::animation_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(FRAME_DATA) * 10},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(ANIMATION_DATA) * 10},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t)}
        }, Vulkan::GetConcurrentFrameCount())) return false;

        DynamicEntity::animation_time = std::chrono::system_clock::now();

        return true;
    }

    void DynamicEntity::Clear()
    {
        DynamicEntity::matrix_descriptor.Clear();
        DynamicEntity::animation_descriptor.Clear();
    }

    DynamicEntity::DynamicEntity()
    {
        this->matrix_chunk = this->matrix_descriptor.ReserveRange(sizeof(Maths::Matrix4x4));
        if(this->matrix_chunk == nullptr) {
            if(!this->matrix_descriptor.ResizeChunk(this->matrix_descriptor.GetChunk()->range + sizeof(Maths::Matrix4x4) * 10, 0, Vulkan::SboAlignment())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "StaticEntity::StaticEntity() : Not enough memory" << std::endl;
                #endif
                return;
            }else{
                this->matrix_chunk = this->matrix_descriptor.ReserveRange(sizeof(Maths::Matrix4x4));
            }
        }

        this->frame_chunk = this->animation_descriptor.ReserveRange(sizeof(FRAME_DATA), DESCRIPTOR_BIND_FRAME);
        if(this->frame_chunk == nullptr) {
            if(!this->animation_descriptor.ResizeChunk(this->animation_descriptor.GetChunk(DESCRIPTOR_BIND_FRAME)->range + sizeof(FRAME_DATA) * 10, DESCRIPTOR_BIND_FRAME)) {
                #if defined(DISPLAY_LOGS)
                std::cout << "StaticEntity::StaticEntity() : Not enough memory" << std::endl;
                #endif
                return;
            }else{
                this->frame_chunk = this->animation_descriptor.ReserveRange(sizeof(FRAME_DATA), DESCRIPTOR_BIND_FRAME);
            }
        }

        this->animation_chunk = this->animation_descriptor.ReserveRange(sizeof(ANIMATION_DATA), DESCRIPTOR_BIND_ANIMATION);
        if(this->animation_chunk == nullptr) {
            if(!this->animation_descriptor.ResizeChunk(this->animation_descriptor.GetChunk(DESCRIPTOR_BIND_ANIMATION)->range + sizeof(ANIMATION_DATA) * 10, DESCRIPTOR_BIND_ANIMATION)) {
                #if defined(DISPLAY_LOGS)
                std::cout << "StaticEntity::StaticEntity() : Not enough memory" << std::endl;
                #endif
                return;
            }else{
                this->animation_chunk = this->animation_descriptor.ReserveRange(sizeof(ANIMATION_DATA), DESCRIPTOR_BIND_ANIMATION);
            }
        }

        DynamicEntity::matrix_descriptor.WriteData(&this->matrix, sizeof(Maths::Matrix4x4), this->matrix_chunk->offset);
    }

    void DynamicEntity::SetMatrix(Maths::Matrix4x4 matrix)
    {
        this->matrix = matrix;
        DynamicEntity::matrix_descriptor.WriteData(&this->matrix, sizeof(Maths::Matrix4x4), this->matrix_chunk->offset);
    }

    void DynamicEntity::UpdateAnimationTimer()
    {
        uint32_t time = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - DynamicEntity::animation_time).count()
        );

        DynamicEntity::animation_descriptor.WriteData(&time, sizeof(uint32_t), DESCRIPTOR_BIND_TIME);
    }

    void DynamicEntity::PlayAnimation(std::string animation, float speed, bool loop)
    {
        if(DataBank::HasAnimation(animation)) {
            // this->animation = animation;
            this->animation.speed = speed;

            this->frame.frame_id = 0;
            this->frame.animation_id = DataBank::GetAnimation(animation).animation_id;
            this->animation.frame_count = DataBank::GetAnimation(animation).frame_count;
            this->animation.duration = static_cast<uint32_t>(DataBank::GetAnimation(animation).duration.count());

            this->animation.loop = loop ? 1 : 0;
            this->animation.play = 1;

            this->animation.start = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - DynamicEntity::animation_time).count()
            );

            DynamicEntity::animation_descriptor.WriteData(&this->animation, sizeof(ANIMATION_DATA),
                                                          static_cast<uint32_t>(this->animation_chunk->offset),
                                                          DESCRIPTOR_BIND_ANIMATION);
        }
    }

    bool DynamicEntity::InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane)
    {
        for(auto& mesh : this->meshes) {
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

    bool DynamicEntity::IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction)
    {
        for(auto& mesh : this->meshes) {
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

    DynamicEntity* DynamicEntity::ToggleSelection(Point<uint32_t> mouse_position)
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

        DynamicEntity* selected_entity = nullptr;
        for(auto& entity : DynamicEntity::entities) {
            if(selected_entity != nullptr) {
                entity.selected = false;
            }else if(entity.IntersectRay(camera_position, mouse_ray)) {
                selected_entity = &entity;
                entity.selected = true;
            }else {
                entity.selected = false;
            }
        }

        return selected_entity;
    }

    std::vector<DynamicEntity*> DynamicEntity::SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end)
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

        std::vector<DynamicEntity*> return_value;
        for(auto& entity : DynamicEntity::entities) {
            if(entity.InSelectBox(left_plane, right_plane, top_plane, bottom_plane)) {
                entity.selected = true;
                return_value.push_back(&entity);
            }else{
                entity.selected = false;
            }
        }

        return return_value;
    }
}