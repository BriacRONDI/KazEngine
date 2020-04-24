#pragma once

#include <chrono>
#include "../Vulkan/Vulkan.h"
#include "../Scene/Map/Map.h"
#include "../ManagedBuffer/ManagedBuffer.h"

#if defined(DISPLAY_LOGS)
#include <iostream>
#endif

#define MULTI_USAGE_BUFFER_MASK VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT

namespace Engine
{
    class Core
    {
        public :

            inline ~Core() { this->Clear(); }
            void Clear();
            bool Initialize();
            void Loop();

        private :

            uint8_t current_frame_index;
            VkCommandPool graphics_command_pool;
            std::vector<Vulkan::RENDERING_RESOURCES> resources;
            Map* map;
            std::vector<ManagedBuffer> game_buffers;
            std::vector<Vulkan::STAGING_BUFFER> staging_buffers;

            bool AllocateRenderingResources();
            bool BuildRenderPass(uint32_t swap_chain_image_index);
            bool RebuildFrameBuffers();
    };
}