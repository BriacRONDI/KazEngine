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
            std::map<std::string, std::shared_ptr<Engine::BONE>> skeletons;

            bool LoadFile(std::string const& filename);
            void Clear();
    };
}