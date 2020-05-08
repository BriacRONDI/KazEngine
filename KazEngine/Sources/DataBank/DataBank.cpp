#include "DataBank.h"

namespace Engine
{
    DataBank* DataBank::instance = nullptr;

    DataBank& DataBank::CreateInstance()
    {
        if(DataBank::instance == nullptr) DataBank::instance = new DataBank;
        return *DataBank::instance;
    }

    void DataBank::DestroyInstance()
    {
        if(DataBank::instance != nullptr) {
            delete DataBank::instance;
            DataBank::instance = nullptr;
        }
    }

    Tools::IMAGE_MAP DataBank::GetImageFromPackage(std::vector<char> const& data_buffer, std::string const& path)
    {
        auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
        auto image_package = DataPacker::Packer::FindPackedItem(data_tree, path);
        return Tools::LoadImageData(image_package.Data(data_buffer.data()), image_package.size);
    }

    std::shared_ptr<Model::Mesh> DataBank::GetMeshFromPackage(std::vector<char> const& data_buffer, std::string const& path)
    {
        auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
        auto node = DataPacker::Packer::FindPackedItem(data_tree, path);
        std::shared_ptr<Model::Mesh> mesh(new Model::Mesh);
        mesh->Deserialize(node.Data(data_buffer.data()));

        return mesh;
    }

    Model::Mesh::MATERIAL DataBank::GetMaterialFromPackage(std::vector<char> const& data_buffer, std::string const& path)
    {
        auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
        auto node = DataPacker::Packer::FindPackedItem(data_tree, path);
        Model::Mesh::MATERIAL material;
        material.Deserialize(node.Data(data_buffer.data()));
        return material;
    }

    Model::Bone DataBank::GetSkeletonFromPackage(std::vector<char> const& data_buffer, std::string const& path)
    {
        auto data_tree = DataPacker::Packer::UnpackMemory(data_buffer);
        auto node = DataPacker::Packer::FindPackedItem(data_tree, path);
        Model::Bone bone;
        bone.Deserialize(node.Data(data_buffer.data()));
        return bone;
    }
}