#pragma once

#include <thread>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "vulkan/vulkan.h"
#include "../../Platform/Common/Window/Window.h"
#include "../../Tools/Tools.h"
#include "../../Tools/Matrix/Maths.h"

#define ENGINE_VERSION VK_MAKE_VERSION(0, 0, 1)
#define ENGINE_NAME "KazEngine"

#define BACKGROUND_STAGING_BUFFER_SIZE 1024 * 1024 * 20     // 20 Mo (de quoi accueillir un gros mesh ou une grosse texture)
#define MAX_CONCURRENT_TEXTURES 50                          // Taille du descriptor pool

#if defined(_DEBUG)
#include <iostream>
#endif

namespace Engine
{
    // Trick utilisé pour déclarer toutes les fonction Vulkan
    // Source : https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1
    #define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
    #define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
    #define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
    #define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
    #include "ListOfFunctions.inl"

    class Vulkan
    {
        public:

            enum RETURN_CODE : uint8_t {
                SUCCESS                                 = 0,
                VULKAN_ALREADY_INITIALIZED              = 1,
                LOAD_LIBRARY_FAILURE                    = 2,
                VULKAN_INSTANCE_CREATION_FAILURE        = 3,
                LOAD_EXPORTED_FUNCTIONS_FAILURE         = 4,
                LOAD_GLOBAL_FUNCTIONS_FAILURE           = 5,
                LOAD_INSTANCE_FUNCTIONS_FAILURE         = 6,
                DEVICE_CREATION_FAILURE                 = 7,
                PRESENTATION_SURFACE_CREATION_FAILURE   = 8,
                LOAD_DEVICE_FUNCTIONS_FAILURE           = 9,
                SWAP_CHAIN_CREATION_FAILURE             = 10,
                RENDER_PASS_CREATION_FAILURE            = 16,
                GRAPHICS_PIPELINE_CREATION_FAILURE      = 17,
                MAIN_THREAD_INITIALIZATION_FAILURE      = 18,
                SHARED_RESOURCES_INITIALIZATION_FAILURE = 19,
                BACKGROUND_INITIALIZATION_FAILURE       = 20,
                DESCRIPTOR_SETS_PREPARATION_FAILURE     = 21,
                THREAD_INITIALIZATION_FAILURE           = 22,
                FRAME_BUFFERS_CREATION_FAILURE          = 23
            };

            // Structure utilisée pour le contenu du vertex buffer
            struct VERTEX {
                Vector4 vertex;
                TexCood texture_coordinates;

                VERTEX() : vertex({}), texture_coordinates({}) {}
                VERTEX(Vector4 v, TexCood t) : vertex(v), texture_coordinates(t) {}
            };

            static Vulkan* GetInstance();                                                                                           // Récupération de l'instance du singleton
            static void DestroyInstance();                                                                                          // Libération des ressources allouées à Vulkan
            RETURN_CODE Initialize(Engine::Window* draw_window, uint32_t application_version, std::string aplication_name);         // Initialisation de Vulkan
            uint32_t CreateTexture(std::vector<unsigned char> data, uint32_t width, uint32_t height);                               // Copie d'une texture dans la carte graphique
            uint32_t CreateVertexBuffer(std::vector<VERTEX>& data);                                                                 // Création d'un vertex buffer
            void Draw();                                                                                                            // Boucle d'affichage principale
            uint32_t CreateModel(uint32_t vertex_buffer_id, uint32_t texture_id);                                                   // Création d'un model

        private:

            ////////////////////
            // HERPER STRUCTS //
            ////////////////////

            struct PHYSICAL_DEVICE {
                VkPhysicalDevice handle;
                VkPhysicalDeviceMemoryProperties memory;
                VkPhysicalDeviceProperties properties;

                PHYSICAL_DEVICE() : handle(VK_NULL_HANDLE), memory({}), properties({}) {}
            };

            struct DEVICE_QUEUE {
                uint32_t index;
                VkQueue handle;

                DEVICE_QUEUE() : index(0), handle(VK_NULL_HANDLE) {}
            };

            struct SWAP_CHAIN_IMAGE {
                VkImage handle;
                VkImageView view;

                SWAP_CHAIN_IMAGE() : handle(VK_NULL_HANDLE), view(VK_NULL_HANDLE) {}
            };

            struct SWAP_CHAIN {
                VkSwapchainKHR handle;
                VkFormat format;
                std::vector<SWAP_CHAIN_IMAGE> images;

                SWAP_CHAIN() : handle(VK_NULL_HANDLE), format(VK_FORMAT_UNDEFINED) {}
            };

            struct PIPELINE {
                VkPipelineLayout layout;
                VkPipeline handle;

                PIPELINE() : layout(VK_NULL_HANDLE), handle(VK_NULL_HANDLE) {}
            };

            struct COMMAND_BUFFER {
                VkCommandBuffer handle;
                VkFence fence;
                bool opened;

                COMMAND_BUFFER() : handle(VK_NULL_HANDLE), fence(VK_NULL_HANDLE), opened(false) {}
            };

            struct STAGING_BUFFER {
                VkBuffer handle;
                VkDeviceMemory memory;
                void* pointer;
                VkDeviceSize size;

                STAGING_BUFFER() : handle(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), pointer(nullptr), size(0) {}
            };

            struct UNIFORM_BUFFER {
                VkBuffer handle;
                VkDeviceMemory memory;
                VkDeviceSize size;
                VkDeviceSize offset;

                UNIFORM_BUFFER() : handle(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), size(0), offset(0) {}
            };

            struct VERTEX_BUFFER {
                VkBuffer handle;
                VkDeviceMemory memory;
                VkDeviceSize size;
                uint32_t vertex_count;

                VERTEX_BUFFER() : handle(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), size(0), vertex_count(0) {}
            };

            struct MAIN_THREAD_RESOURCES {
                VkFramebuffer framebuffer;
                VkSemaphore swap_chain_semaphore;
                VkSemaphore renderpass_semaphore;
                VkSemaphore transfer_semaphore;
                COMMAND_BUFFER graphics_command_buffer;
                COMMAND_BUFFER transfer_command_buffer;

                MAIN_THREAD_RESOURCES() : framebuffer(VK_NULL_HANDLE), swap_chain_semaphore(VK_NULL_HANDLE), renderpass_semaphore(VK_NULL_HANDLE), transfer_semaphore(VK_NULL_HANDLE) {}
            };

            struct TEXTURE {
                VkImage image;
                VkDeviceMemory memory;
                VkImageView view;
                VkSampler sampler;
                std::vector<VkDescriptorSet> descriptors;

                TEXTURE() : image(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), view(VK_NULL_HANDLE), sampler(VK_NULL_HANDLE) {}
            };

            struct BACKGROUD_TASKS_RESOURCES {
                VkCommandPool transfer_command_pool;
                VkCommandPool graphics_command_pool;
                COMMAND_BUFFER transfer_command_buffer;
                COMMAND_BUFFER graphics_command_buffer;
                STAGING_BUFFER staging;
                DEVICE_QUEUE graphics_queue;
                DEVICE_QUEUE transfer_queue;
                VkSemaphore semaphore;

                BACKGROUD_TASKS_RESOURCES() : transfer_command_pool(VK_NULL_HANDLE), graphics_command_pool(VK_NULL_HANDLE), semaphore(VK_NULL_HANDLE) {}
            };

            struct MODEL {
                uint32_t vertex_buffer_id;
                uint32_t texture_id;
            };

            struct TRANSFORMATION {
                Matrix4x4 translation;
                Matrix4x4 rotation_x;
                Matrix4x4 rotation_y;
                Matrix4x4 rotation_z;

                TRANSFORMATION() : translation(IDENTITY_MATRIX), rotation_x(IDENTITY_MATRIX), rotation_y(IDENTITY_MATRIX), rotation_z(IDENTITY_MATRIX) {}
            };

            struct ENTITY {
                TRANSFORMATION transformation;
                uint32_t vertex_count;
                VkBuffer vertex_buffer;
                VkDescriptorSet descriptor_set;
                uint32_t ubo_offset;

                ENTITY() : vertex_count(0), vertex_buffer(VK_NULL_HANDLE), descriptor_set(VK_NULL_HANDLE) {}
            };

            struct CONDITION_VARIABLE {
                std::condition_variable condition;
                std::mutex mutex;
                bool signaled;

                CONDITION_VARIABLE() : signaled(false) {}
            };

            struct THREAD_SHARED_RESOURCES {
                STAGING_BUFFER staging;
                UNIFORM_BUFFER uniform;
                std::vector<ENTITY> entities;
            };

            struct THREAD_HANDLER {
                std::thread handle;
                VkCommandPool command_pool;
                std::vector<COMMAND_BUFFER> graphics_command_buffer;

                THREAD_HANDLER() : command_pool(VK_NULL_HANDLE) {}
            };

            /////////////
            // MEMBERS //
            /////////////

            // Instance du singleton
            static Vulkan* vulkan;

            // Librairie Vulkan
            LibraryHandle library;

            // Instance vulkan
            VkInstance instance;

            // Fenêtre d'affichage
            Window* draw_window;
            Surface draw_surface;
            VkSurfaceKHR presentation_surface;

            // Périphérique physique choisi pour l'affichage
            PHYSICAL_DEVICE physical_device;

            // Queue families
            DEVICE_QUEUE present_queue;
            DEVICE_QUEUE graphics_queue;
            DEVICE_QUEUE transfer_queue;

            // Alignement des UBO
            uint32_t ubo_alignement;

            // Logical device
            VkDevice device;

            // Swap Chain
            bool creating_swap_chain;
            SWAP_CHAIN swap_chain;
            uint8_t concurrent_frame_count;

            // Render Pass
            VkRenderPass render_pass;

            // Pipeline
            PIPELINE graphics_pipeline;

            // Descriptor Sets
            VkDescriptorSetLayout descriptor_set_layout;
            VkDescriptorPool descriptor_pool;

            // Main Thread Resources
            std::vector<MAIN_THREAD_RESOURCES> main;
            VkCommandPool graphics_command_pool;
            VkCommandPool transfer_command_pool;
            uint8_t current_frame_index;

            // Shared Thread Resources
            std::vector<THREAD_SHARED_RESOURCES> shared;

            // Background Tasks
            BACKGROUD_TASKS_RESOURCES background;                           // Ressources utilisées pour les tâches asynchrones comme le chargement de textures ou de vertext buffers

            // Texture 
            uint32_t last_texture_index;                                    // Index de la prochaine texture à allouer
            std::unordered_map<uint32_t, TEXTURE> textures;                 // Structure de stockage pour les textures créées

            // Vertex
            uint32_t last_vbo_index;                                        // Index du prochain vertex buffer à allouer
            std::unordered_map<uint32_t, VERTEX_BUFFER> vertex_buffers;     // Structure de stockage pour les vertex buffers créés

            // Model
            uint32_t last_model_index;                                      // Index du prochain model à allouer
            std::unordered_map<uint32_t, MODEL> models;                     // Ensemble de models

            // Rendu
            Matrix4x4 base_projection;                                      // Matrice de projection précalculée

            // Threads
            std::vector<THREAD_HANDLER> threads;
            uint32_t thread_count;
            CONDITION_VARIABLE thread_sleep;
            CONDITION_VARIABLE frame_ready;
            std::mutex pick_work;
            std::mutex open_buffer;
            volatile bool keep_thread_alive;
            uint32_t pick_index;
            uint32_t job_done_count;

            ////////////////////
            // CORE FUNCTIONS //
            ////////////////////

            Vulkan();
            ~Vulkan();
            bool LoadPlatformLibrary();                                                                         // Chargement de la librairie vulklan
            bool GetVersion(std::string &version_string);                                                       // Récupère la version de vulkan
            bool CreateVulkanInstance(uint32_t application_version, std::string& aplication_name);              // Création de l'instance vulkan
            bool LoadExportedEntryPoints();                                                                     // Chargement de l'exporteur de fonctions vulkan
            bool LoadGlobalLevelEntryPoints();                                                                  // Chargement des fonctions vulkan de portée globale
            bool LoadInstanceLevelEntryPoints();                                                                // Chargement des fonctions vulkan portant sur l'instance
            bool LoadDeviceLevelEntryPoints();                                                                  // Chargement des fonctions vulkan portant sur le logical device
            bool CreatePresentationSurface();                                                                   // Création de la surface de présentation
            bool CreateDevice(char preferred_device_index = -1);                                                // Création du logical device
            void GetDeviceQueues();                                                                             // Récupère le handle des device queues
            bool CreateSwapChain();                                                                             // Création de la swap chain
            bool CreateRenderPass();                                                                            // Création de la render pass
            bool CreatePipeline(bool dynamic_viewport = false);                                                 // Création du pipeline
            bool CreateStagingBuffer(STAGING_BUFFER& staging_buffer, VkDeviceSize size);                        // Création d'un staging buffer
            bool CreateUniformBuffer(UNIFORM_BUFFER& uniform_buffer, VkDeviceSize size);                        // Création d'un uniform buffer
            bool InitMainThread();                                                                              // Allocation des resources de la boucle principale
            void ReleaseMainThread();                                                                           // Libère les ressources de la boucle principale
            bool InitThreadSharedResources();                                                                   // Allocation des resources partagées
            void ReleaseThreadSharedResources();                                                                // Libère les ressources partagées
            bool OnWindowSizeChanged();                                                                         // Callback appelée lors du redimensiontionnement de la fenêtre
            bool InitBackgroundResources();                                                                     // Allocation des ressources d'arrière plan
            void ReleaseBackgroundResources();                                                                  // Libère les ressources d'arrière plan
            bool PrepareDescriptorSets();                                                                       // Création du descriptor pool
            bool UpdateEntities(uint32_t frame_index);                                                          // Mise à jour des transformations des entités
            bool InitThreads();                                                                                 // Initialisation des threads
            static void ThreadJob(uint32_t thread_id);                                                          // Fonction de traitement exécutée par les threads
            void ReleaseThreads();                                                                              // Arrête les threads et libère les ressources associées
            bool CreateFrameBuffers();                                                                          // Création des frame buffers

            //////////////////////
            // HELPER FUNCTIONS //
            //////////////////////

            // Logical Device
            std::vector<VkQueueFamilyProperties> QueryDeviceProperties(VkPhysicalDevice test_physical_device);
            bool IsDeviceEligible(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties);
            uint32_t SelectPresentQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties);
            uint32_t SelectPreferredQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties, VkQueueFlagBits queue_type, uint32_t common_queue);

            // Swap Chain
            VkExtent2D GetSwapChainExtent(VkSurfaceCapabilitiesKHR surface_capabilities);
            uint32_t GetSwapChainNumImages(VkSurfaceCapabilitiesKHR surface_capabilities);
            VkFormat GetSurfaceFormat();
            VkSurfaceTransformFlagBitsKHR GetSwapChainTransform(VkSurfaceCapabilitiesKHR surface_capabilities);
            VkCompositeAlphaFlagBitsKHR GetSwapChainCompositeAlpha(VkSurfaceCapabilitiesKHR surface_capabilities);
            VkPresentModeKHR GetSwapChainPresentMode();

            // Others
            bool MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, uint32_t &type_index);
            bool AllocateCommandBuffer(VkCommandPool& pool, uint32_t count, std::vector<VkCommandBuffer>& buffer, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            bool CreateCommandBuffer(VkCommandPool pool, std::vector<COMMAND_BUFFER>& command_buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            bool CreateSemaphore(VkSemaphore &semaphore);
            bool CreateCommandPool(VkCommandPool& pool, uint32_t queue_family_index);

            ///////////////////////
            // VALIDATION LAYERS //
            ///////////////////////

            #if defined(_DEBUG)
            VkDebugUtilsMessengerEXT report_callback;
            //VkDebugReportCallbackEXT report_callback;
            void CreateDebugReportCallback();
            #endif
    };
}