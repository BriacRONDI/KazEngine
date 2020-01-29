#include "Entity.h"
#include "../Core.h"

namespace Engine
{
    void Entity::Clear()
    {
        this->meshes.clear();
        this->matrix = IDENTITY_MATRIX;
        this->dynamic_buffer_offset = 0;
    }

    uint32_t Entity::UpdateUBO(ManagedBuffer& buffer)
    {
        buffer.WriteData(&this->matrix, sizeof(Matrix4x4), this->dynamic_buffer_offset, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
        return sizeof(Matrix4x4);
    }
}