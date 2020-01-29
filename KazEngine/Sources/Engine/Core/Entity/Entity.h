#pragma once

#include "../../Graphics/Mesh/Mesh.h"
#include "../ManagedBuffer/ManagedBuffer.h"

namespace Engine
{
    class Entity
    {
        public :

            // Taille maximale de l'UBO d'une Entité
            static constexpr uint16_t MAX_UBO_SIZE = 1024;

            // Offset du dynamic uniform buffer
            uint32_t dynamic_buffer_offset = 0;

            // Matrice spatiale de l'entité
            Matrix4x4 matrix = IDENTITY_MATRIX;

            virtual ~Entity() { this->Clear(); }
            virtual uint32_t UpdateUBO(ManagedBuffer& buffer);
            virtual void Clear();
            virtual inline uint32_t GetUboSize() { return sizeof(Matrix4x4); }
            inline void AttachMesh(std::shared_ptr<Mesh>& mesh){ this->meshes.push_back(mesh); }
            inline std::vector<std::shared_ptr<Mesh>>& GetMeshes(){ return this->meshes; }

        protected :

            // Models associés à l'entité
            std::vector<std::shared_ptr<Mesh>> meshes;
    };
}