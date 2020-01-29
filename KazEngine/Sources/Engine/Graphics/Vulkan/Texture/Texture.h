#pragma once

#include "../Vulkan.h"

namespace Engine
{
    class Texture
    {
        public  : 
            bool Create(Tools::IMAGE_MAP const& image);

        private :

            Vulkan::IMAGE_BUFFER image_buffer;
    };
}