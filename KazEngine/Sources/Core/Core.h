#pragma once

#include <Singleton.hpp>
#include <Model.h>

#include "../Vulkan/Vulkan.h"
#include "../DynamicEntityRenderer/DynamicEntityRenderer.h"
#include "../Platform/Common/Timer/Timer.h"
#include "../LOD/LOD.h"
#include "../Camera/Camera.h"
#include "../GlobalData/GlobalData.h"
#include "../MappedDescriptorSet/IMappedDescriptorListener.h"
#include "../ComputeShader/ComputeShader.h"
#include "../UserInterface/UserInterface.h"
#include "../Map/Map.h"
#include "../MovementController/MovementController.h"

#if defined(DISPLAY_LOGS)
#include <thread>
#endif

namespace Engine
{
    class Core : public Singleton<Core>, public IMappedDescriptorListener, public IUserInteraction
    {
        friend Singleton<Core>;

        public :

            /// Initialize core instance
            bool Initialize();

            /// Main Loop
            void Loop();

            inline bool LoadTexture(Tools::IMAGE_MAP image, std::string name) { return GlobalData::GetInstance()->texture_descriptor.AllocateTexture(image, name); }
            bool LoadSkeleton(Model::Bone skeleton, std::string idle = {}, std::string move = {}, std::string attack = {});
            bool LoadModel(LODGroup& lod);
            bool AddToScene(DynamicEntity& entity);

            /////////////////////////////////////////
            // IMappedDescriptorListener interface //
            /////////////////////////////////////////

            void MappedDescriptorSetUpdated(MappedDescriptorSet* descriptor, uint8_t binding);

            ////////////////////////////////
            // IUserInteraction interface //
            ////////////////////////////////

            void SquareSelection(Point<uint32_t> box_start, Point<uint32_t> box_end);
            void ToggleSelection(Point<uint32_t> mouse_position);
            void MoveToPosition(Point<uint32_t> mouse_position){};

        private :

            struct RENDERING_RESOURCES {
                VkCommandBuffer command_buffer;
                VkFence fence;
                VkSemaphore draw_semaphore;

                RENDERING_RESOURCES() : command_buffer(nullptr), fence(nullptr), draw_semaphore(nullptr) {}
            };

            VkCommandPool command_pool;
            std::vector<RENDERING_RESOURCES> resources;
            std::vector<VkSemaphore> present_semaphores;
            std::vector<VkSemaphore> compute_semaphores_1;
            std::vector<VkSemaphore> compute_semaphores_2;
            std::vector<VkSemaphore> compute_semaphores_3;
            ComputeShader cull_lod_shader;
            ComputeShader movement_shader;
            ComputeShader collision_shader;
            ComputeShader init_group_shader;

            Core();
            ~Core();

            bool BuildRenderPass(uint32_t frame_index);
    };
}