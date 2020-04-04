#pragma once

#include "../Graphics/Vulkan/Vulkan.h"
#include "../Graphics/Mesh/Mesh.h"
#include "../Graphics/Camera/Camera.h"
#include "./Entity/Entity.h"
#include "./Entity/SkeletonEntity/SkeletonEntity.h"
#include "./ManagedBuffer/ManagedBuffer.h"
#include "../Graphics/Renderer/Renderer.h"
#include "./ModelManager/ModelManager.h"
#include "../Graphics/Lighting/Lighting.h"
#include "./Map/Map.h"

namespace Engine {

    class Entity;

    class Core : IMouseListener {

        public:

            static constexpr uint32_t MODEL_BUFFER_SIZE = 1024 * 1024 * 10;
            static constexpr uint32_t STORAGE_BUFFER_SIZE = 1024 * 1024 * 10;

            enum SUB_BUFFER_TYPE : uint8_t {
                CAMERA_UBO          = 0,
                ENTITY_UBO          = 1,
                LIGHT_UBO           = 2,
                BONE_OFFSETS_UBO    = 3,
                META_SKELETON_UBO   = 4,
                MAP_PROPERTIES      = 5
            };

            // Buffers
            ManagedBuffer model_buffer;
            ManagedBuffer work_buffer;
            ManagedBuffer storage_buffer;

            static Core& GetInstance();
            void DestroyInstance();
            bool Initialize(uint32_t start_entity_limit = 10000);
            void DrawScene();
            bool AddEntity(std::shared_ptr<Entity> entity, bool buld_command_buffers = true);
            bool AddSkeleton(std::string const& skeleton);
            bool RebuildCommandBuffers();
            inline uint32_t GetAnimationOffset(std::string const& skeleton, std::string const& animation) { return this->animations[skeleton][animation].offset; }
            
            // Séléction d'objet par la souris
            // http://schabby.de/picking-opengl-ray-tracing/
            void MousePick();
            void MoveToPosition();

            ///////////////////////////
            ///    IMouseListener    //
            ///////////////////////////

            virtual void MouseMove(unsigned int x, unsigned int y);
            virtual void MouseButtonDown(MOUSE_BUTTON button);
            virtual void MouseButtonUp(MOUSE_BUTTON button);
            virtual void MouseScrollUp();
            virtual void MouseScrollDown();

        private:

            struct MAIN_LOOP_RESOURCES {
                VkImageSubresourceRange image_subresource_range;
                uint32_t present_queue_family_index;
                uint32_t graphics_queue_family_index;
            };

            struct SKELETAL_ANIMATION_SBO {
                uint32_t offset;
                uint16_t frame_count;
                uint32_t bone_per_frame;
                uint32_t meta_skeleton_offset;
            };

            // Instance de singleton
            static Core* instance;

            // Device vulkan
            VkDevice device;

            // Générateurs de rendu (pipelines)
            std::vector<Renderer> renderers;

            // Générateur de rendu permettant l'affichage des normales
            // Uniquement disponible en mode debug
            #if defined(DISPLAY_LOGS)
            Renderer normal_debug;
            #endif

            // Entités
            std::vector<std::shared_ptr<Entity>> entities;

            // Entité sélectionnée
            std::shared_ptr<Entity> selected_entity;

            // Models chargés dans le vertex buffer
            std::vector<std::shared_ptr<Mesh>> meshes;

            // Animations
            std::map<std::string, std::map<std::string, SKELETAL_ANIMATION_SBO>> animations;

            // Textures chargées en mémoire graphique
            // std::map<std::string, Vulkan::IMAGE_BUFFER> textures;

            // Ressources nécessaires au fonctionnement de la boucle principale
            std::vector<Vulkan::RENDERING_RESOURCES> resources;
            MAIN_LOOP_RESOURCES loop_resources;
            std::vector<VkCommandBuffer> main_render_pass_command_buffers;

            // Indice de l'image en cours de construction
            uint8_t current_frame_index;

            // Singleton
            Core();
            ~Core();

            // Helpers
            bool AllocateRenderingResources();
            bool BuildRenderPass(uint32_t swap_chain_image_index);
            bool RebuildFrameBuffers();
            bool BuildCommandBuffer(uint32_t swap_chain_image_index);
    };
}