#pragma once

#include <vulkan/vulkan.h>
#include <Maths.h>
#include <Tools.h>
#include "../Platform/Common/Window/Window.h"
#include "../ManagedBuffer/Chunk/Chunk.h"

#define ENGINE_VERSION VK_MAKE_VERSION(0, 0, 1)
#define ENGINE_NAME "KazEngine"

// #define STAGING_BUFFER_SIZE  1024 * 1024 * 5   // 5 Mo

#if defined(DISPLAY_LOGS)
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

    class Vulkan : IWindowListener
    {
        public:

            ////////////////////////////////////////
            //       Code de retour renvoyé       //
            // lors de l'initialisation de vulkan //
            ////////////////////////////////////////
            
            enum INIT_RETURN_CODE : uint8_t {
                SUCCESS                                     = 0,
                VULKAN_ALREADY_INITIALIZED                  = 1,
                LOAD_LIBRARY_FAILURE                        = 2,
                VULKAN_INSTANCE_CREATION_FAILURE            = 3,
                LOAD_EXPORTED_FUNCTIONS_FAILURE             = 4,
                LOAD_GLOBAL_FUNCTIONS_FAILURE               = 5,
                LOAD_INSTANCE_FUNCTIONS_FAILURE             = 6,
                DEVICE_CREATION_FAILURE                     = 7,
                PRESENTATION_SURFACE_CREATION_FAILURE       = 8,
                LOAD_DEVICE_FUNCTIONS_FAILURE               = 9,
                SWAP_CHAIN_CREATION_FAILURE                 = 10,
                DESCRIPTOR_SETS_PREPARATION_FAILURE         = 11,
                DEPTH_FORMAT_SELECTION_FAILURE              = 12,
                RENDER_PASS_CREATION_FAILURE                = 13,
                DEPTH_BUFFER_CREATION_FAILURE               = 14,
                BACKGROUND_RESOURCES_INITIALIZATION_FAILURE = 15,
                DEPTH_BUFFER_LAYOUT_UPDATE_FAILURE          = 16,
                MAIN_THREAD_INITIALIZATION_FAILURE          = 17,
                PIPELINES_CREATION_FAILURE                  = 18,
                TRANSFER_ALLOCATION_FAILURE                 = 19
            };

            ////////////////////
            // HERPER STRUCTS //
            ////////////////////

            struct COMMAND_BUFFER {
                VkCommandBuffer handle;
                VkFence fence;

                COMMAND_BUFFER() : handle(nullptr), fence(nullptr) {}
            };

            struct DATA_BUFFER {
                VkBuffer handle;
                VkDeviceMemory memory;
                VkDeviceSize size;

                DATA_BUFFER() : handle(nullptr), memory(nullptr), size(0) {}
                inline void Clear() {
                    if(this->handle != nullptr) vkDestroyBuffer(Vulkan::GetDevice(), this->handle, nullptr);
                    if(this->memory != nullptr) vkFreeMemory(Vulkan::GetDevice(), this->memory, nullptr); // vkUnmapMemory is implicit
                    *this = {};
                }
            };

            struct STAGING_BUFFER : DATA_BUFFER {
                char* pointer;

                STAGING_BUFFER() : DATA_BUFFER(), pointer(nullptr) {}
            };

            struct IMAGE_BUFFER {
                VkImage handle;
                VkDeviceMemory memory;
                VkImageView view;
                VkFormat format;
                VkImageAspectFlags aspect;

                IMAGE_BUFFER() : handle(nullptr), memory(nullptr), view(nullptr), format(VK_FORMAT_UNDEFINED), aspect(0) {}
            };

            struct SWAP_CHAIN_IMAGE {
                VkImage handle;
                VkImageView view;

                SWAP_CHAIN_IMAGE() : handle(nullptr), view(nullptr) {}
            };

            struct SWAP_CHAIN {
                VkSwapchainKHR handle;
                VkFormat format;
                std::vector<SWAP_CHAIN_IMAGE> images;

                SWAP_CHAIN() : handle(nullptr), format(VK_FORMAT_UNDEFINED) {}
            };

            struct DEVICE_QUEUE {
                uint32_t index;
                VkQueue handle;

                DEVICE_QUEUE() : index(0), handle(nullptr) {}
            };

            struct RENDERING_RESOURCES {
                VkFramebuffer framebuffer;
                VkSemaphore semaphore;
                VkFence fence;
                std::vector<VkCommandBuffer> comand_buffers;
                // Vulkan::COMMAND_BUFFER graphics_command_buffer;

                RENDERING_RESOURCES() : framebuffer(nullptr), semaphore(nullptr) {}
            };

            struct PIPELINE {
                VkPipelineLayout layout;
                VkPipeline handle;

                PIPELINE() : layout(nullptr), handle(nullptr) {}
                inline void Clear() {
                    if(this->handle != nullptr) vkDestroyPipeline(Vulkan::GetDevice(), this->handle, nullptr);
                    if(this->layout != nullptr) vkDestroyPipelineLayout(Vulkan::GetDevice(), this->layout, nullptr);
                    *this = {};
                }
            };

            enum VERTEX_BINDING_ATTRIBUTE : uint8_t {
                POSITION        = 0,
                UV              = 1,
                COLOR           = 2,
                NORMAL          = 3,
                BONE_WEIGHTS    = 4,
                BONE_IDS        = 5,
                POSITION_2D     = 6,
                MATRIX          = 7,
                UINT_ID         = 8
            };

            /*struct DATA_CHUNK {
                VkDeviceSize offset;
                VkDeviceSize range;
            };*/

            ///////////////////////////////
            // Interface IWindowListener //
            ///////////////////////////////

            void StateChanged(IWindowListener::E_WINDOW_STATE window_state);
            void SizeChanged(Area<uint32_t> size);

            ///////////////////////////
            // FONCTIONS PRINCIPALES //
            ///////////////////////////

            static std::vector<VkVertexInputAttributeDescription> CreateVertexInputDescription(
                std::vector<std::vector<VERTEX_BINDING_ATTRIBUTE>> attributes,
                std::vector<VkVertexInputBindingDescription> &descriptions);

            static inline Vulkan& CreateInstance() { if(Vulkan::vulkan == nullptr) Vulkan::vulkan = new Vulkan; return *Vulkan::vulkan; }
            static inline bool HasInstance() { return Vulkan::vulkan != nullptr; }                                          // Indique si l'instance vulkan existe
            static inline Vulkan& GetInstance() { return *Vulkan::vulkan; }                                                 // Récupération de l'instance du singleton
            static void DestroyInstance();                                                                                  // Libération des ressources allouées à Vulkan
            bool GetVersion(std::string &version_string);                                                                   // Récupère la version de Vulkan
            static inline VkDevice& GetDevice(){ return Vulkan::vulkan->device; }                                           // Récupère le device de l'instance vulkan
            static inline Surface& GetDrawSurface() { return Vulkan::vulkan->draw_surface; }                                // Récupère la surface d'affichage
            static inline VkRenderPass GetRenderPass() { return Vulkan::vulkan->render_pass; }                              // Récupère la render pass
            static inline uint8_t GetConcurrentFrameCount() { return Vulkan::vulkan->concurrent_frame_count; }              // Récupère le nombre d'images de la swapchain
            static inline SWAP_CHAIN& GetSwapChain() { return Vulkan::vulkan->swap_chain; }                                 // Récupère la swap chain
            static inline DEVICE_QUEUE& GetGraphicsQueue() { return Vulkan::vulkan->graphics_queue; }                       // Récupère la graphics queue
            static inline DEVICE_QUEUE& GetPresentQueue() { return Vulkan::vulkan->present_queue; }                         // Récupère la present queue
            static inline DEVICE_QUEUE& GetTransferQueue() { return Vulkan::vulkan->transfer_queue; }                       // Récupère la transfer queue
            static inline VkFormat GetDepthFormat() { return Vulkan::vulkan->depth_format; }
            uint32_t ComputeUniformBufferAlignment(uint32_t buffer_size);                                                   // Calcule l'alignement correspondant à un Uniform Buffer
            uint32_t ComputeStorageBufferAlignment(uint32_t buffer_size);
            VkDeviceSize ComputeRawDataAlignment(size_t data_size);                                                         // Calcule le segment de mémoire occupé par une donnée en tenant compte du nonCoherentAtomSize
            bool AcquireNextImage(uint32_t& swapchain_image_index, VkSemaphore semaphore);
            bool PresentImage(RENDERING_RESOURCES& rendering_resource, VkSemaphore semaphore, uint32_t swap_chain_image_index);

            bool AllocateCommandBuffer(VkCommandPool& pool, std::vector<VkCommandBuffer>& buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            VkFence CreateFence();

            bool OnWindowSizeChanged();

            // Récupère les limites de la carte graphique
            static inline VkPhysicalDeviceLimits const& GetDeviceLimits() { return Vulkan::vulkan->physical_device.properties.limits; }
            static inline VkDeviceSize UboAlignment() { return Vulkan::vulkan->physical_device.properties.limits.minUniformBufferOffsetAlignment; }
            static inline VkDeviceSize SboAlignment() { return Vulkan::vulkan->physical_device.properties.limits.minStorageBufferOffsetAlignment; }

            // Création d'instance et chargement des fonctions principales
            INIT_RETURN_CODE Initialize(Engine::Window* draw_window, uint32_t application_version, std::string const& aplication_name, bool separate_transfer_queue = true);

            // Chargement d'un shader
            VkPipelineShaderStageCreateInfo LoadShaderModule(std::string const& filename, VkShaderStageFlagBits type);

            // Création d'un frame buffer
            bool CreateFrameBuffer(VkFramebuffer& frame_buffer, VkImageView const view);

            bool CreateCommandPool(VkCommandPool& pool, uint32_t queue_family_index, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

            // Création d'un sémaphore
            bool CreateSemaphore(VkSemaphore &semaphore);

            // Création d'un command buffer
            bool CreateCommandBuffer(VkCommandPool pool, std::vector<COMMAND_BUFFER>& command_buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, bool create_fence = true);

            void ReleaseCommandBuffer(VkCommandPool pool, std::vector<COMMAND_BUFFER>& command_buffers);

            // Envoi de données vers un data buffer
            size_t SendToBuffer(DATA_BUFFER& buffer, COMMAND_BUFFER const& command_buffer, STAGING_BUFFER staging_buffer, VkDeviceSize data_size, VkDeviceSize destination_offset);
            size_t SendToBuffer(DATA_BUFFER& buffer, COMMAND_BUFFER const& command_buffer, STAGING_BUFFER staging_buffer, std::vector<Chunk> chunks);
            // Envoi d'une image vers un buffer d'image
            // bool SendToBuffer(IMAGE_BUFFER& buffer, const void* data, VkDeviceSize data_size, uint32_t width, uint32_t height);
            size_t SendToBuffer(IMAGE_BUFFER& buffer, Tools::IMAGE_MAP const& image);
            bool MoveData(DATA_BUFFER& buffer, COMMAND_BUFFER const& command_buffer, VkDeviceSize data_size, VkDeviceSize source_offset, VkDeviceSize destination_offset);

            // Applique une birrière de changement de queue family à un data buffer
            /*bool TransitionBufferQueueFamily(
                              DATA_BUFFER& buffer, COMMAND_BUFFER command_buffer, DEVICE_QUEUE queue,
                              VkAccessFlags source_mask, VkAccessFlags dest_mask,
                              uint32_t source_queue_index, uint32_t dest_queue_index,
                              VkPipelineStageFlags source_stage, VkPipelineStageFlags dest_stage);*/

            // Création d'un buffer d'image
            IMAGE_BUFFER CreateImageBuffer(VkImageUsageFlags usage, VkImageAspectFlags aspect, uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);

            bool TransitionImageLayout(IMAGE_BUFFER& buffer,
                                       VkAccessFlags source_mask, VkAccessFlags dest_mask,
                                       VkImageLayout old_layout, VkImageLayout new_layout,
                                       VkPipelineStageFlags source_stage, VkPipelineStageFlags dest_stage);     // Change le layout d'un buffer d'image

            // Création d'un buffer de données
            bool CreateDataBuffer(DATA_BUFFER& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkFlags requirement, std::vector<uint32_t> const& queue_families = {});

            // Destruction d'un buffer de données
            void ReleaseDataBuffer(DATA_BUFFER& buffer);

            // Création d'une pipeline
            bool CreatePipeline(bool dynamic_viewport,
                                std::vector<VkDescriptorSetLayout> const& descriptor_set_layouts,
                                std::vector<VkPipelineShaderStageCreateInfo> const& shader_stages,
                                std::vector<VkVertexInputBindingDescription> const& vertex_binding_description,
                                std::vector<VkVertexInputAttributeDescription> const& vertex_attribute_descriptions,
                                std::vector<VkPushConstantRange> const& push_constant_ranges,
                                PIPELINE& pipeline,
                                VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL,
                                VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                bool blend_state = false);

            bool CreateComputePipeline(VkPipelineShaderStageCreateInfo stage,
                                       std::vector<VkDescriptorSetLayout> const& descriptor_set_layouts,
                                       std::vector<VkPushConstantRange> const& push_constant_ranges,
                                       PIPELINE& pipeline);

            inline Window* GetDrawWindow() { return this->draw_window; }

        private:

            ////////////////////
            // HERPER STRUCTS //
            ////////////////////

            struct PHYSICAL_DEVICE {
                VkPhysicalDevice handle;
                VkPhysicalDeviceMemoryProperties memory;
                VkPhysicalDeviceProperties properties;
                VkPhysicalDeviceFeatures features;

                PHYSICAL_DEVICE() : handle(nullptr), memory({}), properties({}) {}
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

            // Logical device
            VkDevice device;

            // Swap Chain
            bool creating_swap_chain;
            SWAP_CHAIN swap_chain;
            uint8_t concurrent_frame_count;
            VkImageSubresourceRange image_subresource_range;

            // Depth Buffer
            IMAGE_BUFFER depth_buffer;
            VkFormat depth_format;

            // Render Pass
            VkRenderPass render_pass;

            // Ressources utilisées pour les créations de buffers, copies de données et transitions de layouts
            // VkCommandPool main_command_pool;
            // COMMAND_BUFFER main_command_buffer;
            // DATA_BUFFER staging_buffer;
            // void* staging_pointer;
            VkCommandPool transfert_command_pool;
            COMMAND_BUFFER transfer_command_buffer;
            STAGING_BUFFER transfer_staging;

            ////////////////////
            // CORE FUNCTIONS //
            ////////////////////

            Vulkan();       // Constructeur
            ~Vulkan();      // Destructeur

            bool LoadPlatformLibrary();                                                                         // Chargement de la librairie vulkan

            bool LoadExportedEntryPoints();                                                                     // Chargement de l'exporteur de fonctions vulkan
            bool LoadGlobalLevelEntryPoints();                                                                  // Chargement des fonctions vulkan de portée globale
            bool LoadInstanceLevelEntryPoints();                                                                // Chargement des fonctions vulkan portant sur l'instance
            bool LoadDeviceLevelEntryPoints();                                                                  // Chargement des fonctions vulkan portant sur le logical device

            bool CreateVulkanInstance(uint32_t application_version, std::string const& aplication_name);        // Création de l'instance vulkan
            bool CreatePresentationSurface();                                                                   // Création de la surface de présentation
            bool CreateDevice(bool separate_transfer_queue, char preferred_device_index = -1);                  // Création du logical device
            void GetDeviceQueues();                                                                             // Récupère le handle des device queues
            bool CreateSwapChain();                                                                             // Création de la swap chain
            VkFormat FindDepthFormat();                                                                         // Recherche du format d'image pour le depth buffer
            bool CreateRenderPass();                                                                            // Création de la render pass
            bool AllocateTransferResources();
            void ReleaseTransferResources();
           

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

            // Resources
            bool MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, uint32_t &type_index);

            // Divers
            void ReleaseDepthBuffer();

            ///////////////////////
            // VALIDATION LAYERS //
            ///////////////////////

            #if defined(DISPLAY_LOGS)
            VkDebugUtilsMessengerEXT report_callback;
            void CreateDebugReportCallback();
            #endif
    };
}