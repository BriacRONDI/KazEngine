#pragma once

#include "Entity.h"

namespace Engine
{
    class Entity;

    class IEntityListener
    {
        public :

            virtual void AddMesh(Entity& entity, std::shared_ptr<Model::Mesh> mesh) = 0;
    };
}