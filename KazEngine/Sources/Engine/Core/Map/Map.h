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

            struct MAP_UBO {
                Vector3 selection_position;
                uint32_t display_selection;
                Vector3 destination_position;
                uint32_t display_destination;

                MAP_UBO() : display_selection(0), display_destination(0) {}
            };

            static constexpr uint32_t VERTEX_BUFFER_SIZE = 1024 * 1024 * 10;
            
            static inline Map& GetInstance() { if(Map::instance == nullptr) Map::instance = new Map; return *Map::instance; }
            MAP_UBO const& GetUniformBuffer();
            void DestroyInstance();
            VkCommandBuffer BuildCommandBuffer(uint32_t swap_chain_image_index, VkFramebuffer frame_buffer);
            inline void SetDestination(Vector3 const& position) { this->destination_position = position; this->uniform_buffer.display_destination = 1; }
            inline void UnsetDestination() { this->uniform_buffer.display_destination = 0; }
            inline void SetSelection(float const* position) { this->selected_position = position; this->uniform_buffer.display_selection = 1; }
            inline void UnsetSelection() { this->uniform_buffer.display_selection = 0; }

        private :

            static Map* instance;
            std::vector<VkCommandBuffer> command_buffers;
            ManagedBuffer map_data_buffer;
            Renderer renderer;
            MAP_UBO uniform_buffer;

            const float* selected_position = nullptr;
            Vector3 destination_position;

            VkDeviceSize index_buffer_offet = 0;

            Map();
            ~Map();

            void UpdateVertexBuffer();
    };
}