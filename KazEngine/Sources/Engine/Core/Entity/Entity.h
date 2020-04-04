#pragma once

#include <chrono>

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

            float move_speed = 5.0f / 1000.0f;

            virtual ~Entity() { this->Clear(); }
            virtual void Update(ManagedBuffer& buffer);
            virtual void Clear();
            virtual inline uint32_t GetUboSize() { return sizeof(Matrix4x4); }
            virtual void MoveToPosition(Vector3 const& destination);
            inline void AttachMesh(std::shared_ptr<Mesh>& mesh){ this->meshes.push_back(mesh); }
            inline std::vector<std::shared_ptr<Mesh>>& GetMeshes(){ return this->meshes; }
            virtual std::vector<Mesh::HIT_BOX> GetHitBoxes();

        protected :

            // Models associés à l'entité
            std::vector<std::shared_ptr<Mesh>> meshes;

            // Déplacement sur la carte
            std::chrono::system_clock::time_point move_start;
            std::chrono::milliseconds move_duration;
            bool moving = false;
            Vector3 move_destination;
            Vector3 move_direction;
            Vector3 move_initial_position;
            float move_length;

            virtual uint32_t UpdateUBO(ManagedBuffer& buffer);
    };
}