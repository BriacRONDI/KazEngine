#pragma once

#include "../../Core.h"

namespace Engine
{
    class DefaultColorEntity : public Entity
    {
        public :

            // Couleur globale de l'entité
            Vector3 default_color;

            virtual uint32_t UpdateUBO(ManagedBuffer& buffer)
            {
                uint32_t bytes_written = Entity::UpdateUBO(buffer);
                buffer.WriteData(&this->default_color, sizeof(Vector3), this->dynamic_buffer_offset + bytes_written, Core::SUB_BUFFER_TYPE::ENTITY_UBO);
                return bytes_written + sizeof(Vector3);
            }

            virtual inline uint32_t GetUboSize()
            {
                return Entity::GetUboSize() + sizeof(Vector3);
            }
    };
}