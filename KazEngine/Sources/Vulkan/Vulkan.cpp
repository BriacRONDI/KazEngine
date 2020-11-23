#include "Vulkan.h"

namespace Engine
{
    #define VK_EXPORTED_FUNCTION( fun ) PFN_##fun fun;
    #define VK_GLOBAL_LEVEL_FUNCTION( fun) PFN_##fun fun;
    #define VK_INSTANCE_LEVEL_FUNCTION( fun ) PFN_##fun fun;
    #define VK_DEVICE_LEVEL_FUNCTION( fun ) PFN_##fun fun;
    #include "ListOfFunctions.inl"

    Vulkan::Vulkan()
    {
        this->library               = nullptr;
        this->instance              = nullptr;
        this->device                = nullptr;
        this->draw_window           = nullptr;
        this->presentation_surface  = nullptr;
        this->render_pass           = nullptr;

        this->depth_format          = VK_FORMAT_UNDEFINED;
    }

    bool Vulkan::Initialize(Engine::Window* draw_window, uint32_t application_version, std::string aplication_name)
    {
        this->draw_window = draw_window;
        this->draw_surface = draw_window->GetSurface();
        draw_window->AddListener(this);

        if(!this->LoadPlatformLibrary()) return false;

        #if defined(DISPLAY_LOGS)
        std::string version;
        if(vk::GetVulkanVersion(version)) std::cout << "Version de vulkan : " << version << std::endl;
        #endif

        if(!this->LoadExportedEntryPoints()) return false;
        if(!this->LoadGlobalLevelEntryPoints()) return false;
        if(!this->CreateVulkanInstance(application_version, aplication_name)) return false;
        if(!this->LoadInstanceLevelEntryPoints()) return false;

        #if defined(DISPLAY_LOGS)
        this->report_callback = VK_NULL_HANDLE;
        this->CreateDebugReportCallback();
        #endif

        if(!this->CreatePresentationSurface()) return false;
        if(!this->CreateDevice(false)) return false;
        if(!this->LoadDeviceLevelEntryPoints()) return false;

        this->depth_format = this->FindDepthFormat();
        this->swap_chain.format = this->GetSurfaceFormat();
        this->GetDeviceQueues();

        if(!this->CreateRenderPass()) return false;

        if(!this->RebuildPresentResources()) return false;

        return true;
    }

    Vulkan::~Vulkan()
    {
        // Frame Buffers
        for(auto frame_buffer : this->frame_buffers) vk::Destroy(frame_buffer);

        // Depth Buffer
        this->ClearDepthBuffer();

        // Render Pass
        vk::Destroy(this->render_pass);

        // Swap Chain images
        for(int i=0; i<this->swap_chain.images.size(); i++) vk::Destroy(this->swap_chain.images[i].view);
        this->swap_chain.images.clear();

        // Swap Chain
        if(this->swap_chain.handle != nullptr) vkDestroySwapchainKHR(this->device, this->swap_chain.handle, nullptr);

        // Device
        if(this->device != nullptr && vkDestroyDevice) vkDestroyDevice(this->device, nullptr);

        // Surface
        if(this->presentation_surface != nullptr) vkDestroySurfaceKHR(this->instance, this->presentation_surface, nullptr);

        #if defined(DISPLAY_LOGS)
        if(this->report_callback != nullptr) vkDestroyDebugUtilsMessengerEXT(this->instance, this->report_callback, nullptr);
        #endif

        // Instance
        if(this->instance != nullptr) vkDestroyInstance(this->instance, nullptr);

        // Library
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        if(this->library != nullptr) FreeLibrary(this->library);
        #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        if(this->library != nullptr) dlclose(this->library);
        #endif
    }

    void Vulkan::ClearDepthBuffer()
    {
        if(this->depth_buffer.view != nullptr) vkDestroyImageView(this->device, this->depth_buffer.view, nullptr);
        if(this->depth_buffer.memory != nullptr) vkFreeMemory(this->device, this->depth_buffer.memory, nullptr);
        if(this->depth_buffer.handle != nullptr) vkDestroyImage(this->device, this->depth_buffer.handle, nullptr);
    }

    bool Vulkan::LoadPlatformLibrary()
    {
        // Load DLL for MS Windows
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        this->library = LoadLibrary(L"vulkan-1.dll");

        // Load .so for Linux / MACOS
        #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        this->Library = dlopen("libvulkan.so.1", RTLD_NOW);

        #endif

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::LoadPlatformLibrary() : " << ((this->library != nullptr) ? "Success" : "Failure") << std::endl;
        #endif

        // Succès si library n'est pas nul
        return this->library != nullptr;
    }

    bool Vulkan::LoadExportedEntryPoints()
    {
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        #define LoadProcAddress GetProcAddress
        #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        #define LoadProcAddress dlsym
        #endif

        #if defined(DISPLAY_LOGS)
        #define VK_EXPORTED_FUNCTION(fun)                                                                   \
        if(!(fun = (PFN_##fun)LoadProcAddress(this->library, #fun))) {                                      \
            std::cout << "Vulkan::LoadExportedEntryPoints()(" << #fun << ") : " << "Failure" << std::endl;  \
            return false;                                                                                   \
        }
        #else
        #define VK_EXPORTED_FUNCTION(fun) \
        if(!(fun = (PFN_##fun)LoadProcAddress(this->library, #fun))) return false;
        #endif
        #include "ListOfFunctions.inl"

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::LoadExportedEntryPoints() : Success" << std::endl;
        #endif

        // Success
        return true;
    }

    bool Vulkan::LoadGlobalLevelEntryPoints()
	{
        #if defined(DISPLAY_LOGS)
        #define VK_GLOBAL_LEVEL_FUNCTION(fun)                                                               \
        if(!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) {                                      \
            std::cout << "Vulkan::LoadGlobalLevelEntryPoints(" << #fun << ") : " << "Failure" << std::endl; \
            return false;                                                                                   \
        }
        #else
        #define VK_GLOBAL_LEVEL_FUNCTION(fun) \
        if(!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) return false;
        #endif
        #include "ListOfFunctions.inl"

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::LoadGlobalLevelEntryPoints() : Success" << std::endl;
        #endif

		// Succès
		return true;
	}

    bool Vulkan::LoadInstanceLevelEntryPoints()
    {
        #if defined(DISPLAY_LOGS)
        #define VK_INSTANCE_LEVEL_FUNCTION(fun)                                                                 \
        if(!(fun = (PFN_##fun)vkGetInstanceProcAddr(this->instance, #fun))) {                                   \
            std::cout << "Vulkan::LoadInstanceLevelEntryPoints(" << #fun << ") : " << "Failure" << std::endl;   \
            return false;                                                                                       \
        }
        #else
        #define VK_INSTANCE_LEVEL_FUNCTION(fun) \
        if(!(fun = (PFN_##fun)vkGetInstanceProcAddr(this->instance, #fun))) return false;
        #endif
        #include "ListOfFunctions.inl"

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::LoadInstanceLevelEntryPoints() : Success" << std::endl;
        #endif

        return true;
    }

    bool Vulkan::LoadDeviceLevelEntryPoints()
    {
        #if defined(DISPLAY_LOGS)
        #define VK_DEVICE_LEVEL_FUNCTION(fun)                                                               \
        if(!(fun = (PFN_##fun)vkGetDeviceProcAddr(this->device, #fun))) {                                   \
            std::cout << "Vulkan::LoadDeviceLevelEntryPoints(" << #fun << ") : " << "Failure" << std::endl; \
            return false;                                                                                   \
        }
        #else
        #define VK_DEVICE_LEVEL_FUNCTION(fun) \
        if(!(fun = (PFN_##fun)vkGetDeviceProcAddr(this->device, #fun))) return false;
        #endif
        #include "ListOfFunctions.inl"

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::LoadDeviceLevelEntryPoints() : Success" << std::endl;
        #endif

        return true;
    }

    bool Vulkan::CreateVulkanInstance(uint32_t application_version, std::string aplication_name)
    {
        VkApplicationInfo app_info;
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext = VK_NULL_HANDLE;
        app_info.pApplicationName = aplication_name.c_str();
        app_info.applicationVersion = application_version;
        app_info.pEngineName = ENGINE_NAME;
        app_info.engineVersion = ENGINE_VERSION;
        app_info.apiVersion = VK_API_VERSION_1_0;

        std::vector<const char*> instance_extension_names = {
            VK_KHR_SURFACE_EXTENSION_NAME,          // Enable surface extension
            #if defined(VK_USE_PLATFORM_WIN32_KHR)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME     // Enable surface extension for win32
            #elif defined(VK_USE_PLATFORM_XCB_KHR)
            VK_KHR_XCB_SURFACE_EXTENSION_NAME
            #elif defined(VK_USE_PLATFORM_XLIB_KHR)
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME
            #endif
        };

        VkInstanceCreateInfo inst_info;
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pNext = nullptr;
        inst_info.flags = 0;
        inst_info.pApplicationInfo = &app_info;
        inst_info.enabledExtensionCount = static_cast<uint32_t>(instance_extension_names.size());
        inst_info.ppEnabledExtensionNames = instance_extension_names.data();
        inst_info.enabledLayerCount = 0;
        inst_info.ppEnabledLayerNames = nullptr;

        ///////////////////////////////
        //  Enable Validation Layers //
        //        (debug mode)       //
        ///////////////////////////////

        #if defined(DISPLAY_LOGS)
        #define USE_RENDERDOC
        // Core Validation Layers
        std::vector<const char*> instance_layer_names = {"VK_LAYER_KHRONOS_validation"};

        // Extensions
        instance_extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instance_extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        #if defined(USE_RENDERDOC)
        instance_extension_names.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        #else
        instance_extension_names.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
        #endif
         
        // Best practice feature (doesn't work with renderdoc)
        #if defined(USE_RENDERDOC)
        std::vector<VkValidationFeatureEnableEXT> enabled_features;
        #else
        std::vector<VkValidationFeatureEnableEXT> enabled_features = {VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
        #endif
        VkValidationFeaturesEXT validation_features;
        validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        validation_features.pNext = nullptr;
        validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(enabled_features.size());
        validation_features.pEnabledValidationFeatures = enabled_features.data();
        validation_features.disabledValidationFeatureCount = 0;
        validation_features.pDisabledValidationFeatures = nullptr;

        inst_info.enabledLayerCount = static_cast<uint32_t>(instance_layer_names.size());
        inst_info.ppEnabledLayerNames = instance_layer_names.data();
        inst_info.enabledExtensionCount = static_cast<uint32_t>(instance_extension_names.size());
        inst_info.ppEnabledExtensionNames = instance_extension_names.data();
        #if !defined(USE_RENDERDOC)
        inst_info.pNext = &validation_features;
        #endif
        #endif

        // Create the vulkan api instance
        VkResult result = vkCreateInstance(&inst_info, nullptr, &this->instance);

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::CreateVulkanInstance() : " << ((result == VK_SUCCESS) ? "Success" : "Failure") << std::endl;
        #endif

        // Succès
        return result == VK_SUCCESS;
    }

    std::vector<VkQueueFamilyProperties> Vulkan::QueryDeviceProperties(VkPhysicalDevice physical_device)
    {
        // On récupère les propriétés des queue families de la carte
        uint32_t queue_family_count = UINT32_MAX;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        if(queue_family_count == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "QueryDeviceProperties => vkGetPhysicalDeviceQueueFamilyProperties 1/2 : Failed" << std::endl;
            #endif
            return std::vector<VkQueueFamilyProperties>();
        }

        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, &queue_family_properties[0]);
        if(queue_family_count == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "QueryDeviceProperties => vkGetPhysicalDeviceQueueFamilyProperties 2/2 : Failed" << std::endl;
            #endif
            return std::vector<VkQueueFamilyProperties>();
        }

        return queue_family_properties;
    }

    bool Vulkan::CreatePresentationSurface()
    {
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = nullptr;
        createInfo.hwnd = this->draw_surface.handle;
        createInfo.pNext = nullptr;
        VkResult result = vkCreateWin32SurfaceKHR(this->instance, &createInfo, nullptr, &this->presentation_surface);

        #elif defined(VK_USE_PLATFORM_XCB_KHR)
        VkXcbSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.connection = this->connection.;
        createInfo.window = this->surface.handle;
        createInfo.pNext = nullptr;
        VkResult result = vkCreateXcbSurfaceKHR(this->instance, &createInfo, nullptr, &this->presentation_surface);

        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        VkXlibSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        createInfo.dpy = this->surface.display;
        createInfo.window = this->surface.handle;
        createInfo.pNext = nullptr;
        VkResult result = vkCreateXlibSurfaceKHR(this->instance, &createInfo, nullptr, &this->presentation_surface);

        #endif

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::CreatePresentationSurface() : " << ((result == VK_SUCCESS) ? "Success" : "Failure") << std::endl;
        #endif

        return result == VK_SUCCESS;
    }

    bool Vulkan::IsDeviceEligible(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties)
    {
        bool graphics_queue_found = false;
        bool compute_queue_found = false;

        for(uint32_t i = 0; i < queue_family_properties.size(); i++) {
            VkBool32 support_present;
            vkGetPhysicalDeviceSurfaceSupportKHR(test_physical_device, i, this->presentation_surface, &support_present);
            if((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) graphics_queue_found = true;
            if((queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) compute_queue_found = true;
            if(graphics_queue_found && compute_queue_found && support_present) return true;
        }
        
        return false;
    }

    uint32_t Vulkan::SelectPresentQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties)
    {
        uint32_t index = UINT32_MAX;

        for(uint32_t i = 0; i < queue_family_properties.size(); i++) {
            VkBool32 support_present;
            vkGetPhysicalDeviceSurfaceSupportKHR(test_physical_device, i, this->presentation_surface, &support_present);
            if(support_present == VK_TRUE && (index > queue_family_properties.size() || queue_family_properties[i].queueCount > queue_family_properties[index].queueCount)) index = i;
        }

        return index;
    }

    uint32_t Vulkan::SelectPreferredQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties, VkQueueFlagBits queue_type, uint32_t common_queue)
    {
        // If the desired queue is compatible, pick it
        if(common_queue != UINT32_MAX && (queue_family_properties[common_queue].queueFlags & queue_type)) return common_queue;

        struct SELECTED_QUEUE {
            uint32_t capabilities;
            uint32_t index;
        };

        // else, pick the one with most capabilities
        std::vector<uint32_t> queue_capabilities(queue_family_properties.size());
        SELECTED_QUEUE selected = {0, 0};
        for(uint32_t i=0; i<queue_family_properties.size(); i++) {
            if(!(queue_family_properties[i].queueFlags & queue_type)) continue;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) queue_capabilities[i]++;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) queue_capabilities[i]++;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) queue_capabilities[i]++;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) queue_capabilities[i]++;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_PROTECTED_BIT) queue_capabilities[i]++;
            if(queue_capabilities[i] > selected.capabilities) selected = {queue_capabilities[i], i};
        }

        return selected.index;
    }

    uint32_t Vulkan::SelectDedicatedQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties, VkQueueFlagBits queue_type, std::vector<uint32_t> other_queues)
    {
        struct SELECTED_QUEUE {
            uint32_t capabilities;
            uint32_t index;
        };

        std::vector<uint32_t> queue_capabilities(queue_family_properties.size());
        SELECTED_QUEUE selected = {UINT32_MAX, 0};
        for(uint32_t i=0; i<queue_family_properties.size(); i++) {

            // The queue family is not compatible
            if(!(queue_family_properties[i].queueFlags & queue_type)) continue;

            // The queue family is already used
            if(std::find(other_queues.begin(), other_queues.end(), i) != other_queues.end()) continue;

            if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) queue_capabilities[i]++;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) queue_capabilities[i]++;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) queue_capabilities[i]++;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) queue_capabilities[i]++;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_PROTECTED_BIT) queue_capabilities[i]++;

            // Select the queue with less capabilities
            if(queue_capabilities[i] < selected.capabilities) selected = {queue_capabilities[i], i};
        }

        return selected.index;
    }

    bool Vulkan::CreateDevice(bool separate_transfer_queue, char preferred_device_index)
    {
        uint32_t physical_device_count = 1;
        VkResult result = vkEnumeratePhysicalDevices(this->instance, &physical_device_count, nullptr);
        if(result != VK_SUCCESS || physical_device_count == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateDevice() => vkEnumeratePhysicalDevices 1/2 : Failed" << std::endl;
            #endif
            return false;
        }


        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        result = vkEnumeratePhysicalDevices(this->instance, &physical_device_count, &physical_devices[0]);
        if(result != VK_SUCCESS || physical_device_count == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateDevice() => vkEnumeratePhysicalDevices 2/2 : Failed" << std::endl;
            #endif
            return false;
        }

        std::vector<VkQueueFamilyProperties> queue_family_properties;
        if(preferred_device_index >= 0) {
            queue_family_properties = this->QueryDeviceProperties(physical_devices[preferred_device_index]);
            if(preferred_device_index < (int)physical_device_count && this->IsDeviceEligible(physical_devices[preferred_device_index], queue_family_properties)) {
                this->physical_device.handle = physical_devices[preferred_device_index];
                #if defined(DISPLAY_LOGS)
                std::cout << "Vulkan::CreateDevice() : Selected physical device : " << preferred_device_index << std::endl;
                #endif
            }
        }else{
            uint32_t max_queue_count = 0;
            int selected_index = -1;

            for(uint32_t i=0; i<physical_devices.size(); i++) {
                std::vector<VkQueueFamilyProperties> properties = this->QueryDeviceProperties(physical_devices[i]);
                if(this->IsDeviceEligible(physical_devices[0], properties)) {

                    uint32_t device_queue_count = 0;
                    for(int j=0; j<properties.size(); j++) device_queue_count += properties[j].queueCount;
                    if(device_queue_count > max_queue_count) {
                        selected_index = i;
                        max_queue_count = device_queue_count;
                        queue_family_properties = properties;
                    }
                }
            }

            if(selected_index >= 0) {
                this->physical_device.handle = physical_devices[selected_index];

                vkGetPhysicalDeviceMemoryProperties(this->physical_device.handle, &this->physical_device.memory);
                vkGetPhysicalDeviceProperties(this->physical_device.handle, &this->physical_device.properties);
                vkGetPhysicalDeviceFeatures(this->physical_device.handle, &this->physical_device.features);

                #if defined(DISPLAY_LOGS)
                std::cout << "Vulkan::CreateDevice() : Selected physical device : " << selected_index << " : " << this->physical_device.properties.deviceName << std::endl;
                #endif
            }
        }

        #if defined(DISPLAY_LOGS)
        for(uint32_t i=0; i<queue_family_properties.size(); i++) {
            std::cout << "Vulkan::CreateDevice() : Queue[" << i << "] : ";

            bool delim = false;
            if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                std::cout << "GRAPHICS";
                delim = true;
            }

            if(queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                if(delim) std::cout << " | ";
                std::cout << "TRANSFER";
                delim = true;
            }

            if(queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                if(delim) std::cout << " | ";
                std::cout << "COMPUTE";
                delim = true;
            }

            if(queue_family_properties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
                if(delim) std::cout << " | ";
                std::cout << "SPARSE_BINDING";
                delim = true;
            }

            if(queue_family_properties[i].queueFlags & VK_QUEUE_PROTECTED_BIT) {
                if(delim) std::cout << " | ";
                std::cout << "PROTECTED";
            }

            std::cout << std::endl;
        }
        #endif

        if(this->physical_device.handle == VK_NULL_HANDLE) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateDevice() : No eligible device found" << std::endl;
            #endif
            return false;
        }

        this->present_queue.index = this->SelectPresentQueue(this->physical_device.handle, queue_family_properties);

        this->graphics_queue.index = this->SelectPreferredQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_GRAPHICS_BIT, this->present_queue.index);
        if(separate_transfer_queue)
            this->transfer_queue.index = this->SelectDedicatedQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_TRANSFER_BIT, {this->graphics_queue.index});
        else this->transfer_queue.index = this->graphics_queue.index;
        this->compute_queue.index = this->SelectDedicatedQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_COMPUTE_BIT, {this->graphics_queue.index, this->transfer_queue.index});
        this->compute_queue.index = this->graphics_queue.index;

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::CreateDevice() : Graphics queue family : " << this->graphics_queue.index << ", count : " << queue_family_properties[this->graphics_queue.index].queueCount << std::endl;
        std::cout << "Vulkan::CreateDevice() : Transfert queue family : " << this->transfer_queue.index << ", count : " << queue_family_properties[this->transfer_queue.index].queueCount << std::endl;
        std::cout << "Vulkan::CreateDevice() : Present queue family : " << this->present_queue.index << ", count : " << queue_family_properties[this->present_queue.index].queueCount << std::endl;
        std::cout << "Vulkan::CreateDevice() : Compute queue family : " << this->compute_queue.index << ", count : " << queue_family_properties[this->compute_queue.index].queueCount << std::endl;
        #endif

        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        std::array<uint32_t, 3> queues = {this->present_queue.index, this->graphics_queue.index, this->transfer_queue.index};
        std::vector<float> queue_priorities;// = {0.0f};

        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = nullptr;

        if(queue_family_properties[this->graphics_queue.index].queueCount > 1) queue_priorities = {0.0f, 0.0f};
        else queue_priorities = {0.0f};

        queue_info.queueCount = static_cast<uint32_t>(queue_priorities.size());
        queue_info.pQueuePriorities = &queue_priorities[0];
        queue_infos.push_back(queue_info);

        if(this->present_queue.index != this->graphics_queue.index) {
            queue_info.queueCount = 1;
            queue_info.queueFamilyIndex = this->present_queue.index;
            queue_infos.push_back(queue_info);
        }

        if(this->transfer_queue.index != this->graphics_queue.index) {
            
            if(queue_family_properties[this->transfer_queue.index].queueCount > 1) queue_priorities = {0.0f, 0.0f};
            else queue_priorities = {0.0f};

            queue_info.queueCount = static_cast<uint32_t>(queue_priorities.size());
            queue_info.pQueuePriorities = &queue_priorities[0];
            queue_info.queueFamilyIndex = this->transfer_queue.index;
            queue_infos.push_back(queue_info);
        }

        if(this->compute_queue.index != this->graphics_queue.index) {
            queue_priorities = {0.0f, 0.0f};
            queue_info.queueCount = 2;
            queue_info.queueFamilyIndex = this->compute_queue.index;
            queue_infos.push_back(queue_info);
        }

        VkPhysicalDeviceFeatures features = {};
        // features.vertexPipelineStoresAndAtomics = VK_TRUE;
        // features.fillModeNonSolid = VK_TRUE;
        features.multiDrawIndirect = VK_TRUE;
        #if defined(DISPLAY_LOGS)
        features.geometryShader = VK_TRUE;
        features.wideLines = VK_TRUE;
        #endif

        // Enable swapchain extension
        std::vector<const char*> device_extension_name = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        VkDeviceCreateInfo device_info;
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = nullptr;
        device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
        device_info.pQueueCreateInfos = &queue_infos[0];
        device_info.enabledExtensionCount = static_cast<uint32_t>(device_extension_name.size());
        device_info.ppEnabledExtensionNames = &device_extension_name[0];
        device_info.enabledLayerCount = 0;
        device_info.ppEnabledLayerNames = nullptr;
        device_info.pEnabledFeatures = &features;
        device_info.flags = 0;

        result = vkCreateDevice(this->physical_device.handle, &device_info, nullptr, &this->device);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateDevice() => vkCreateDevice : Failed" << std::endl;
            #endif
            return false;
        }

        // Succès
        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::CreateDevice() : Success" << std::endl;
        #endif
        return true;
    }

    void Vulkan::GetDeviceQueues()
    {
        vkGetDeviceQueue(this->device, this->present_queue.index, 0, &this->present_queue.handle);
        vkGetDeviceQueue(this->device, this->graphics_queue.index, 0, &this->graphics_queue.handle);
        vkGetDeviceQueue(this->device, this->transfer_queue.index, 0, &this->transfer_queue.handle);
        vkGetDeviceQueue(this->device, this->compute_queue.index, 0, &this->compute_queue.handle);
    }

    VkFormat Vulkan::FindDepthFormat()
    {
        std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		for(auto& format : depthFormats)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(this->physical_device.handle, format, &formatProps);
			if(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) return format;
		}

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::FindDepthFormat() => Format : VK_FORMAT_UNDEFINED" << std::endl;
        #endif

        return VK_FORMAT_UNDEFINED;
    }

    VkExtent2D Vulkan::GetSwapChainExtent(VkSurfaceCapabilitiesKHR surface_capabilities)
    {
        if(surface_capabilities.currentExtent.width == -1) {
            VkExtent2D swap_chain_extent = {this->draw_surface.width, this->draw_surface.height};
            if(swap_chain_extent.width < surface_capabilities.minImageExtent.width) swap_chain_extent.width = surface_capabilities.minImageExtent.width;
            if(swap_chain_extent.height < surface_capabilities.minImageExtent.height) swap_chain_extent.height = surface_capabilities.minImageExtent.height;
            if(swap_chain_extent.width > surface_capabilities.maxImageExtent.width) swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
            if(swap_chain_extent.height > surface_capabilities.maxImageExtent.height) swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
            return swap_chain_extent;
        }

        return surface_capabilities.currentExtent;
    }

    uint32_t Vulkan::GetSwapChainNumImages(VkSurfaceCapabilitiesKHR surface_capabilities)
    {
        uint32_t image_count = surface_capabilities.minImageCount + 1;
        if(surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) image_count = surface_capabilities.maxImageCount;
        return image_count;
    }

    VkFormat Vulkan::GetSurfaceFormat()
    {
        // Récupère la liste des VkFormats supportés
        uint32_t formatCount = UINT32_MAX;
        VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(this->physical_device.handle, this->presentation_surface, &formatCount, nullptr);
        if(res != VK_SUCCESS || formatCount == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::GetSurfaceFormat() => vkGetPhysicalDeviceSurfaceFormatsKHR 1/2 : Failed" << std::endl;
            #endif
            return VK_FORMAT_UNDEFINED;
        }

        std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(this->physical_device.handle, this->presentation_surface, &formatCount, &surfFormats[0]);
        if(res != VK_SUCCESS || formatCount == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::GetSurfaceFormat() => vkGetPhysicalDeviceSurfaceFormatsKHR 2/2 : Failed" << std::endl;
            #endif
            return VK_FORMAT_UNDEFINED;
        }

        if(formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) return VK_FORMAT_B8G8R8A8_UNORM;
        else return surfFormats[0].format;
    }

    VkSurfaceTransformFlagBitsKHR Vulkan::GetSwapChainTransform(VkSurfaceCapabilitiesKHR surface_capabilities)
    {
        if(surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        else return surface_capabilities.currentTransform;
    }

    VkCompositeAlphaFlagBitsKHR Vulkan::GetSwapChainCompositeAlpha(VkSurfaceCapabilitiesKHR surface_capabilities)
    {
        if(surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        if(surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        if(surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if(surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    VkPresentModeKHR Vulkan::GetSwapChainPresentMode()
    {
        uint32_t present_modes_count;
        VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(this->physical_device.handle, this->presentation_surface, &present_modes_count, nullptr);
        if(res != VK_SUCCESS || present_modes_count == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "GetSwapCHainPresentMode => vkGetPhysicalDeviceSurfacePresentModesKHR 1/2 : Failed" << std::endl;
            #endif
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        std::vector<VkPresentModeKHR> present_modes(present_modes_count);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(this->physical_device.handle, this->presentation_surface, &present_modes_count, &present_modes[0]);
        if(res != VK_SUCCESS || present_modes_count == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "GetSwapCHainPresentMode => vkGetPhysicalDeviceSurfacePresentModesKHR 2/2 : Failed" << std::endl;
            #endif
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        for(VkPresentModeKHR &present_mode : present_modes)
            if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return VK_PRESENT_MODE_MAILBOX_KHR;

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    bool Vulkan::CreateSwapChain()
    {
        // Wait for GPU to get idle
        vkDeviceWaitIdle(this->device);

        // Get presentation surface capabilities
        VkSurfaceCapabilitiesKHR surface_capabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physical_device.handle, this->presentation_surface, &surface_capabilities);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateSwapChain() => vkGetPhysicalDeviceSurfaceCapabilitiesKHR : Failed" << std::endl;
            #endif
            return false;
        }

        VkExtent2D swap_chain_extent = this->GetSwapChainExtent(surface_capabilities);
        uint32_t swapchain_image_count = this->GetSwapChainNumImages(surface_capabilities);
        VkSurfaceTransformFlagBitsKHR swap_chain_transform = this->GetSwapChainTransform(surface_capabilities);
        VkCompositeAlphaFlagBitsKHR swap_chain_composite_alpha = this->GetSwapChainCompositeAlpha(surface_capabilities);
        VkPresentModeKHR swap_chain_present_mode = this->GetSwapChainPresentMode();

        // Si la swap chain est reconstruite, on conserve l'ancienne pour la détruire après remplacement
        VkSwapchainKHR old_swap_chain = this->swap_chain.handle;

        // Déclaration de la structure pour la SwapChain
        VkSwapchainCreateInfoKHR swapchain_ci = {};
        swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_ci.pNext = nullptr;
        swapchain_ci.surface = this->presentation_surface;
        swapchain_ci.minImageCount = swapchain_image_count;
        swapchain_ci.imageFormat = this->swap_chain.format;
        swapchain_ci.imageExtent.width = swap_chain_extent.width;
        swapchain_ci.imageExtent.height = swap_chain_extent.height;
        swapchain_ci.preTransform = swap_chain_transform;
        swapchain_ci.compositeAlpha = swap_chain_composite_alpha;
        swapchain_ci.imageArrayLayers = 1;
        swapchain_ci.presentMode = swap_chain_present_mode;
        swapchain_ci.oldSwapchain = old_swap_chain;
        swapchain_ci.clipped = VK_TRUE;
        swapchain_ci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_ci.queueFamilyIndexCount = 0;
        swapchain_ci.pQueueFamilyIndices = nullptr;

        std::vector<uint32_t> shared_queue_families = {
            this->graphics_queue.index,
            this->present_queue.index
        };

        if(this->graphics_queue.index != this->present_queue.index) {
            swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchain_ci.queueFamilyIndexCount = static_cast<uint32_t>(shared_queue_families.size());
            swapchain_ci.pQueueFamilyIndices = &shared_queue_families[0];
        }

        result = vkCreateSwapchainKHR(this->device, &swapchain_ci, nullptr, &this->swap_chain.handle);

        for(int i=0; i<this->swap_chain.images.size(); i++)
            if(this->swap_chain.images[i].view != VK_NULL_HANDLE)
                vkDestroyImageView(this->device, this->swap_chain.images[i].view, nullptr);
        this->swap_chain.images.clear();
        if(old_swap_chain != VK_NULL_HANDLE) vkDestroySwapchainKHR(this->device, old_swap_chain, nullptr);

        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateSwapChain() => vkCreateSwapchainKHR : Failed" << std::endl;
            #endif
            return false;
        }

        uint32_t swap_chain_image_count;
        result = vkGetSwapchainImagesKHR(this->device, this->swap_chain.handle, &swap_chain_image_count, nullptr);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateSwapChain() => vkGetSwapchainImagesKHR 1/2 : Failed" << std::endl;
            #endif
            return false;
        }

        std::vector<VkImage> swap_chain_image(swap_chain_image_count);
        result = vkGetSwapchainImagesKHR(this->device, this->swap_chain.handle, &swap_chain_image_count, &swap_chain_image[0]);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSwapChain => vkGetSwapchainImagesKHR 2/2 : Failed" << std::endl;
            #endif
            return false;
        }

        VkImageSubresourceRange image_subresource_range;
        image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_subresource_range.baseMipLevel = 0;
        image_subresource_range.levelCount = 1;
        image_subresource_range.baseArrayLayer = 0;
        image_subresource_range.layerCount = 1;

        // On créé les image views de la swap chain
        this->swap_chain.images.resize(swap_chain_image_count);
        for(uint32_t i = 0; i<swap_chain_image_count; i++) {

            // Lecture de l'image
            this->swap_chain.images[i].handle = swap_chain_image[i];

            VkImageViewCreateInfo color_image_view = {};
            color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            color_image_view.pNext = nullptr;
            color_image_view.flags = 0;
            color_image_view.image = swap_chain_image[i];
            color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            color_image_view.format = this->swap_chain.format;
            color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
            color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
            color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
            color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
            color_image_view.subresourceRange = image_subresource_range;

            VkResult result = vkCreateImageView(this->device, &color_image_view, nullptr, &this->swap_chain.images[i].view);
            if(result != VK_SUCCESS) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Vulkan::CreateSwapChain() => vkCreateImageView[" << i << "] : Failed" << std::endl;
                #endif
                return false;
            }
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::CreateSwapChain() : Success" << std::endl;
        #endif

        return true;
    }

    bool Vulkan::CreateRenderPass()
    {
        VkAttachmentDescription attachment[2];

        attachment[0].format = this->swap_chain.format;
        attachment[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment[0].flags = 0;

        attachment[1].format = this->depth_format;
        attachment[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment[1].flags = 0;

        VkAttachmentReference color_reference = {};
        color_reference.attachment = 0;
        color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_reference = {};
        depth_reference.attachment = 1;
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;
        subpass.pResolveAttachments = nullptr;
        subpass.pDepthStencilAttachment = &depth_reference;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;

        std::vector<VkSubpassDependency> dependencies;

        VkSubpassDependency dependency;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	    dependency.dstSubpass = 0;
	    dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies.push_back(dependency);

	    dependency.srcSubpass = 0;
	    dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies.push_back(dependency);

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = nullptr;
        rp_info.attachmentCount = 2;
        rp_info.pAttachments = attachment;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
        rp_info.pDependencies = dependencies.data();

        VkResult result = vkCreateRenderPass(this->device, &rp_info, nullptr, &this->render_pass);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateRenderPass() => vkCreateRenderPass : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::CreateRenderPass() : Success" << std::endl;
        #endif
        return true;
    }

    bool Vulkan::MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, uint32_t &type_index)
    {
        for(uint32_t i = 0; i < this->physical_device.memory.memoryTypeCount; i++) {
            if((type_bits & 1) == 1) {

                if((this->physical_device.memory.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                    type_index = i;
                    return true;
                }
            }
            type_bits >>= 1;
        }

        return false;
    }

    bool Vulkan::CreateFrameBuffers()
    {
        if(this->frame_buffers.empty()) this->frame_buffers.resize(this->swap_chain.images.size());

        for(uint32_t i=0; i<this->swap_chain.images.size(); i++) {

            if(this->frame_buffers[i] != nullptr) {
                vkDestroyFramebuffer(this->device, this->frame_buffers[i], nullptr);
                this->frame_buffers[i] = nullptr;
            }

            VkImageView pAttachments[2];
            pAttachments[0] = this->swap_chain.images[i].view;
            pAttachments[1] = this->depth_buffer.view;

            VkFramebufferCreateInfo framebuffer_create_info = {};
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.pNext = nullptr;
            framebuffer_create_info.flags = 0;
            framebuffer_create_info.renderPass = this->render_pass;
            framebuffer_create_info.attachmentCount = 2;
            framebuffer_create_info.pAttachments = pAttachments;
            framebuffer_create_info.width = this->draw_surface.width;
            framebuffer_create_info.height = this->draw_surface.height;
            framebuffer_create_info.layers = 1;

            VkResult result = vkCreateFramebuffer(this->device, &framebuffer_create_info, nullptr, &this->frame_buffers[i]);
            if(result != VK_SUCCESS) {
                #if defined(DISPLAY_LOGS)
                std::cout << "Vulkan::CreateFrameBuffers() => vkCreateFramebuffer : Failed" << std::endl;
                #endif
                return false;
            }
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan::CreateFrameBuffers() : Success" << std::endl;
        #endif

        return true;
    }

    bool Vulkan::RebuildPresentResources()
    {
        vkDeviceWaitIdle(this->device);

        this->draw_surface = this->draw_window->GetSurface();
        if(!this->draw_surface.width || !this->draw_surface.height) return false;

        this->ClearDepthBuffer();
        this->depth_buffer = vk::CreateImageBuffer(
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                this->draw_surface.width,
                this->draw_surface.height,
                this->depth_format);

        if(this->depth_buffer.view == nullptr) return false;

        if(!this->CreateSwapChain()) return false;
        if(!this->CreateFrameBuffers()) return false;

        return true;
    }

    bool Vulkan::AcquireNextImage(uint32_t& swapchain_image_index, VkSemaphore semaphore)
    {
        VkResult result = vkAcquireNextImageKHR(this->device, this->swap_chain.handle, UINT64_MAX, semaphore, VK_NULL_HANDLE, &swapchain_image_index);
        switch(result) {
            #if defined(DISPLAY_LOGS)
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                std::cout << "Vulkan::AcquireNextImage() : VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
                return false;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                std::cout << "Vulkan::AcquireNextImage() : VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
                return false;
            case VK_ERROR_DEVICE_LOST:
                std::cout << "Vulkan::AcquireNextImage() : VK_ERROR_DEVICE_LOST" << std::endl;
                return false;
            case VK_ERROR_SURFACE_LOST_KHR:
                std::cout << "Vulkan::AcquireNextImage() : VK_ERROR_SURFACE_LOST_KHR" << std::endl;
                return false;
            case VK_TIMEOUT:
                std::cout << "Vulkan::AcquireNextImage() : VK_TIMEOUT" << std::endl;
                return false;
            case VK_NOT_READY:
                std::cout << "Vulkan::AcquireNextImage() : VK_NOT_READY" << std::endl;
                return false;
            case VK_ERROR_VALIDATION_FAILED_EXT:
                std::cout << "Vulkan::AcquireNextImage() : VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
                return false;
            #endif

            case VK_SUCCESS:
                break;
            case VK_SUBOPTIMAL_KHR:
                break; 
            case VK_ERROR_OUT_OF_DATE_KHR:
                this->RebuildPresentResources();
                return false;
            default:
                #if defined(DISPLAY_LOGS)
                std::cout << "Vulkan::AcquireNextImage() : Failed" << std::endl;
                #endif
                return false;
        }

        return true;
    }

    bool Vulkan::PresentImage(std::vector<VkSemaphore> semaphores, uint32_t swap_chain_image_index)
    {
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = nullptr;
        present_info.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());
        present_info.pWaitSemaphores = semaphores.data();
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &this->swap_chain.handle;
        present_info.pImageIndices = &swap_chain_image_index;
        present_info.pResults = nullptr;

        VkResult result = vkQueuePresentKHR(this->present_queue.handle, &present_info);

        switch(result)
        {
            case VK_SUCCESS:
                break;
            case VK_ERROR_OUT_OF_DATE_KHR:
            case VK_SUBOPTIMAL_KHR:
                this->RebuildPresentResources();
                return false;
            default:
                #if defined(DISPLAY_LOGS)
                std::cout << "Vulkan::PresentImage() => vkQueuePresentKHR : Failed" << std::endl;
                #endif
                return false;
        }

        return true;
    }

    #if defined(DISPLAY_LOGS)
    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(
            VkDebugReportFlagsEXT       flags,
            VkDebugReportObjectTypeEXT  objectType,
            uint64_t                    object,
            size_t                      location,
            int32_t                     messageCode,
            const char*                 pLayerPrefix,
            const char*                 pMessage,
            void*                       pUserData)
    {
        std::cerr << pMessage << std::endl;
        return VK_FALSE;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    void Vulkan::CreateDebugReportCallback()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        VkResult result = vkCreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->report_callback);
        if(result != VK_SUCCESS) std::cout << "CreateDebugReportCallback => vkCreateDebugUtilsMessengerEXT : Failed" << std::endl;
    }
    #endif
}