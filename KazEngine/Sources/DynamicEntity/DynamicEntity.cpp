#include "DynamicEntity.h"

namespace Engine
{
    DynamicEntity::DynamicEntity()
    {
        this->selected  = false;
        this->matrix    = nullptr;
        this->frame     = nullptr;
        this->animation = nullptr;
        this->instance_id = UINT32_MAX;

        auto matrix_chunk = GlobalData::GetInstance()->dynamic_entity_descriptor.ReserveRange(sizeof(Maths::Matrix4x4), ENTITY_MATRIX_BINDING);
        if(matrix_chunk == nullptr) return;

        auto frame_chunk = GlobalData::GetInstance()->dynamic_entity_descriptor.ReserveRange(sizeof(FRAME_DATA), ENTITY_FRAME_BINDING);
        if(frame_chunk == nullptr) return;

        auto animation_chunk = GlobalData::GetInstance()->dynamic_entity_descriptor.ReserveRange(sizeof(ANIMATION_DATA), ENTITY_ANIMATION_BINDING);
        if(animation_chunk == nullptr) return;

        auto movement_chunk = GlobalData::GetInstance()->dynamic_entity_descriptor.ReserveRange(sizeof(MOVEMENT_DATA), ENTITY_MOVEMENT_BINDING);
        if(movement_chunk == nullptr) return;

        this->instance_id = static_cast<uint32_t>(matrix_chunk->offset / sizeof(Maths::Matrix4x4));
        this->matrix = reinterpret_cast<Maths::Matrix4x4*>(GlobalData::GetInstance()->dynamic_entity_descriptor.AccessData(this->instance_id * sizeof(Maths::Matrix4x4), ENTITY_MATRIX_BINDING));
        this->frame = reinterpret_cast<FRAME_DATA*>(GlobalData::GetInstance()->dynamic_entity_descriptor.AccessData(this->instance_id * sizeof(FRAME_DATA), ENTITY_FRAME_BINDING));
        this->animation = reinterpret_cast<ANIMATION_DATA*>(GlobalData::GetInstance()->dynamic_entity_descriptor.AccessData(this->instance_id * sizeof(ANIMATION_DATA), ENTITY_ANIMATION_BINDING));
        this->movement = reinterpret_cast<MOVEMENT_DATA*>(GlobalData::GetInstance()->dynamic_entity_descriptor.AccessData(this->instance_id * sizeof(MOVEMENT_DATA), ENTITY_MOVEMENT_BINDING));

        *this->matrix = IDENTITY_MATRIX;
        *this->frame = {};
        *this->animation = {};
        (*this->movement).destination = {};
        (*this->movement).moving = -1;
        (*this->movement).radius = 0.5f;
        (*this->movement).grid_position = {-1, -1, -1, -1};
        (*this->movement).skeleton_id = 0;

        GlobalData::GetInstance()->dynamic_entity_descriptor.AddListener(this);
    }

    void DynamicEntity::MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding)
    {
        switch(binding) {

            case ENTITY_MATRIX_BINDING :
                this->matrix = reinterpret_cast<Maths::Matrix4x4*>(descriptor->AccessData(this->instance_id * sizeof(Maths::Matrix4x4), binding));
                break;

            case ENTITY_FRAME_BINDING :
                this->frame = reinterpret_cast<FRAME_DATA*>(descriptor->AccessData(this->instance_id * sizeof(FRAME_DATA), binding));
                break;

            case ENTITY_ANIMATION_BINDING :
                this->animation = reinterpret_cast<ANIMATION_DATA*>(descriptor->AccessData(this->instance_id * sizeof(ANIMATION_DATA), binding));
                break;

            case ENTITY_MOVEMENT_BINDING :
                this->movement = reinterpret_cast<MOVEMENT_DATA*>(descriptor->AccessData(this->instance_id * sizeof(MOVEMENT_DATA), binding));
                break;
        }
    }

    void DynamicEntity::PlayAnimation(std::string animation, float speed, bool loop)
    {
        if(GlobalData::GetInstance()->animations.count(animation)) {
            this->animation->speed = speed;

            this->frame->frame_id = 0;
            this->frame->animation_id = GlobalData::GetInstance()->animations[animation].id;
            this->animation->frame_count = GlobalData::GetInstance()->animations[animation].frame_count;
            this->animation->duration = GlobalData::GetInstance()->animations[animation].duration_ms;

            this->animation->loop = loop ? 1 : 0;
            this->animation->play = 1;

            this->animation->start = static_cast<uint32_t>(Timer::EngineStartDuration().count());
        }
    }

    void DynamicEntity::StopAnimation()
    {
        this->frame->frame_id = 0;
        this->frame->animation_id = -1;
        this->animation = {};
    }

    bool DynamicEntity::InSelectBox(Maths::Plane left_plane, Maths::Plane right_plane, Maths::Plane top_plane, Maths::Plane bottom_plane)
    {
        for(auto& lod : this->models) {
            if(lod->GetHitBox() != nullptr) {
                Maths::Vector3 box_min = *this->matrix * lod->GetHitBox()->near_left_bottom_point;
                Maths::Vector3 box_max = *this->matrix * lod->GetHitBox()->far_right_top_point;
                if(Maths::aabb_inside_half_space(left_plane, box_min, box_max) && Maths::aabb_inside_half_space(right_plane, box_min, box_max)
                && Maths::aabb_inside_half_space(top_plane, box_min, box_max) && Maths::aabb_inside_half_space(bottom_plane, box_min, box_max))
                    return true;
            }
        }

        return false;
    }

    bool DynamicEntity::IntersectRay(Maths::Vector3 const& ray_origin, Maths::Vector3 const& ray_direction)
    {
        for(auto& lod : this->models) {
            if(lod->GetHitBox() != nullptr) {
                if(Maths::ray_box_aabb_intersect(ray_origin, ray_direction,
                                                 *this->matrix * lod->GetHitBox()->near_left_bottom_point,
                                                 *this->matrix * lod->GetHitBox()->far_right_top_point,
                                                 -2000.0f, 2000.0f))
                    return true;
            }
        }

        return false;
    }
}