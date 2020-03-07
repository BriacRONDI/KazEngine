#pragma once

#include "../../Graphics/Vulkan/Vulkan.h"
#include "../../Graphics/Renderer/Renderer.h"
#include "../../Graphics/Camera/Camera.h"
#include "../ManagedBuffer/ManagedBuffer.h"
#include "../../Graphics/Vulkan/DescriptorSetManager/DescriptorSetManager.h"
#include "../../Common/Maths/Maths.h"

namespace Engine
{
    class Map
    {
        public :

            static constexpr uint32_t VERTEX_BUFFER_SIZE = 1024 * 1024 * 10;
            
            static inline Map& GetInstance() { if(Map::instance == nullptr) Map::instance = new Map; return *Map::instance; }
            void DestroyInstance();
            VkCommandBuffer BuildCommandBuffer(uint32_t swap_chain_image_index, VkFramebuffer frame_buffer);

        private :

            static Map* instance;
            std::vector<VkCommandBuffer> command_buffers;
            ManagedBuffer map_data_buffer;
            Renderer renderer;

            VkDeviceSize index_buffer_offet = 0;

            Map();
            ~Map();

            void UpdateVertexBuffer();
    };
}