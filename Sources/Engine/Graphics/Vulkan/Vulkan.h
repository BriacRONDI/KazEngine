#pragma once

#include <thread>
#include <mutex>
#include <array>
#include <unordered_map>
#include <chrono>

#include "vulkan/vulkan.h"
#include "../../Platform/Common/Window/Window.h"
#include "../../Tools/Tools.h"
#include "../../Tools/Matrix/Maths.h"
// #include "../Mesh/Mesh.h"

#if defined(_DEBUG)
#include <iostream>
#endif

#define ENGINE_VERSION VK_MAKE_VERSION(0, 0, 1)
#define ENGINE_NAME "KazEngine"

#define STAGING_BUFFER_SIZE 1024 * 1024 * 20

namespace Engine
{
    #define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
    #define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
    #define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
    #define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
    #include "ListOfFunctions.inl"

    class Vulkan
    {
        public:

            enum ERROR_MESSAGE : uint8_t {
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
                DEPTH_BUFFER_CREATION_FAILURE           = 11,
                STAGING_BUFFER_CREATION_FAILURE         = 12,
                VERTEX_BUFFER_CREATION_FAILURE          = 13,
                UNIFORM_BUFFER_CREATION_FAILURE         = 14,
                DESCRIPTOR_SETS_CREATION_FAILURE        = 15,
                RENDER_PASS_CREATION_FAILURE            = 16,
                GRAPHICS_PIPELINE_CREATION_FAILURE      = 17,
                COMMAND_BUFFERS_CREATION_FAILURE        = 18,
                SEMAPHORES_CREATION_FAILURE             = 19,
                FENCES_CREATION_FAILURE                 = 20,
                UNIFORM_BUFFER_UPDATE_FAILURE           = 21
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
            ERROR_MESSAGE Initialize(Engine::Window* draw_window, uint32_t application_version, std::string aplication_name);       // Initialisation de Vulkan
            void Start();
            void Stop();
            uint32_t CreateTexture(std::vector<unsigned char> data, uint32_t width, uint32_t height);                               // Copie d'une texture dans la carte graphique
            uint32_t CreateVertexBuffer(std::vector<VERTEX>& data);                                                                 // Création d'un vertex buffer
            uint32_t CreateMesh(uint32_t model_id);                                                                                 // Création d'un mesh
            static void ThreadLoop(Vulkan* self);                                                                                   // Boucle principale d'affichage

        private:

            // Le mesh utilise le VULKAN_BUFFER
            // friend class Mesh;

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

            struct DEPTH_BUFFER {
                VkImage image;
                VkImageView view;
                VkDeviceMemory memory;

                DEPTH_BUFFER() : image(VK_NULL_HANDLE), view(VK_NULL_HANDLE), memory(VK_NULL_HANDLE) {}
            };

            struct VULKAN_BUFFER {
                VkBuffer handle;
                VkDeviceMemory memory;
                void* pointer;
                VkDeviceSize size;

                VULKAN_BUFFER() : handle(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), pointer(nullptr), size(0) {}
            };

            struct PIPELINE {
                VkPipelineLayout layout;
                VkPipeline handle;

                PIPELINE() : layout(VK_NULL_HANDLE), handle(VK_NULL_HANDLE) {}
            };

            struct DESCRIPTOR_POOL {
                VkDescriptorSetLayout layout;
                VkDescriptorPool pool;

                DESCRIPTOR_POOL() : layout(VK_NULL_HANDLE), pool(VK_NULL_HANDLE) {}
            };

            struct TRANSFER_RESOURCE {
                VkCommandPool command_pool;
                VkCommandBuffer command_buffer;
                VkFence fence;

                TRANSFER_RESOURCE() : command_pool(VK_NULL_HANDLE), command_buffer(VK_NULL_HANDLE), fence(VK_NULL_HANDLE) {}
            };

            struct RENDERING_RESOURCE {
                VkFramebuffer framebuffer;
                VkCommandBuffer command_buffer;
                VkSemaphore swap_chain_semaphore;
                VkSemaphore render_pass_semaphore;
                VkFence fence;
                VkDescriptorSet descriptor_set;

                RENDERING_RESOURCE() :
                    framebuffer(VK_NULL_HANDLE),
                    command_buffer(VK_NULL_HANDLE),
                    swap_chain_semaphore(VK_NULL_HANDLE),
                    render_pass_semaphore(VK_NULL_HANDLE),
                    fence(VK_NULL_HANDLE),
                    descriptor_set(VK_NULL_HANDLE) {
                }
            };

            struct TEXTURE {
                VkImage image;
                VkDeviceMemory memory;
                VkImageView view;
                VkSampler sampler;

                TEXTURE() : image(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), view(VK_NULL_HANDLE), sampler(VK_NULL_HANDLE) {}
            };

            struct MESH {
                Matrix4x4 transformations;
                uint32_t vertex_buffer_index;
                uint32_t offset;

                MESH() : transformations(IDENTITY_MATRIX), vertex_buffer_index(UINT32_MAX), offset(UINT32_MAX) {}
            };

            /////////////
            // MEMBERS //
            /////////////

            // Instance du singleton
            static Vulkan* global_instance;

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
            uint32_t present_stack_index;
            uint32_t graphics_stack_index;
            uint32_t transfer_stack_index;
            std::vector<DEVICE_QUEUE> queue_stack;

            // Alignement des UBO
            uint32_t ubo_alignement;

            // Logical device
            VkDevice device;

            // Swap Chain
            bool creating_swap_chain;
            SWAP_CHAIN swap_chain;

            // Buffers
            DEPTH_BUFFER depth_buffer;
            VULKAN_BUFFER staging_buffer;
            VULKAN_BUFFER uniform_buffer;

            // Base commune pour les descriptor sets
            DESCRIPTOR_POOL descriptor_pool;

            // Render Pass
            VkRenderPass render_pass;

            // Pipeline
            PIPELINE graphics_pipeline;

            // Graphics
            VkCommandPool graphics_pool;
            
            // Transfer
            TRANSFER_RESOURCE transfer;

            // Threads
            //unsigned int image_count;
            std::vector<RENDERING_RESOURCE> rendering;
            std::thread draw_thread;
            bool keep_runing;

            // Data
            uint32_t last_texture_index;                                    // Index de la prochaine texture à allouer
            uint32_t last_vbo_index;                                        // Index du prochain vertex buffer à allouer
            uint32_t last_mesh_index;                                       // Index du prochain mesh à allouer
            std::unordered_map<uint32_t, TEXTURE> textures;                 // Structure de stockage pour les textures créées
            std::unordered_map<uint32_t, VULKAN_BUFFER> vertex_buffers;     // Structure de stockage pour les vertex buffers créés
            // Matrix4x4 projection;                                           // Matrice de projection contenue dans le Uniform Buffer
            std::unordered_map<uint32_t, MESH> meshes;                      // Ensemble de meshes manipulables

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
            bool CreateDepthBuffer();                                                                           // Création du depth buffer
            bool CreateStagingBuffer(VkDeviceSize size);                                                        // Création du staging buffer
            bool CreateUniformBuffer();                                                                         // Création du uniform buffer
            bool CreateRenderPass();                                                                            // Création de la render pass
            bool CreateDescriptorSets();                                                                        // Création des descriptor sets
            bool CreatePipeline(bool dynamic_viewport = false);                                                 // Création du pipeline
            bool CreateCommandBuffers();                                                                        // Création des command buffers
            bool CreateSemaphores();                                                                            // Création des sémaphores
            bool CreateFences();                                                                                // Création des fences
            bool UpdateMeshUniformBuffers();                                                                    // Mise à jour du contenu des uniform buffers des mesh
            bool OnWindowSizeChanged();                                                                         // Reconstruction de la Swap Chain lorsque la fenêtre est redimensionnée

            //static void ThreadLoop(Vulkan* self);                                                     // Boucle principale d'affichage

            /////////////
            // HELPERS //
            /////////////

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

            // Buffers
            bool CreateCommandPool(VkCommandPool& pool, uint32_t queue_family_index);
            bool AllocateCommandBuffer(VkCommandPool& pool, uint32_t count, std::vector<VkCommandBuffer>& buffer);
            bool MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, uint32_t &type_index);
            bool UpdateVertexBuffer(std::vector<VERTEX>& data, VULKAN_BUFFER& vertex_buffer);                      // Mise à jour des données du vertex buffer

            #if defined(_DEBUG)
            VkDebugUtilsMessengerEXT report_callback;
            void CreateDebugReportCallback();
            #endif
    };
}
