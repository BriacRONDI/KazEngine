#pragma once

#include "../Vulkan/Vulkan.h"
#include "../Scene/Map/Map.h"
#include "../ManagedBuffer/ManagedBuffer.h"
#include "../Camera/Camera.h"
#include "../Scene/EntityRender/EntityRender.h"
#include "../DataBank/DataBank.h"
#include "../Platform/Common/Timer/Timer.h"
#include "../UserInterface/UserInterface.h"

#if defined(DISPLAY_LOGS)
#include <iostream>
#endif

#define MULTI_USAGE_BUFFER_MASK VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT


namespace Engine
{
    class Core : public IWindowListener, public IUserInteraction
    {
        public :

            inline ~Core() { this->Clear(); }
            void Clear();
            bool Initialize();
            void Loop();
            inline EntityRender& GetEntityRender() { return *this->entity_render; }

            /////////////////////
            // IWindowListener //
            /////////////////////

            virtual inline void StateChanged(E_WINDOW_STATE window_state) {};
            virtual void SizeChanged(Area<uint32_t> size);

            //////////////////////
            // IUserInteraction //
            //////////////////////

            virtual void SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end);
            virtual void ToggleSelection(Point<uint32_t> mouse_position);
            virtual void MoveToPosition(Point<uint32_t> mouse_position);

        private :

            // uint8_t current_frame_index;
            VkCommandPool graphics_command_pool;
            std::vector<Vulkan::RENDERING_RESOURCES> resources;
            Map* map;
            EntityRender* entity_render;
            UserInterface* user_interface;
            std::vector<VkSemaphore> swap_chain_semaphores;
            std::vector<Vulkan::COMMAND_BUFFER> transfer_buffers;

            bool AllocateRenderingResources();
            void DestroyRenderingResources();
            bool BuildRenderPass(uint32_t swap_chain_image_index);
            bool RebuildFrameBuffers();
    };
}