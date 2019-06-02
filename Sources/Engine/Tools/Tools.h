#pragma once

#include <vector>
#include <fstream>

#if defined(_DEBUG)
#include <iostream>
#endif

namespace Engine
{
    namespace Tools
    {
        std::vector<char> GetBinaryFileContents(std::string const &filename);
    }
}