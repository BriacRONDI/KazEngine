#pragma once

#include <vector>
#include <fstream>

#include <vector>

#if defined(_DEBUG)
#include <iostream>
#endif

namespace Engine
{
    namespace Tools
    {
        struct IMAGE_MAP {
            uint32_t width;
            uint32_t height;
            uint32_t format;
            std::vector<unsigned char> data;
        };

        std::vector<char> GetBinaryFileContents(std::string const &filename);
        IMAGE_MAP LoadImageFile(std::string filename);
    }
}