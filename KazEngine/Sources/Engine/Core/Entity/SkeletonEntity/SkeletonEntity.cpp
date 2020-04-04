#include "SkeletonEntity.h"
#include "../../Core.h"

namespace Engine
{
    uint32_t SkeletonEntity::UpdateUBO(ManagedBuffer& buffer)
    {
        uint32_t local_offset = Entity::UpdateUBO(buffer);
        buffer.WriteData(&this->frame_index, sizeof(uint32_t), this->dynamic_buffer_offset + local_offset, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
        local_offset += sizeof(uint32_t);
        buffer.WriteData(&this->animation_id, sizeof(uint32_t), this->dynamic_buffer_offset + local_offset, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
        local_offset += sizeof(uint32_t);
        return local_offset;
    }

    void SkeletonEntity::SetAnimation(std::string const& animation)
    {
        Core& engine = Core::GetInstance();
        this->animation_id = engine.GetAnimationOffset(this->skeleton, animation) / sizeof(Matrix4x4);
    }

    void SkeletonEntity::Update(ManagedBuffer& buffer)
    {
        Entity::Update(buffer);

        if(this->moving) {
            float animation_ratio = static_cast<float>(this->move_duration.count()) / 500.0f;
            this->frame_index = static_cast<uint32_t>(animation_ratio * 30) % 30;
        }
    }
}