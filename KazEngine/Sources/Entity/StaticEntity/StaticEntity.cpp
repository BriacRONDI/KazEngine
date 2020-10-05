#include "StaticEntity.h"

namespace Engine
{
    DescriptorSet StaticEntity::matrix_descriptor;

    void StaticEntity::Clear()
    {
        StaticEntity::matrix_descriptor.Clear();
    }

    StaticEntity::StaticEntity()
    {
        this->matrix_chunk = this->matrix_descriptor.ReserveRange(sizeof(Maths::Matrix4x4));
        if(this->matrix_chunk == nullptr) {
            if(!this->matrix_descriptor.ResizeChunk(this->matrix_descriptor.GetChunk()->range + sizeof(Maths::Matrix4x4) * 10, 0, Vulkan::SboAlignment())) {
                #if defined(DISPLAY_LOGS)
                std::cout << "StaticEntity::StaticEntity() : Not enough memory" << std::endl;
                #endif
            }else{
                this->matrix_chunk = this->matrix_descriptor.ReserveRange(sizeof(Maths::Matrix4x4));
            }
        }

        StaticEntity::matrix_descriptor.WriteData(&this->matrix, sizeof(Maths::Matrix4x4), this->matrix_chunk->offset);
    }

    bool StaticEntity::Initialize()
    {
        return StaticEntity::matrix_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Maths::Matrix4x4) * 10}
        });
    }

    void StaticEntity::SetMatrix(Maths::Matrix4x4 matrix)
    {
        this->matrix = matrix;
        StaticEntity::matrix_descriptor.WriteData(&this->matrix, sizeof(Maths::Matrix4x4), this->matrix_chunk->offset);
    }
}