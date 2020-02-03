#pragma once

#include "../../Core.h"

namespace Engine
{
    class SkeletonEntity : public Entity
    {
        public :

            uint32_t frame_index = 0;   // Indice de progression de l'animation
            std::string animation;      // Animation en cours

            virtual inline uint32_t UpdateUBO(ManagedBuffer& buffer)
            {
                uint32_t bytes_written = Entity::UpdateUBO(buffer);
                // buffer.WriteData(&this->bones, sizeof(std::array<Matrix4x4, MAX_BONES>), this->dynamic_buffer_offset + bytes_written, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
                return bytes_written;
            }

            virtual inline uint32_t GetUboSize()
            {
                return Entity::GetUboSize();
            }
    };
}