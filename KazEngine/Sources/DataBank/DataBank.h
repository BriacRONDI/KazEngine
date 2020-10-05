#pragma once

#include <Tools.h>
#include <DataPacker.h>
#include <Model.h>

#include "../ManagedBuffer/ManagedBuffer.h"
#include "../Vulkan/Vulkan.h"

namespace Engine
{
    class DataBank
    {
        public :

            struct BAKED_ANIMATION {
                uint32_t animation_id;
                uint32_t frame_count;
                std::chrono::milliseconds duration;
            };
            
            static DataBank& CreateInstance();
            static inline DataBank& GetInstance() { return *DataBank::instance; }
            static void DestroyInstance();

            static Tools::IMAGE_MAP GetImageFromPackage(std::vector<char> const& data_buffer, std::string const& path);
            static std::shared_ptr<Model::Mesh> GetMeshFromPackage(std::vector<char> const& data_buffer, std::string const& path);
            static Model::Mesh::MATERIAL GetMaterialFromPackage(std::vector<char> const& data_buffer, std::string const& path);
            static Model::Bone GetSkeletonFromPackage(std::vector<char> const& data_buffer, std::string const& path);
            
            static inline ManagedBuffer& GetInstancedBuffer() { return DataBank::instance->instanced_buffer; }
            static inline ManagedBuffer& GetCommonBuffer() { return DataBank::instance->common_buffer; }

            static inline void AddTexture(Tools::IMAGE_MAP image, std::string name) { DataBank::instance->textures[name] = image; };
            static inline std::map<std::string, Tools::IMAGE_MAP>& GetTextures() { return DataBank::instance->textures; }
            static inline Tools::IMAGE_MAP& GetTexture(std::string name) { return DataBank::instance->textures[name]; }
            static inline bool HasTexture(std::string name) { return DataBank::instance->textures.count(name) > 0; }

            static inline void AddMaterial(Model::Mesh::MATERIAL material, std::string name) { DataBank::instance->materials[name] = material; }
            static inline std::map<std::string, Model::Mesh::MATERIAL>& GetMaterials() { return DataBank::instance->materials; }
            static inline Model::Mesh::MATERIAL& GetMaterial(std::string name) { return DataBank::instance->materials[name]; }
            static inline bool HasMaterial(std::string name) { return DataBank::instance->materials.count(name) > 0; }

            static inline void AddSkeleton(Model::Bone skeleton, std::string name) { DataBank::instance->skeletons[name] = skeleton; }
            static inline std::map<std::string, Model::Bone>& GetSkeletons() { return DataBank::instance->skeletons; }
            static inline Model::Bone& GetSkeleton(std::string name) { return DataBank::instance->skeletons[name]; }
            static inline bool HasSkeleton(std::string name) { return DataBank::instance->skeletons.count(name) > 0; }

            static inline void AddAnimation(BAKED_ANIMATION animation, std::string name) { DataBank::instance->animations[name] = animation; }
            static inline std::map<std::string, BAKED_ANIMATION>& GetAnimations() { return DataBank::instance->animations; }
            static inline BAKED_ANIMATION& GetAnimation(std::string name) { return DataBank::instance->animations[name]; }
            static inline bool HasAnimation(std::string name) { return DataBank::instance->animations.count(name) > 0; }

        private :

            static DataBank* instance;

            std::map<std::string, Tools::IMAGE_MAP> textures;
            std::map<std::string, Model::Mesh::MATERIAL> materials;
            std::map<std::string, Model::Bone> skeletons;
            std::map<std::string, BAKED_ANIMATION> animations;
            ManagedBuffer instanced_buffer;
            ManagedBuffer common_buffer;

            DataBank() = default;
            ~DataBank() = default;
    };
}