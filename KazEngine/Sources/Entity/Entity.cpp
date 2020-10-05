#include "Entity.h"
#include "IEntityListener.h"

namespace Engine
{
    uint32_t Entity::next_id = 0;

    void Entity::AddLOD(std::shared_ptr<LODGroup> lod)
    {
        this->lods.push_back(lod);
        for(auto& listener : Entity::Listeners) listener->AddLOD(*this, lod);
    }
}