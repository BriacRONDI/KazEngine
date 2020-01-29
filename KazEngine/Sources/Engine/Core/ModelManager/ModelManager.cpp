#include "ModelManager.h"

namespace Engine
{
    /**
     * Nettoyage des ressources
     */
    void ModelManager::Clear()
    {
        this->textures.clear();
        this->materials.clear();
        this->models.clear();
        this->skeletons.clear();
    }

    /**
     * Chargement d'un fichier kea (Kaz Engine Archive)
     */
    bool ModelManager::LoadFile(std::string const& filename)
    {
        // Lecture du contenu du fichier
        std::vector<char> raw_data = Engine::Tools::GetBinaryFileContents(filename);
        if(raw_data.size() == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "LoadFile(" << filename << ") => GetBinaryFileContents : Failed" << std::endl;
            #endif
            return false;
        }

        // Lecture de la table de données du fichier
        std::vector<Engine::Packer::DATA> package = Engine::Packer::UnpackMemory(raw_data);
        if(package.size() == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "LoadFile(" << filename << ") => UnpackMemory : Failed" << std::endl;
            #endif
            return false;
        }

        // Récupération la liste des Mesh
        Engine::Packer::DATA mesh_list_pack = Engine::Packer::FindPackedItem(package, "/meshes");
        if(mesh_list_pack.size == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "LoadFile(" << filename << ") => FindPackedItem(\"/meshes\") : Failed" << std::endl;
            #endif
            return false;
        }

        for(auto& mesh_pack : mesh_list_pack.children) {

            // Désérialization du mesh
            std::shared_ptr<Engine::Mesh> mesh(new Engine::Mesh);
            mesh->Deserialize(raw_data.data() + mesh_pack.position + mesh_pack.HeaderSize());
            if(mesh->vertex_buffer.size() == 0) {
                #if defined(DISPLAY_LOGS)
                std::cout << "LoadFile(" << filename << ") Mesh(" << mesh_pack.name << ") => Deserialize : Failed" << std::endl;
                #endif
                return false;
            }

            if(!mesh->skeleton.empty() && !this->skeletons.count(mesh->skeleton)) {

                // Recherche du squelette dans le fichier
                Engine::Packer::DATA skeleton_packed_data = Engine::Packer::FindPackedItem(package, "/bones/" + mesh->skeleton);
                if(skeleton_packed_data.size == 0) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "LoadFile(" << filename << ") Mesh(" << mesh_pack.name << ") Skeleton(" << mesh->skeleton << ") => FindPackedItem : Failed" << std::endl;
                    #endif
                    return false;
                }

                // Désérialization du squelette
                std::shared_ptr<Engine::BONE> bone(new Engine::BONE);
                bone->Deserialize(raw_data.data() + skeleton_packed_data.position + skeleton_packed_data.HeaderSize());
                this->skeletons[bone->name] = bone;
                #if defined(DISPLAY_LOGS)
                std::cout << "LoadFile(" << filename << ") Skeleton(" << bone->name << ") : Loaded (" << bone->Count() << " bones)" << std::endl;
                #endif
            }

            for(auto& mesh_material : mesh->materials) {
                
                // Si le matériau est déjà référencé, on passe au suivant
                if(this->materials.count(mesh_material.first) > 0) continue;

                // Recherche du matériau dans le fichier
                Engine::Packer::DATA material_packed_data = Engine::Packer::FindPackedItem(package, "/materials/" + mesh_material.first);
                if(material_packed_data.size == 0) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "LoadFile(" << filename << ") Mesh(" << mesh_pack.name << ") Material(" << mesh_material.first << ") => FindPackedItem : Failed" << std::endl;
                    #endif
                    return false;
                }

                // Désérialization du matériau
                Engine::Mesh::MATERIAL material;
                size_t bytes_read = material.Deserialize(raw_data.data() + material_packed_data.position + material_packed_data.HeaderSize());
                if(!bytes_read) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "LoadFile(" << filename << ") Mesh(" << mesh_pack.name << ") Material(" << mesh_material.first << ") => Deserialize : Failed" << std::endl;
                    #endif
                    return false;
                }

                // On regarde si le matériau est associé à une texture.
                if(!material.texture.empty()) {

                    // Si la texture est déjà chargée, on passe au matériau suivant
                    if(this->textures.count(material.texture) > 0) continue;

                    // Recherche de la texture dans le fichier
                    Engine::Packer::DATA texture_packed_data = Engine::Packer::FindPackedItem(package, "/textures/" + material.texture);
                    if(texture_packed_data.size == 0) {
                        #if defined(DISPLAY_LOGS)
                        std::cout << "LoadFile(" << filename << ") Mesh(" << mesh->name << ") Material("
                                  << mesh_material.first << ") => FindPackedItem(/textures/" << material.texture << ") : Failed" << std::endl;
                        #endif
                        return false;
                    }

                    // Chargement de l'image
                    auto data_offset = raw_data.data() + texture_packed_data.position + texture_packed_data.HeaderSize();
                    std::vector<char> image_data = std::vector<char>(data_offset, data_offset + texture_packed_data.size);
                    Engine::Tools::IMAGE_MAP image = Engine::Tools::LoadImageData(image_data);
                    if(image.data.size() == 0) {
                        #if defined(DISPLAY_LOGS)
                        std::cout << "LoadFile(" << filename << ") Mesh(" << mesh->name << ") Material("
                                  << mesh_material.first << ") => Texture(" << material.texture << ") => LoadImageData : Failed" << std::endl;
                        #endif
                        return false;
                    }

                    // Mise en mémoire de la texture
                    this->textures[material.texture] = image;
                }

                // Mise en mémoire du matériau
                this->materials[mesh_material.first] = material;
            }

            #if defined(DISPLAY_LOGS)
            std::cout << "LoadFile(" << filename << ") Mesh(" << mesh->name << ") : Loaded" << std::endl;
            #endif
            // Mise en mémoire du Mesh
            mesh->UpdateRenderMask(this->materials);
            this->models[mesh->name] = mesh;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "LoadFile(" << filename << ") : Success" << std::endl;
        #endif
        return true;
    }
}