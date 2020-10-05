#pragma once

#include "Entity.h"

namespace Engine
{
    class Entity;
    class LODGroup;

    class IEntityListener
    {
        public :

            virtual void NewEntity(Entity* entity) = 0;
            virtual void AddLOD(Entity& entity, std::shared_ptr<LODGroup> lod) = 0;
    };
}