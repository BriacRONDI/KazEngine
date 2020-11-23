#pragma once

#include <Singleton.hpp>
#include <EventEmitter.hpp>
#include "../Vulkan/Vulkan.h"
#include "../GlobalData/GlobalData.h"
#include "../InstancedDescriptorSet/IInstancedDescriptorListener.h"
#include "../Camera/Camera.h"
#include "IUserInteraction.h"
#include "../DynamicEntityRenderer/DynamicEntityRenderer.h"

namespace Engine
{
    class UserInterface : public Singleton<UserInterface>, public IMouseListener, public IInstancedDescriptorListener, public Tools::EventEmitter<IUserInteraction>
    {
        friend Singleton<UserInterface>;

        public :

            struct MOUSE_SELECTION_SQUARE {
                Point<uint32_t> start;
                Point<uint32_t> end;
                uint32_t display;
            };

            bool Initialize();
            void Clear();
            void Refresh() { std::fill(this->refresh.begin(), this->refresh.end(), true); }
            bool DisplayInterface() { return this->selection_square.display > 0 && Camera::GetInstance()->IsRtsMode(); }
            VkCommandBuffer BuildCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer);
            void Update(uint8_t frame_index) { if(this->DisplayInterface()) GlobalData::GetInstance()->mouse_square_descriptor.WriteData(&this->selection_square, sizeof(MOUSE_SELECTION_SQUARE), 0, 0, frame_index); }

            //////////////////////////////
            // IMouseListener interface //
            //////////////////////////////

            void MouseMove(unsigned int x, unsigned int y);
            void MouseButtonDown(MOUSE_BUTTON button);
            void MouseButtonUp(MOUSE_BUTTON button);
            void MouseWheelUp() {};
            void MouseWheelDown() {};

            ////////////////////////////////////////////
            // IInstancedDescriptorListener interface //
            ////////////////////////////////////////////

            void InstancedDescriptorSetUpdated(InstancedDescriptorSet* descriptor, uint8_t binding) { this->Refresh(); }

        private :

            VkRenderPass render_pass;
            std::shared_ptr<Chunk> ui_vbo_chunk;
            vk::PIPELINE pipeline;
            VkCommandPool command_pool;
            std::vector<bool> refresh;
            std::vector<VkCommandBuffer> command_buffers;
            MOUSE_SELECTION_SQUARE selection_square;
            std::vector<bool> update_selection_square;

            UserInterface();
            ~UserInterface() { this->Clear(); }
            bool SetupRenderPass();
            bool SetupPipeline();
            bool SetupVertexBuffer();
    };
}