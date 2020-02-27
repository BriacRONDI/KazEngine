#pragma once

#include "../Entity.h"

namespace Engine
{
    class SkeletonEntity : public Entity
    {
        public :
            
            std::string skeleton;       // Squelette associé à l'entité
            uint32_t frame_index = 0;   // Indice de progression de l'animation
            uint32_t animation_id = 0;  // Offset de l'animation dans le SBO du squelette

            virtual uint32_t UpdateUBO(ManagedBuffer& buffer);
            virtual inline uint32_t GetUboSize() { return Entity::GetUboSize() + sizeof(uint32_t) * 2; }
            virtual void SetAnimation(std::string const& animation);
    };
}