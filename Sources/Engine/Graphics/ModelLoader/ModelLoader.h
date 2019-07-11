#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h> 
#include <assimp/postprocess.h>

#include "../Vulkan/Vulkan.h"

namespace Engine
{
    class ModelLoader
    {
        public:

            bool LoadFromFile(std::string file_name);

        private:

            std::unordered_map<uint32_t, uint32_t> textures;

            bool LoadMaterials(const aiScene* scene, std::string directory);
            bool ProcessNode(aiNode* node, const aiScene* scene);
            bool ProcessMesh(aiMesh* mesh, const aiScene* scene);
            std::string GetDirectory(std::string& file_name);
            void ShowMaterialInformation(const aiMaterial* pMaterial);
            void ShowTextureInformation(const aiMaterial* pMaterial, aiTextureType pType, unsigned int pTextureNumber);
    };
}