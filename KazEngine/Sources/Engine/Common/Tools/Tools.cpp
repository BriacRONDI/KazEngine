#include "Tools.h"

#if !defined(STBI_INCLUDE_STB_IMAGE_H)
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"
#endif

namespace Engine
{
    namespace Tools
    {
        /**
         * Récupère le contenu d'un fichier dans un buffer
         */
        std::vector<char> GetBinaryFileContents(std::string const &filename)
        {
            // Ouverture du fichier
            std::ifstream file(filename, std::ios::binary);

            // En cas d'échec cd'ouverture du fichier, on renvoie un buffer vide
            if(file.fail()) {
                #if defined(DISPLAY_LOGS)
                std::cout << "GetBinaryFileContents(\"" << filename << "\") failed" << std::endl;
                #endif
                return {};
            }

            // On récupère la taille du fichier
            std::streampos begin, end;
            begin = file.tellg();
            file.seekg(0, std::ios::end);
            end = file.tellg();

            // On dimensionne le buffer à la taille du fichier
            std::vector<char> result(static_cast<size_t>(end - begin));

            // Lecture des données
            file.seekg(0, std::ios::beg);
            file.read(&result[0], end - begin);
            file.close();

            // Renvoi du buffer
            return result;
        }

        /**
         * Sauvegarde des données dans un fichier
         */
        bool WriteToFile(std::vector<char> const& data, std::string const &filename)
        {
            // Ouverture du fichier
            std::ofstream file(filename, std::ofstream::binary);

            // En cas d'échec cd'ouverture du fichier, on sort en erreur
            if(file.fail()) {
                #if defined(DISPLAY_LOGS)
                std::cout << "SaveToFile(\"" << filename << "\") failed" << std::endl;
                #endif
                return false;
            }

            // Écriture des données
            file.write(data.data(), data.size());

            // Fermeture du fichier
            file.close();

            // Succès
            return true;
        }

        /**
         * Lecture d'un fichier image en utilisant la librairie stb
         * https://github.com/nothings/stb
         */
        IMAGE_MAP LoadImageFile(std::string filename)
        {
            IMAGE_MAP image = {};
            stbi_uc* stbi_data = stbi_load(filename.c_str(), reinterpret_cast<int*>(&image.width), reinterpret_cast<int*>(&image.height), reinterpret_cast<int*>(&image.format), STBI_rgb_alpha);
            if(stbi_data != nullptr) {
                image.data.resize(image.width * image.height * STBI_rgb_alpha);
                std::memcpy(image.data.data(), stbi_data, image.width * image.height * STBI_rgb_alpha);
                stbi_image_free(stbi_data);
            }
            return image;
        }

        /**
         * Lecture d'un fichier image stocké en mémoire en utilisant la librairie stb
         */
        IMAGE_MAP LoadImageData(std::vector<char> buffer)
        {
            IMAGE_MAP image = {};
            stbi_uc* stbi_data = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(buffer.data()), static_cast<int>(buffer.size()), reinterpret_cast<int*>(&image.width), reinterpret_cast<int*>(&image.height), reinterpret_cast<int*>(&image.format), STBI_rgb_alpha);
            if(stbi_data != nullptr) {
                image.data.resize(image.width * image.height * STBI_rgb_alpha);
                std::memcpy(image.data.data(), stbi_data, image.width * image.height * STBI_rgb_alpha);
                stbi_image_free(stbi_data);
            }
            return image;
        }

        /**
         * Récupère le répertoire contenant le fichier
         */
        std::string GetFileDirectory(std::string const& path)
        {
            if(path[path.size() - 1] == '\\' || path[path.size() - 1] == '/') return path;

            std::size_t pos = path.find_last_of('/');
            if(pos == std::string::npos) pos = path.find_last_of('\\');
            if(pos != std::string::npos) return path.substr(0, pos);

            return std::string();
        }

        /**
         * Récupère le nom du fichier dans le chemin complet
         */
        std::string GetFileName(std::string const& path)
        {
            if(path.empty()) return {};
            if(path[path.size() - 1] == '\\' || path[path.size() - 1] == '/') return {};

            std::size_t pos = path.find_last_of('/');
            if(pos == std::string::npos) pos = path.find_last_of('\\');
            if(pos != std::string::npos) return path.substr(pos + 1);
            else return path;
        }
    }
}