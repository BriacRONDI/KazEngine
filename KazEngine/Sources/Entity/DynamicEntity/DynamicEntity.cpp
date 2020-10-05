#include "DynamicEntity.h"
#include "../IEntityListener.h"

namespace Engine
{
    Maths::Matrix4x4* DynamicEntity::matrix_array = nullptr;
    /*DynamicEntity::MOVEMENT_DATA* DynamicEntity::movement_array = nullptr;
    DynamicEntity::MOVEMENT_GROUP* DynamicEntity::groups_array = nullptr;*/
    // uint32_t DynamicEntity::movement_group_count = 0;
    DescriptorSet DynamicEntity::matrix_descriptor;
    DescriptorSet DynamicEntity::animation_descriptor;
    DescriptorSet DynamicEntity::selection_descriptor;
    std::vector<DynamicEntity*> DynamicEntity::entities;
    //DescriptorSet DynamicEntity::movement_descriptor;

    bool DynamicEntity::Initialize()
    {
        if(!DynamicEntity::matrix_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Maths::Matrix4x4) * 10}
        }, false)) return false;
        DynamicEntity::matrix_array = reinterpret_cast<Maths::Matrix4x4*>(DynamicEntity::matrix_descriptor.DirectStagingAccess());

        /*if(!DynamicEntity::movement_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MOVEMENT_DATA) * 10},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t)},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MOVEMENT_GROUP) * 10}
        }, false)) return false;*/

        if(!DynamicEntity::animation_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(FRAME_DATA) * 10},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(ANIMATION_DATA) * 10}
        })) return false;

        if(!DynamicEntity::selection_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t) * 101},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(Maths::Vector4) * 8}
        })) return false;

        for(uint8_t i=0; i<Vulkan::GetConcurrentFrameCount(); i++) DynamicEntity::Update(i);

        /*DynamicEntity::movement_group_count = 0;
        DynamicEntity::movement_descriptor.WriteData(&DynamicEntity::movement_group_count, sizeof(uint32_t), 0, DESCRIPTOR_BIND_GROUP_COUNT);*/

        return true;
    }

    void DynamicEntity::Clear()
    {
        DynamicEntity::matrix_descriptor.Clear();
        DynamicEntity::animation_descriptor.Clear();
        // DynamicEntity::movement_descriptor.Clear();
        DynamicEntity::selection_descriptor.Clear();
    }

    DynamicEntity::DynamicEntity(Maths::Matrix4x4 matrix)
    {
        this->selected = false;
        this->animation = {};
        this->frame = {};

        this->matrix_chunk = this->matrix_descriptor.ReserveRange(sizeof(Maths::Matrix4x4));
        if(this->matrix_chunk == nullptr) {
            if(!this->matrix_descriptor.ResizeChunk(this->matrix_descriptor.GetChunk()->range + sizeof(Maths::Matrix4x4) * 10, 0, Vulkan::SboAlignment())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "StaticEntity::StaticEntity() : Not enough memory" << std::endl;
                #endif
                return;
            }else{
                this->matrix_chunk = this->matrix_descriptor.ReserveRange(sizeof(Maths::Matrix4x4));
                DynamicEntity::matrix_array = reinterpret_cast<Maths::Matrix4x4*>(DynamicEntity::matrix_descriptor.DirectStagingAccess());
            }
        }

        /*this->movement_chunk = this->movement_descriptor.ReserveRange(sizeof(MOVEMENT_DATA));
        if(this->movement_chunk == nullptr) {
            if(!this->movement_descriptor.ResizeChunk(this->movement_descriptor.GetChunk()->range + sizeof(MOVEMENT_DATA) * 10, 0, Vulkan::SboAlignment())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "StaticEntity::StaticEntity() : Not enough memory" << std::endl;
                #endif
                return;
            }else{
                this->movement_chunk = this->movement_descriptor.ReserveRange(sizeof(MOVEMENT_DATA));
            }
        }*/

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

        /*MOVEMENT_DATA move = {};
        move.radius = 0.5f;
        move.moving = -1;*/

        DynamicEntity::matrix_descriptor.WriteData(&matrix, sizeof(Maths::Matrix4x4), this->matrix_chunk->offset);
        DynamicEntity::animation_descriptor.WriteData(&this->frame, sizeof(FRAME_DATA), this->frame_chunk->offset, DESCRIPTOR_BIND_FRAME);
        DynamicEntity::animation_descriptor.WriteData(&this->animation, sizeof(ANIMATION_DATA), this->animation_chunk->offset, DESCRIPTOR_BIND_ANIMATION);
        // DynamicEntity::movement_descriptor.WriteData(&move, sizeof(MOVEMENT_DATA), this->movement_chunk->offset);

        DynamicEntity::entities.push_back(this);

        for(auto& listener : this->Listeners)
            listener->NewEntity(this);
    }

    void DynamicEntity::PlayAnimation(std::string animation, float speed, bool loop)
    {
        if(DataBank::HasAnimation(animation)) {
            this->animation.speed = speed;

            this->frame.frame_id = 0;
            this->frame.animation_id = DataBank::GetAnimation(animation).animation_id;
            this->animation.frame_count = DataBank::GetAnimation(animation).frame_count;
            this->animation.duration = static_cast<uint32_t>(DataBank::GetAnimation(animation).duration.count());

            this->animation.loop = loop ? 1 : 0;
            this->animation.play = 1;

            this->animation.start = static_cast<uint32_t>(Timer::EngineStartDuration().count());

            DynamicEntity::animation_descriptor.WriteData(&this->frame, sizeof(FRAME_DATA), this->frame_chunk->offset, DESCRIPTOR_BIND_FRAME);
            DynamicEntity::animation_descriptor.WriteData(&this->animation, sizeof(ANIMATION_DATA), this->animation_chunk->offset, DESCRIPTOR_BIND_ANIMATION);
        }
    }

    void DynamicEntity::StopAnimation()
    {
        this->animation = {};
        this->frame = {};

        DynamicEntity::animation_descriptor.WriteData(&this->frame, sizeof(FRAME_DATA), this->frame_chunk->offset, DESCRIPTOR_BIND_FRAME);
        DynamicEntity::animation_descriptor.WriteData(&this->animation, sizeof(ANIMATION_DATA), this->animation_chunk->offset, DESCRIPTOR_BIND_ANIMATION);
    }

    void DynamicEntity::SetAnimationFrame(std::string animation, uint32_t frame_id)
    {
        if(DataBank::HasAnimation(animation)) {
            this->frame.animation_id = DataBank::GetAnimation(animation).animation_id;
            this->animation.start = frame_id;

            DynamicEntity::animation_descriptor.WriteData(&this->frame, sizeof(FRAME_DATA), this->frame_chunk->offset, DESCRIPTOR_BIND_FRAME);
            DynamicEntity::animation_descriptor.WriteData(&this->animation, sizeof(ANIMATION_DATA), this->animation_chunk->offset, DESCRIPTOR_BIND_ANIMATION);
        }
    }

    bool DynamicEntity::InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane)
    {
        for(auto& lod : this->lods) {
            if(lod->GetHitBox() != nullptr) {
                Maths::Vector3 box_min = this->GetMatrix() * lod->GetHitBox()->near_left_bottom_point;
                Maths::Vector3 box_max = this->GetMatrix() * lod->GetHitBox()->far_right_top_point;
                if(Maths::aabb_inside_half_space(left_plane, box_min, box_max) && Maths::aabb_inside_half_space(right_plane, box_min, box_max)
                && Maths::aabb_inside_half_space(top_plane, box_min, box_max) && Maths::aabb_inside_half_space(bottom_plane, box_min, box_max))
                    return true;
            }
        }

        return false;
    }

    bool DynamicEntity::IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction)
    {
        for(auto& lod : this->lods) {
            if(lod->GetHitBox() != nullptr) {
                if(Maths::ray_box_aabb_intersect(ray_origin, ray_direction,
                                                 this->GetMatrix() * lod->GetHitBox()->near_left_bottom_point,
                                                 this->GetMatrix() * lod->GetHitBox()->far_right_top_point,
                                                 -2000.0f, 2000.0f))
                    return true;
            }
        }

        return false;
    }

    void DynamicEntity::UpdateSelection(std::vector<DynamicEntity*> entities)
    {
        uint32_t count = static_cast<uint32_t>(entities.size());
        
        if(DynamicEntity::selection_descriptor.GetChunk()->range < (count + 1) * sizeof(uint32_t)) {
            if(!DynamicEntity::selection_descriptor.ResizeChunk((count + 1) * sizeof(uint32_t), 0, Vulkan::SboAlignment())) {
                count = static_cast<uint32_t>(DynamicEntity::selection_descriptor.GetChunk()->range / sizeof(uint32_t)) - 1;
                #if defined(DISPLAY_LOGS)
                std::cout << "Map::UpdateSelection : Not engough memory" << std::endl;
                #endif
            }
        }

        DynamicEntity::selection_descriptor.WriteData(&count, sizeof(uint32_t), 0);

        std::vector<uint32_t> selected_ids(count);
        for(uint32_t i=0; i<count; i++) selected_ids[i] = entities[i]->GetInstanceId();

        DynamicEntity::selection_descriptor.WriteData(selected_ids.data(), selected_ids.size() * sizeof(uint32_t), sizeof(uint32_t));
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
        for(auto entity : DynamicEntity::entities) {
            if(selected_entity != nullptr) {
                entity->selected = false;
            }else if(entity->IntersectRay(camera_position, mouse_ray)) {
                selected_entity = entity;
                entity->selected = true;
            }else {
                entity->selected = false;
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

        /*Maths::Vector4 planes[] = {
            Maths::Vector4(left_plane.origin.x, left_plane.origin.y, left_plane.origin.z, 1.0f),
            Maths::Vector4(left_plane.normal.x, left_plane.normal.y, left_plane.normal.z, 1.0f),
            Maths::Vector4(right_plane.origin.x, right_plane.origin.y, right_plane.origin.z, 1.0f),
            Maths::Vector4(right_plane.normal.x, right_plane.normal.y, right_plane.normal.z, 1.0f),
            Maths::Vector4(top_plane.origin.x, top_plane.origin.y, top_plane.origin.z, 1.0f),
            Maths::Vector4(top_plane.normal.x, top_plane.normal.y, top_plane.normal.z, 1.0f),
            Maths::Vector4(bottom_plane.origin.x, bottom_plane.origin.y, bottom_plane.origin.z, 1.0f),
            Maths::Vector4(bottom_plane.normal.x, bottom_plane.normal.y, bottom_plane.normal.z, 1.0f)
        };*/

        // DynamicEntity::selection_descriptor.WriteData(planes, sizeof(planes), 0, DESCRIPTOR_BIND_PLANES);

        std::vector<DynamicEntity*> return_value;
        for(auto entity : DynamicEntity::entities) {
            if(entity->InSelectBox(left_plane, right_plane, top_plane, bottom_plane)) {
                entity->selected = true;
                return_value.push_back(entity);
            }else{
                entity->selected = false;
            }
        }

        return return_value;
    }

    //void DynamicEntity::MoveToPosition(Maths::Vector3 destination)
    //{
    //    /*MOVEMENT_GROUP group;
    //    group.destination = {destination.x, destination.z};
    //    group.scale = 2;
    //    group.unit_radius = 0.5f;
    //    group.unit_count = 0;
    //    group.inside_count = 0;
    //    group.fill_count = 0;
    //    group.padding = 0;

    //    uint32_t group_id = DynamicEntity::movement_group_count + 1;
    //    for(uint8_t i=0; i<DynamicEntity::movement_group_count; i++) {
    //        MOVEMENT_GROUP group_check = DynamicEntity::groups_array[i];
    //        if(group_check.unit_count == 0) {
    //            group_id = i+1;
    //            break;
    //        }
    //    }

    //    for(auto& entity : DynamicEntity::entities)
    //        if(entity->selected && (entity->GetMatrix()[12] != destination.x || entity->GetMatrix()[14] != destination.z))
    //            group.unit_count++;
    //    if(group.unit_count < 2) group_id = 0;

    //    for(auto& entity : DynamicEntity::entities) {
    //        MOVEMENT_DATA movement = entity->GetMovement();
    //        if(entity->selected && (entity->GetMatrix()[12] != destination.x || entity->GetMatrix()[14] != destination.z)) {
    //            movement.destination = {destination.x, destination.z};
    //            movement.moving = group_id;
    //        }
    //        DynamicEntity::movement_descriptor.WriteData(&movement, sizeof(MOVEMENT_DATA), entity->movement_chunk->offset, DESCRIPTOR_BIND_MOVEMENT);
    //    }

    //    if(group.unit_count < 2) return;

    //    if(group_id == (DynamicEntity::movement_group_count + 1)) {
    //        auto chunk = DynamicEntity::movement_descriptor.ReserveRange(sizeof(MOVEMENT_GROUP), DESCRIPTOR_BIND_GROUPS);
    //        if(chunk == nullptr) {
    //            if(!DynamicEntity::movement_descriptor.ResizeChunk(
    //                DynamicEntity::movement_descriptor.GetChunk(DESCRIPTOR_BIND_GROUPS)->range + sizeof(MOVEMENT_GROUP),
    //                DESCRIPTOR_BIND_GROUPS, Vulkan::SboAlignment()))
    //            {
    //                #if defined(DISPLAY_LOGS)
    //                std::cout << "DynamicEntity::MoveToPosition() : Not enough memory" << std::endl;
    //                #endif
    //                return;
    //            }else{
    //                chunk = DynamicEntity::movement_descriptor.ReserveRange(sizeof(MOVEMENT_GROUP), DESCRIPTOR_BIND_GROUPS);
    //            }
    //        }
    //    }

    //    DynamicEntity::movement_group_count++;
    //    DynamicEntity::movement_descriptor.WriteData(&group, sizeof(MOVEMENT_GROUP), (group_id - 1) * sizeof(MOVEMENT_GROUP), DESCRIPTOR_BIND_GROUPS);
    //    DynamicEntity::movement_descriptor.WriteData(&DynamicEntity::movement_group_count, sizeof(uint32_t), 0, DESCRIPTOR_BIND_GROUP_COUNT);*/
    //}

    void DynamicEntity::Update(uint8_t instance_id)
    {
        // DynamicEntity::matrix_array = reinterpret_cast<Maths::Matrix4x4*>(DynamicEntity::matrix_descriptor.DirectStagingAccess());
        /*DynamicEntity::movement_array = reinterpret_cast<MOVEMENT_DATA*>(DynamicEntity::movement_descriptor.DirectStagingAccess());
        DynamicEntity::movement_group_count = *reinterpret_cast<uint32_t*>(DynamicEntity::movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_GROUP_COUNT));
        DynamicEntity::groups_array = reinterpret_cast<MOVEMENT_GROUP*>(DynamicEntity::movement_descriptor.DirectStagingAccess(0, DESCRIPTOR_BIND_GROUPS));*/

        /*for(uint8_t i=0; i<DynamicEntity::movement_group_count; i++) {
            MOVEMENT_GROUP group = DynamicEntity::groups_array[i];
            uint32_t max_count = 1;
            for(int j=1; j<group.scale; j++) max_count += j * 6;
            if(group.inside_count >= max_count) {
                group.scale++;
                group.fill_count = 0;
                group.inside_count = 0;

                DynamicEntity::movement_descriptor.WriteData(&group, sizeof(MOVEMENT_GROUP), i * sizeof(MOVEMENT_GROUP), DESCRIPTOR_BIND_GROUPS);
            }
        }*/

        /*for(auto entity : DynamicEntity::entities) {
            if(entity->moving) {
                auto move_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - entity->move_start);
                Maths::Vector3 new_position = entity->move_initial_position + entity->move_direction * entity->move_speed * static_cast<float>(move_duration.count());

                if((new_position - entity->move_initial_position).Length() >= entity->move_length) {
                    entity->moving = false;
                    entity->matrix[12] = entity->move_destination.x;
                    entity->matrix[13] = entity->move_destination.y;
                    entity->matrix[14] = entity->move_destination.z;
                }else{
                    entity->matrix[12] = new_position.x;
                    entity->matrix[13] = new_position.y;
                    entity->matrix[14] = new_position.z;
                }

                DynamicEntity::matrix_descriptor.WriteData(&entity->matrix, sizeof(Maths::Matrix4x4), entity->matrix_chunk->offset);
            }
        }*/
    }
}