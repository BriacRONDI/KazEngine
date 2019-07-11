#include "Tools.h"

#if !defined(STBI_INCLUDE_STB_IMAGE_H)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
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
            std::ifstream file(filename, std::ios::binary);
            if(file.fail()) {
                #if defined(_DEBUG)
                std::cout << "GetBinaryFileContents(\"" << filename << "\") failed" << std::endl;
                #endif
                return std::vector<char>();
            }

            std::streampos begin, end;
            begin = file.tellg();
            file.seekg( 0, std::ios::end );
            end = file.tellg();

            std::vector<char> result(static_cast<size_t>(end - begin));
            file.seekg( 0, std::ios::beg );
            file.read( &result[0], end - begin );
            file.close();

            return result;
        }

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
    }
}