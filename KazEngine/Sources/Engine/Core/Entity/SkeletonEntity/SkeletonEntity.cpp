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
}