#pragma once

#include "../Entity.h"

namespace Engine
{
    class SkeletonEntity : public Entity
    {
        public :
            
            uint32_t bones_per_frame;   // Nombre de bones pour chaque image de l'animation
            uint32_t frame_index = 0;   // Indice de progression de l'animation
            std::string animation;      // Animation en cours

            virtual uint32_t UpdateUBO(ManagedBuffer& buffer);
            virtual inline uint32_t GetUboSize() { return Entity::GetUboSize() + sizeof(uint32_t) * 2; }
    };
}