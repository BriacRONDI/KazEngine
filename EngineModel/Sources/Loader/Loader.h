#pragma once

#include <Tools.h>
#include <DataPacker.h>

#include "../Mesh/Mesh.h"
#include "../Skeleton/Skeleton.h"

namespace Model
{
    class Loader
    {
        public :
            static inline Tools::IMAGE_MAP GetImageFromPackage(std::vector<char> const& data_buffer, std::string const& path)
            {
                auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
                auto image_package = DataPacker::Packer::FindPackedItem(data_tree, path);
                return Tools::LoadImageData(image_package.Data(data_buffer.data()), image_package.size);
            }

            static inline std::shared_ptr<Mesh> GetMeshFromPackage(std::vector<char> const& data_buffer, std::string const& path)
            {
                auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
                auto node = DataPacker::Packer::FindPackedItem(data_tree, path);
                std::shared_ptr<Mesh> mesh(new Mesh);
                mesh->Deserialize(node.Data(data_buffer.data()));

                return mesh;
            }

            /*static inline Mesh::MATERIAL GetMaterialFromPackage(std::vector<char> const& data_buffer, std::string const& path)
            {
                auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
                auto node = DataPacker::Packer::FindPackedItem(data_tree, path);
                Mesh::MATERIAL material;
                material.Deserialize(node.Data(data_buffer.data()));
                return material;
            }*/

            static inline Bone GetSkeletonFromPackage(std::vector<char> const& data_buffer, std::string const& path)
            {
                auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
                auto node = DataPacker::Packer::FindPackedItem(data_tree, path);
                Bone bone;
                bone.Deserialize(node.Data(data_buffer.data()));
                return bone;
            }

        private :
            
            Loader(){}
    };
}