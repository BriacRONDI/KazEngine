#pragma once

#include <Singleton.hpp>

#include "../Platform/Common/Window/Window.h"
#include "VulkanTools.h"

#if defined(DISPLAY_LOGS)
#include <iostream>
#endif

#define ENGINE_VERSION VK_MAKE_VERSION(0, 0, 1)
#define ENGINE_NAME "KazEngine"

namespace Engine
{
    // Trick used to declare all the vulkan functions
    // Source : https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1
    #define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
    #define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
    #define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
    #define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
    #include "ListOfFunctions.inl"

    class Vulkan : public Singleton<Vulkan>, public IWindowListener
    {
        friend Singleton<Vulkan>;

        public :

            struct DEVICE_QUEUE {
                uint32_t index;
                VkQueue handle;

                DEVICE_QUEUE() : index(0), handle(nullptr) {}
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

            /// Initialize vulkan instance
            bool Initialize(Engine::Window* draw_window, uint32_t application_version, std::string aplication_name);

            /// Find the memory type mathing the requirements
            bool MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, uint32_t &type_index);

            /// Build the swap chain and the frame buffers
            bool RebuildPresentResources();

            /// Acquire swap chain image
            bool AcquireNextImage(uint32_t& swapchain_image_index, VkSemaphore semaphore);

            /// Present Image
            bool PresentImage(std::vector<VkSemaphore> semaphores, uint32_t swap_chain_image_index);

            static VkRenderPass GetRenderPass() { return Singleton<Vulkan>::instance->render_pass; }
            static Surface& GetDrawSurface() { return Singleton<Vulkan>::instance->draw_surface; }
            static VkDevice GetDevice() { return Singleton<Vulkan>::instance->device; }
            static uint32_t GetSwapChainImageCount() { return static_cast<uint32_t>(Singleton<Vulkan>::instance->swap_chain.images.size()); }
            static DEVICE_QUEUE GetGraphicsQueue() { return Singleton<Vulkan>::instance->graphics_queue; }
            static DEVICE_QUEUE GetTransferQueue() { return Singleton<Vulkan>::instance->transfer_queue; }
            static DEVICE_QUEUE GetComputeQueue() { return Singleton<Vulkan>::instance->compute_queue; }
            static VkFramebuffer GetFrameBuffer(uint8_t frame_index) { return Singleton<Vulkan>::instance->frame_buffers[frame_index]; }
            static VkPhysicalDeviceLimits const& GetDeviceLimits() { return Singleton<Vulkan>::instance->physical_device.properties.limits; }
            static VkDeviceSize UboAlignment() { return Singleton<Vulkan>::instance->physical_device.properties.limits.minUniformBufferOffsetAlignment; }
            static VkDeviceSize SboAlignment() { return Singleton<Vulkan>::instance->physical_device.properties.limits.minStorageBufferOffsetAlignment; }
            static SWAP_CHAIN GetSwapChain() { return Singleton<Vulkan>::instance->swap_chain; }
            static VkFormat GetDepthFormat() { return Singleton<Vulkan>::instance->depth_format; }

            ///////////////////////////////
            // Interface IWindowListener //
            ///////////////////////////////

            void StateChanged(IWindowListener::E_WINDOW_STATE window_state) {}
            void SizeChanged(Area<uint32_t> size) {}

        private :

            struct PHYSICAL_DEVICE {
                VkPhysicalDevice handle;
                VkPhysicalDeviceMemoryProperties memory;
                VkPhysicalDeviceProperties properties;
                VkPhysicalDeviceFeatures features;

                PHYSICAL_DEVICE() : handle(nullptr), memory({}), properties({}), features({}) {}
            };

            /// Vulkan shared library handle
            LibraryHandle library;

            /// Vulkan api instance
            VkInstance instance;

            /// Vulkan api device
            VkDevice device;

            /// Vulkan api current physical device
            PHYSICAL_DEVICE physical_device;

            /// vulkan draw window
            Window* draw_window;

            /// Platform dependant drawing surface details
            Surface draw_surface;

            /// vulkan api presentation surface
            VkSurfaceKHR presentation_surface;

            DEVICE_QUEUE present_queue;
            DEVICE_QUEUE graphics_queue;
            DEVICE_QUEUE transfer_queue;
            DEVICE_QUEUE compute_queue;

            /// Depth buffer format
            VkFormat depth_format;

            /// Depth buffer
            vk::IMAGE_BUFFER depth_buffer;

            /// Swap Chain
            SWAP_CHAIN swap_chain;

            /// Render Pass
            VkRenderPass render_pass;

            /// Frame Buffers
            std::vector<VkFramebuffer> frame_buffers;

            /// Private constructor for singleton
            Vulkan();

            /// Destroy all vulkan instance objects
            ~Vulkan();

            /// Link Vulkan shared library
            bool LoadPlatformLibrary();

            /// Load vkGetInstanceProcAddr
            bool LoadExportedEntryPoints();

            /// Load vulkan global level functions
            bool LoadGlobalLevelEntryPoints();

            /// Load vulkan instance level functions
            bool LoadInstanceLevelEntryPoints();

            /// Load vulkan device level functions
            bool LoadDeviceLevelEntryPoints();

            /// Create the vulkan api instance
            bool CreateVulkanInstance(uint32_t application_version, std::string aplication_name);

            /// Create the vulkan api device
            bool CreateDevice(bool separate_transfer_queue, char preferred_device_index = -1);

            /// Create the vulkan api drawinf surface
            bool CreatePresentationSurface();

            /// Select the present queue family
            uint32_t SelectPresentQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties);

            /// Select the most suitable queue family for our needs
            uint32_t SelectPreferredQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties, VkQueueFlagBits queue_type, uint32_t common_queue);

            /// Select a dedicated queue family for a given purpose
            uint32_t SelectDedicatedQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties, VkQueueFlagBits queue_type, std::vector<uint32_t> other_queues);

            /// Get physical device properties from vulkan
            std::vector<VkQueueFamilyProperties> QueryDeviceProperties(VkPhysicalDevice physical_device);

            /// Check if a physical devcice has a graphics and a compute queue family
            bool IsDeviceEligible(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties);

            /// Get queue handles
            void GetDeviceQueues();

            /// Find a suitable image format for depth buffer
            VkFormat FindDepthFormat();

            /// Find a suitable extent for the swap chain
            VkExtent2D GetSwapChainExtent(VkSurfaceCapabilitiesKHR surface_capabilities);

            /// Find the most optimized count of images for the swapchain
            uint32_t GetSwapChainNumImages(VkSurfaceCapabilitiesKHR surface_capabilities);

            /// Find the most suitable surface format for the swapchain
            VkFormat GetSurfaceFormat();

            /// Find the best transform flag for the swap chaijn
            VkSurfaceTransformFlagBitsKHR GetSwapChainTransform(VkSurfaceCapabilitiesKHR surface_capabilities);

            /// Find the most suitable composite alpha for the swap chain
            VkCompositeAlphaFlagBitsKHR GetSwapChainCompositeAlpha(VkSurfaceCapabilitiesKHR surface_capabilities);

            /// Find the most suitable presentation mode, MAILBOX is preferred
            VkPresentModeKHR GetSwapChainPresentMode();

            /// Create vulkan swap chain
            bool CreateSwapChain();

            /// Create the render pass
            bool CreateRenderPass();

            /// Create the frame buffers
            bool CreateFrameBuffers();

            /// Destroy the depth buffer
            void ClearDepthBuffer();

            #if defined(DISPLAY_LOGS)
            VkDebugUtilsMessengerEXT report_callback;
            void CreateDebugReportCallback();
            #endif
    };
}