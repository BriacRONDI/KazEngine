#pragma once

#include <chrono>

#include "../Graphics/Vulkan/Vulkan.h"
#include "../Graphics/Mesh/Mesh.h"
#include "../Graphics/Camera/Camera.h"
#include "./Entity/Entity.h"
#include "./ManagedBuffer/ManagedBuffer.h"
#include "../Graphics/Renderer/Renderer.h"
#include "./ModelManager/ModelManager.h"

namespace Engine {

    class Entity;

    class Core {

        public:

            static constexpr uint32_t MODEL_BUFFER_SIZE = 1024 * 1024 * 10;
            static constexpr uint32_t WORK_BUFFER_SIZE = 1024 * 1024 * 10;

            enum SUB_BUFFER_TYPE : uint8_t {
                CAMERA_UBO      = 0,
                ENTITY_UBO      = 1,
                SKELETON_UBO    = 2,
                MESH_UBO        = 3
            };

            // Camera
            Camera camera;

            // Gestionnaire de models
            ModelManager model_manager;

            // Buffers
            ManagedBuffer model_buffer;
            ManagedBuffer work_buffer;

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

            // Device vulkan
            VkDevice device;

            // Gestion des descriptor sets
            DescriptorSetManager ds_manager;

            // G�n�rateurs de rendu (pipelines)
            std::vector<Renderer> renderers;

            // G�n�rateur de rendu permettant l'affichage des normales
            // Uniquement disponible en mode debug
            #if defined(DISPLAY_LOGS)
            Renderer normal_debug;
            #endif

            // Entit�s
            std::vector<std::shared_ptr<Entity>> entities;

            // Models charg�s dans le vertex buffer
            std::vector<std::shared_ptr<Mesh>> meshes;

            // Skeletons
            std::map<std::string, uint32_t> skeletons;

            // Textures charg�es en m�moire graphique
            std::map<std::string, Vulkan::IMAGE_BUFFER> textures;

            // Ressources n�cessaires au fonctionnement de la boucle principale
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