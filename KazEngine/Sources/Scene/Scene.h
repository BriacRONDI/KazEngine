#pragma once

#include "../Vulkan/Vulkan.h"

namespace Engine
{
    class Scene
    {
        public :

            static bool Initialize();
            static inline Scene& GetInstance() { return *Scene::instance; }
            std::vector<VkCommandBuffer> const& GetCommandBuffers();

        private :

            static Scene* instance;

            Scene() = default;
            ~Scene() = default;
    };
}