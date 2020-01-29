#include "Texture.h"

namespace Engine
{
    /**
     * Création d'une texture dans la carte graphique
     */
    bool Texture::Create(Tools::IMAGE_MAP const& image)
    {
        // Création d'un buffer pour l'image
        this->image_buffer = Vulkan::GetInstance().CreateImageBuffer(
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            image.width, image.height);

        // En cas d'échec, on renvoie false
        if(this->image_buffer.handle == VK_NULL_HANDLE) return false;
        
        return true;
    }
}