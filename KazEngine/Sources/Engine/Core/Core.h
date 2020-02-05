#pragma once

#include <chrono>

#include "../Graphics/Vulkan/Vulkan.h"
#include "../Graphics/Mesh/Mesh.h"
#include "../Graphics/Camera/Camera.h"
#include "./Entity/Entity.h"
#include "./Entity/SkeletonEntity/SkeletonEntity.h"
#include "./ManagedBuffer/ManagedBuffer.h"
#include "../Graphics/Renderer/Renderer.h"
#include "./ModelManager/ModelManager.h"
#include "../Graphics/Lighting/Lighting.h"

namespace Engine {

    class Entity;

    class Core {

        public:

            static constexpr uint32_t MODEL_BUFFER_SIZE = 1024 * 1024 * 10;
            static constexpr uint32_t WORK_BUFFER_SIZE = 1024 * 1024 * 1;
            static constexpr uint32_t STORAGE_BUFFER_SIZE = 1024 * 1024 * 10;

            enum SUB_BUFFER_TYPE : uint8_t {
                CAMERA_UBO          = 0,
                ENTITY_UBO          = 1,
                LIGHT_UBO           = 2,
                BONE_OFFSETS_UBO    = 3,
                META_SKELETON_UBO   = 4
            };

            // Camera
            Camera camera;

            // Lumières
            // Lighting lighting;

            // Gestionnaire de models
            ModelManager model_manager;

            // Buffers
            ManagedBuffer model_buffer;
            ManagedBuffer work_buffer;
            ManagedBuffer storage_buffer;

            Core();
            ~Core();
            bool Initialize();
            void Clear();
            void DrawScene();
            bool AddEntity(std::shared_ptr<Entity> entity);
            bool AddTexture(Tools::IMAGE_MAP const& image, std::string const& name, Renderer& renderer);
            bool AddSkeleton(std::string const& skeleton);

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

            // Device vulkan
            VkDevice device;

            // Gestion des descriptor sets
            DescriptorSetManager ds_manager;

            // Générateurs de rendu (pipelines)
            std::vector<Renderer> renderers;

            // Générateur de rendu permettant l'affichage des normales
            // Uniquement disponible en mode debug
            #if defined(DISPLAY_LOGS)
            Renderer normal_debug;
            #endif

            // Entités
            std::vector<std::shared_ptr<Entity>> entities;

            // Models chargés dans le vertex buffer
            std::vector<std::shared_ptr<Mesh>> meshes;

            // Skeletons
            std::map<std::string, SKELETAL_ANIMATION_SBO> skeletons;

            // Textures chargées en mémoire graphique
            std::map<std::string, Vulkan::IMAGE_BUFFER> textures;

            // Ressources nécessaires au fonctionnement de la boucle principale
            std::vector<Vulkan::RENDERING_RESOURCES> resources;
            MAIN_LOOP_RESOURCES loop_resources;

            // Indice de l'image en cours de construction
            uint8_t current_frame_index;

            // Buffers
            /*ManagedBuffer model_buffer;
            ManagedBuffer work_buffer;*/

            // Helpers
            bool AllocateRenderingResources();
            inline bool RebuildCommandBuffers();
            bool BuildCommandBuffer(uint32_t swap_chain_image_index);
            bool RebuildFrameBuffers();
    };
}