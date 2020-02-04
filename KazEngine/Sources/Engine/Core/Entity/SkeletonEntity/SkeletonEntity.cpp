#include "SkeletonEntity.h"
#include "../../Core.h"

namespace Engine
{
    uint32_t SkeletonEntity::UpdateUBO(ManagedBuffer& buffer)
    {
        uint32_t local_offset = Entity::UpdateUBO(buffer);
        buffer.WriteData(&this->frame_index, sizeof(uint32_t), this->dynamic_buffer_offset + local_offset, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
        local_offset += sizeof(uint32_t);
        buffer.WriteData(&this->bones_per_frame, sizeof(uint32_t), this->dynamic_buffer_offset + local_offset, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
        local_offset += sizeof(uint32_t);
        return local_offset;
    }
}