#pragma once

#include "../../Graphics/Mesh/Mesh.h"

namespace Engine
{
    class ModelManager
    {
        public :

            std::map<std::string, std::shared_ptr<Mesh>> models;
            std::map<std::string, Mesh::MATERIAL> materials;
            std::map<std::string, Engine::Tools::IMAGE_MAP> textures;
            std::map<std::string, std::shared_ptr<Engine::Bone>> skeletons;

            static inline ModelManager& GetInstance() { if(ModelManager::instance == nullptr) ModelManager::instance = new ModelManager; return *ModelManager::instance; }
            void DestroyInstance();
            bool LoadFile(std::string const& filename);
            void Clear();

        private :
            
            static ModelManager* instance;

            ModelManager() = default;
            ~ModelManager() = default;
    };
}