#include "Tools.h"

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
    }
}