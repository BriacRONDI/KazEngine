#include "Vulkan.h"

namespace Engine
{
    #define VK_EXPORTED_FUNCTION( fun ) PFN_##fun fun;
    #define VK_GLOBAL_LEVEL_FUNCTION( fun) PFN_##fun fun;
    #define VK_INSTANCE_LEVEL_FUNCTION( fun ) PFN_##fun fun;
    #define VK_DEVICE_LEVEL_FUNCTION( fun ) PFN_##fun fun;
    #include "ListOfFunctions.inl"

    // Initialisation du singleton
    Vulkan* Vulkan::vulkan = nullptr;

    /**
     * Affichage des erreurs lorsque les validation layers sont activées
     */
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

    /**
     * Création du rapport d'erreur pour les validation layers
     */
    void Vulkan::CreateDebugReportCallback()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        VkResult result = vkCreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->report_callback);
        if(result != VK_SUCCESS) std::cout << "CreateDebugReportCallback => vkCreateDebugUtilsMessengerEXT : Failed" << std::endl;
    }
    #endif

    /**
     * Constructeur
     */
    Vulkan::Vulkan()
    {
        #if defined(DISPLAY_LOGS)
        std::string vulkan_version;
        this->GetVersion(vulkan_version);
        std::cout << "Version de vulkan : " << vulkan_version << std::endl;
        #endif

        this->library                   = nullptr;
        this->draw_window               = nullptr;
        this->instance                  = VK_NULL_HANDLE;
        this->device                    = VK_NULL_HANDLE;
        this->presentation_surface      = VK_NULL_HANDLE;
        this->device                    = VK_NULL_HANDLE;
        this->render_pass               = VK_NULL_HANDLE;
        this->concurrent_frame_count    = 0;
        this->creating_swap_chain       = false;
        this->depth_format              = VK_FORMAT_UNDEFINED;
    }

    /**
     * Destructeur
     * Libère toutes les ressources
     */
    Vulkan::~Vulkan()
    {
        // On attend que la carte graphique arrête de travailler
        if(this->device != nullptr) vkDeviceWaitIdle(this->device);

        // Depth Buffer
        this->ReleaseDepthBuffer();

        // Libère les buffers principaux
        // this->ReleaseMainBuffers();

        this->ReleaseTransferResources();

        // Render Pass
        if(this->render_pass != nullptr) vkDestroyRenderPass(this->device, this->render_pass, nullptr);

        // Image Views de la Swap Chain
        for(int i=0; i<this->swap_chain.images.size(); i++)
            if(this->swap_chain.images[i].view != nullptr)
                vkDestroyImageView(this->device, this->swap_chain.images[i].view, nullptr);
        this->swap_chain.images.clear();

        // Swap Chain
        if(this->swap_chain.handle != nullptr) vkDestroySwapchainKHR(this->device, this->swap_chain.handle, nullptr);

        // Device
        if(this->device != nullptr && vkDestroyDevice) vkDestroyDevice(this->device, nullptr);

        // Surface
        if(this->presentation_surface != nullptr) vkDestroySurfaceKHR(this->instance, this->presentation_surface, nullptr);

        // Validation Layers
        #if defined(DISPLAY_LOGS)
        if(this->report_callback != nullptr) vkDestroyDebugUtilsMessengerEXT(this->instance, this->report_callback, nullptr);
        #endif

        // Instance
        if(this->instance != nullptr) vkDestroyInstance(this->instance, nullptr);

        // Librairie
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        if(this->library != nullptr) FreeLibrary(this->library);
        #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        if(this->library != nullptr) dlclose(this->library);
        #endif

        // On arrête d'écouter les évènements de la fenêtre
        this->draw_window->RemoveListener(this);
    }

    /**
     * Libération des resources
     */
    void Vulkan::DestroyInstance()
    {
        if(Vulkan::vulkan == nullptr) return;
        delete Vulkan::vulkan;
        Vulkan::vulkan = nullptr;
    }

    void Vulkan::StateChanged(IWindowListener::E_WINDOW_STATE window_state)
    {
    }

    void Vulkan::SizeChanged(Area<uint32_t> size)
    {
        this->draw_surface = draw_window->GetSurface();
    }

    /**
     * Récupère la version de Vulkan
     */
    bool Vulkan::GetVersion(std::string &version_string)
    {
        bool success = false;

        ///////////////////////////////
        //// Plateforme MS Windows ////
        ///////////////////////////////

        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        DWORD ver_handle;
        DWORD ver_size = GetFileVersionInfoSize(L"vulkan-1.dll", &ver_handle);
        if (ver_size > 0) {
            std::vector<char> ver_data(ver_size);
            if (GetFileVersionInfo(L"vulkan-1.dll", ver_handle, ver_size, ver_data.data())) {
                UINT size = 0;
                LPBYTE buffer = NULL;
                if (VerQueryValue(ver_data.data(), L"\\", (VOID FAR * FAR *)&buffer, &size)) {
                    if(size) {
                        VS_FIXEDFILEINFO *ver_info = (VS_FIXEDFILEINFO *)buffer;
                        if (ver_info->dwSignature == 0xfeef04bd) {
                            version_string = std::to_string((ver_info->dwFileVersionMS >> 16) & 0xffff);
                            version_string += ".";
                            version_string += std::to_string((ver_info->dwFileVersionMS >> 0) & 0xffff);
                            version_string += ".";
                            version_string += std::to_string((ver_info->dwFileVersionLS >> 16) & 0xffff);
                            success = true;
                        }
                    }
                }
            }
            ver_data.clear();
        }
        #endif

        return success;
    }

    /**
     * Création de l'instance Vulkan et chargement des fonction principales
     */
    Vulkan::INIT_RETURN_CODE Vulkan::Initialize(Engine::Window* draw_window, uint32_t application_version, std::string const& aplication_name, bool separate_transfer_queue)
    {
        // Valeur de retourn de la fonction
        INIT_RETURN_CODE init_status = INIT_RETURN_CODE::SUCCESS;

        // On récupère la fenêtre et la surface d'affichage
        this->draw_window = draw_window;
        this->draw_surface = draw_window->GetSurface();
        draw_window->AddListener(this);

        ////////////////////////////////
        // Chargement de la librairie //
        ////////////////////////////////

        // Chargement de la librairie vulkan
        if(!this->LoadPlatformLibrary())
            init_status = INIT_RETURN_CODE::LOAD_LIBRARY_FAILURE;

        ////////////////////
        // INITIALISATION //
        ////////////////////

        // Chargement de l'exporteur de fonctions vulkan
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->LoadExportedEntryPoints())
            init_status = INIT_RETURN_CODE::LOAD_EXPORTED_FUNCTIONS_FAILURE;

        // Chargement des fonctions vulkan globales
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->LoadGlobalLevelEntryPoints())
            init_status = INIT_RETURN_CODE::LOAD_GLOBAL_FUNCTIONS_FAILURE;

        // Création de l'instance vulkan
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->CreateVulkanInstance(application_version, aplication_name))
            init_status = INIT_RETURN_CODE::VULKAN_INSTANCE_CREATION_FAILURE;

        // Chargement des fonctions portant sur l'instance
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->LoadInstanceLevelEntryPoints())
            init_status = INIT_RETURN_CODE::LOAD_INSTANCE_FUNCTIONS_FAILURE;

        #if defined(DISPLAY_LOGS)
        this->report_callback = VK_NULL_HANDLE;
        this->CreateDebugReportCallback();
        #endif

        // Création de la surface de présentation
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->CreatePresentationSurface())
            init_status = INIT_RETURN_CODE::DEVICE_CREATION_FAILURE;

        // Création du logical device
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->CreateDevice(separate_transfer_queue))
            init_status = INIT_RETURN_CODE::DEVICE_CREATION_FAILURE;

        // Chargement des fonctions portant sur le logical device
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->LoadDeviceLevelEntryPoints())
            init_status = INIT_RETURN_CODE::LOAD_DEVICE_FUNCTIONS_FAILURE;

        // Récupère le handle des device queues
        this->GetDeviceQueues();

        // On recherche le meilleur format d'image pour le depth buffer
        this->depth_format = this->FindDepthFormat();

        // Création de la swap chain
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->CreateSwapChain())
            init_status = INIT_RETURN_CODE::SWAP_CHAIN_CREATION_FAILURE;

        // Création de la render pass
        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->CreateRenderPass())
            init_status = INIT_RETURN_CODE::RENDER_PASS_CREATION_FAILURE;

        // Création du command buffer principal
        /* if(init_status == INIT_RETURN_CODE::SUCCESS && !this->InitMainBuffers())
            init_status = INIT_RETURN_CODE::MAIN_THREAD_INITIALIZATION_FAILURE; */

        if(init_status == INIT_RETURN_CODE::SUCCESS && !this->AllocateTransferResources())
            init_status = INIT_RETURN_CODE::TRANSFER_ALLOCATION_FAILURE;

        // Création du depth buffer
        if(init_status == INIT_RETURN_CODE::SUCCESS) {
            this->depth_buffer = this->CreateImageBuffer(
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                this->draw_surface.width,
                this->draw_surface.height,
                this->depth_format
            );

            if(this->depth_buffer.view == VK_NULL_HANDLE)
                init_status = INIT_RETURN_CODE::DEPTH_BUFFER_CREATION_FAILURE;
        }

        // En cas d'échecx d'initialisation, on libère les ressources alouées
        if(init_status != INIT_RETURN_CODE::SUCCESS) Vulkan::DestroyInstance();

        // Renvoi du résultat de l'initilisation
        return init_status;
    }

    /**
     * Chargement de la librairie vulkan
     */
    bool Vulkan::LoadPlatformLibrary()
    {
        // Chargement de la DLL pour Windows
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        this->library = LoadLibrary(L"vulkan-1.dll");

        // Chargement de la librairie partagée pour Linux / MACOS
        #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        this->Library = dlopen("libvulkan.so.1", RTLD_NOW);

        #endif

        #if defined(DISPLAY_LOGS)
        std::cout << "LoadPlatformLibrary : " << ((this->library != nullptr) ? "Success" : "Failure") << std::endl;
        #endif

        // Succès si library n'est pas nul
        return this->library != nullptr;
    }

    /**
    * Chargement de la fonction Vulkan permettant d'exporter toutes les autres
    */
    bool Vulkan::LoadExportedEntryPoints()
    {
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        #define LoadProcAddress GetProcAddress
        #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        #define LoadProcAddress dlsym
        #endif

        #if defined(DISPLAY_LOGS)
        #define VK_EXPORTED_FUNCTION(fun)                                                           \
        if(!(fun = (PFN_##fun)LoadProcAddress(this->library, #fun))) {                                    \
            std::cout << "LoadExportedEntryPoints(" << #fun << ") : " << "Failure" << std::endl;    \
            return false;                                                                           \
        }
        #else
        #define VK_EXPORTED_FUNCTION(fun) \
        if(!(fun = (PFN_##fun)LoadProcAddress(this->library, #fun))) return false;
        #endif
        #include "ListOfFunctions.inl"

        #if defined(DISPLAY_LOGS)
        std::cout << "LoadExportedEntryPoints : Success" << std::endl;
        #endif

        // Succès
        return true;
    }

	/**
	* Chargement des fonctions Vulkan de portée globale
	*/
	bool Vulkan::LoadGlobalLevelEntryPoints()
	{
        #if defined(DISPLAY_LOGS)
        #define VK_GLOBAL_LEVEL_FUNCTION(fun)                                                           \
        if(!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) {                                  \
            std::cout << "LoadGlobalLevelEntryPoints(" << #fun << ") : " << "Failure" << std::endl;     \
            return false;                                                                               \
        }
        #else
        #define VK_GLOBAL_LEVEL_FUNCTION(fun) \
        if(!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) return false;
        #endif
        #include "ListOfFunctions.inl"

        #if defined(DISPLAY_LOGS)
        std::cout << "LoadGlobalLevelEntryPoints : Success" << std::endl;
        #endif

		// Succès
		return true;
	}

    /**
     * Charge les fonctions Vulkan liées à l'instance
     */
    bool Vulkan::LoadInstanceLevelEntryPoints()
    {
        #if defined(DISPLAY_LOGS)
        #define VK_INSTANCE_LEVEL_FUNCTION(fun)                                                         \
        if(!(fun = (PFN_##fun)vkGetInstanceProcAddr(this->instance, #fun))) {                           \
            std::cout << "LoadInstanceLevelEntryPoints(" << #fun << ") : " << "Failure" << std::endl;   \
            return false;                                                                               \
        }
        #else
        #define VK_INSTANCE_LEVEL_FUNCTION(fun) \
        if(!(fun = (PFN_##fun)vkGetInstanceProcAddr(this->instance, #fun))) return false;
        #endif
        #include "ListOfFunctions.inl"

        #if defined(DISPLAY_LOGS)
        std::cout << "LoadInstanceLevelEntryPoints : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Chargement des fonctions Vulkan liées au GPU logique
     */
    bool Vulkan::LoadDeviceLevelEntryPoints()
    {
        #if defined(DISPLAY_LOGS)
        #define VK_DEVICE_LEVEL_FUNCTION(fun)                                                           \
        if(!(fun = (PFN_##fun)vkGetDeviceProcAddr(this->device, #fun))) {                               \
            std::cout << "LoadDeviceLevelEntryPoints(" << #fun << ") : " << "Failure" << std::endl;     \
            return false;                                                                               \
        }
        #else
        #define VK_DEVICE_LEVEL_FUNCTION(fun) \
        if(!(fun = (PFN_##fun)vkGetDeviceProcAddr(this->device, #fun))) return false;
        #endif
        #include "ListOfFunctions.inl"

        #if defined(DISPLAY_LOGS)
        std::cout << "LoadDeviceLevelEntryPoints : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Création de l'instance Vulkan
     */
    bool Vulkan::CreateVulkanInstance(uint32_t application_version, std::string const& aplication_name)
    {
        // Initialisation de la structure VkApplicationInfo
        VkApplicationInfo app_info;
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext = VK_NULL_HANDLE;
        app_info.pApplicationName = aplication_name.c_str();
        app_info.applicationVersion = application_version;
        app_info.pEngineName = ENGINE_NAME;
        app_info.engineVersion = ENGINE_VERSION;
        app_info.apiVersion = VK_API_VERSION_1_0;

        // Activation de l'extension SURFACE
        std::vector<const char*> instance_extension_names = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            #if defined(VK_USE_PLATFORM_WIN32_KHR)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            #elif defined(VK_USE_PLATFORM_XCB_KHR)
            VK_KHR_XCB_SURFACE_EXTENSION_NAME
            #elif defined(VK_USE_PLATFORM_XLIB_KHR)
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME
            #endif
        };

        // Initialisation de la structure VkInstanceCreateInfo
        VkInstanceCreateInfo inst_info;
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pNext = nullptr;
        inst_info.flags = 0;
        inst_info.pApplicationInfo = &app_info;
        inst_info.enabledExtensionCount = static_cast<uint32_t>(instance_extension_names.size());
        inst_info.ppEnabledExtensionNames = instance_extension_names.data();
        inst_info.enabledLayerCount = 0;
        inst_info.ppEnabledLayerNames = nullptr;

        ///////////////////////
        //  Activation des   //
        // Validation Layers //
        //   (mode debug)    //
        ///////////////////////

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
         
        // Ajoute du valideur de bonne pratiques
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

        // Modification de la structure VkInstanceCreateInfo pour prendre en compte les validation layers
        inst_info.enabledLayerCount = static_cast<uint32_t>(instance_layer_names.size());
        inst_info.ppEnabledLayerNames = instance_layer_names.data();
        inst_info.enabledExtensionCount = static_cast<uint32_t>(instance_extension_names.size());
        inst_info.ppEnabledExtensionNames = instance_extension_names.data();
        #if !defined(USE_RENDERDOC)
        inst_info.pNext = &validation_features;
        #endif
        #endif

        // Création de l'instance Vulkan
        VkResult result = vkCreateInstance(&inst_info, nullptr, &this->instance);

        #if defined(DISPLAY_LOGS)
        std::cout << "CreateVulkanInstance : " << ((result == VK_SUCCESS) ? "Success" : "Failure") << std::endl;
        #endif

        // Succès
        return result == VK_SUCCESS;
    }

    /**
     * Création de la sufrace de présentation permettant l'affichage des graphismes sur une fenêtre
     */
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
        std::cout << "CreatePresentationSurface : " << ((result == VK_SUCCESS) ? "Success" : "Failure") << std::endl;
        #endif

        return result == VK_SUCCESS;
    }

    /**
     * Création du logical device Vulkan
     */
    bool Vulkan::CreateDevice(bool separate_transfer_queue, char preferred_device_index)
    {
        // En premier lieu on énumère les différents périphériques compatibles avec Vulkan afin d'en trouver un qui soit compatible avec notre application

        // Récupération du nombre de périphériques
        uint32_t physical_device_count = 1;
        VkResult result = vkEnumeratePhysicalDevices(this->instance, &physical_device_count, nullptr);
        if(result != VK_SUCCESS || physical_device_count == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateDevice => vkEnumeratePhysicalDevices 1/2 : Failed" << std::endl;
            #endif
            return false;
        }

        // Liste des périphériques
        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        result = vkEnumeratePhysicalDevices(this->instance, &physical_device_count, &physical_devices[0]);
        if(result != VK_SUCCESS || physical_device_count == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateDevice => vkEnumeratePhysicalDevices 2/2 : Failed" << std::endl;
            #endif
            return false;
        }

        // Si un périphérique préféré a été donné en argument, on vérifie qu'il est compatible avec notre application, auquel cas on choisira celui-ci
        std::vector<VkQueueFamilyProperties> queue_family_properties;
        if(preferred_device_index >= 0) {
            queue_family_properties = this->QueryDeviceProperties(physical_devices[preferred_device_index]);
            if(preferred_device_index < (int)physical_device_count && this->IsDeviceEligible(physical_devices[preferred_device_index], queue_family_properties)) {
                this->physical_device.handle = physical_devices[preferred_device_index];
                #if defined(DISPLAY_LOGS)
                std::cout << "CreateDevice : Selected physical device : " << preferred_device_index << std::endl;
                #endif
            }
        }else{
            for(uint32_t i=0; i<physical_devices.size(); i++) {
                queue_family_properties = this->QueryDeviceProperties(physical_devices[i]);
                if(this->IsDeviceEligible(physical_devices[0], queue_family_properties)) {
                    this->physical_device.handle = physical_devices[0];
                    #if defined(DISPLAY_LOGS)
                    std::cout << "CreateDevice : Selected physical device : " << i << std::endl;
                    #endif
                    break;
                }
            }
        }

        if(this->physical_device.handle == VK_NULL_HANDLE) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateDevice : No eligible device found" << std::endl;
            #endif
            return false;
        }

        // Sélection de la present queue
        this->present_queue.index = this->SelectPresentQueue(this->physical_device.handle, queue_family_properties);

        // Sélection des autres queues
        this->graphics_queue.index = this->SelectPreferredQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_GRAPHICS_BIT, this->present_queue.index);
        if(separate_transfer_queue)
            this->transfer_queue.index = this->SelectPreferredQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_TRANSFER_BIT, this->present_queue.index);
        else this->transfer_queue.index = this->graphics_queue.index;
       
        #if defined(DISPLAY_LOGS)
        std::cout << "CreateDevice : Graphics queue family : " << this->graphics_queue.index << ", count : " << queue_family_properties[this->graphics_queue.index].queueCount << std::endl;
        std::cout << "CreateDevice : Transfert queue family : " << this->transfer_queue.index << ", count : " << queue_family_properties[this->transfer_queue.index].queueCount << std::endl;
        std::cout << "CreateDevice : Present queue family : " << this->present_queue.index << ", count : " << queue_family_properties[this->present_queue.index].queueCount << std::endl;
        #endif

        // Préparation de la création des queues
        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        std::array<uint32_t, 3> queues = {this->present_queue.index, this->graphics_queue.index, this->transfer_queue.index};
        std::vector<float> queue_priorities;// = {0.0f};

        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = nullptr;

        // Graphics Queue
        // Une queue principale pour l'affichage,
        // si possible une queue secondaire pour les textures
        if(queue_family_properties[this->graphics_queue.index].queueCount > 1) queue_priorities = {0.0f, 0.0f};
        else queue_priorities = {0.0f};

        queue_info.queueCount = static_cast<uint32_t>(queue_priorities.size());
        queue_info.pQueuePriorities = &queue_priorities[0];
        queue_infos.push_back(queue_info);

        // Present Queue
        // Une seule queue nécessaire, si possible dédiée
        if(this->present_queue.index != this->graphics_queue.index) {
            queue_info.queueCount = 1;
            queue_info.queueFamilyIndex = this->present_queue.index;
            queue_infos.push_back(queue_info);
        }

        // Transfer Queue
        // Une queue principale pour la boucle principale,
        // si possible une queue secondaire pour les textures
        if(this->transfer_queue.index != this->graphics_queue.index) {
            
            if(queue_family_properties[this->transfer_queue.index].queueCount > 1) queue_priorities = {0.0f, 0.0f};
            else queue_priorities = {0.0f};

            queue_info.queueCount = static_cast<uint32_t>(queue_priorities.size());
            queue_info.pQueuePriorities = &queue_priorities[0];
            queue_info.queueFamilyIndex = this->transfer_queue.index;
            queue_infos.push_back(queue_info);
        }

        // Activation du Geometry Shader
        VkPhysicalDeviceFeatures features = {};
        features.vertexPipelineStoresAndAtomics = VK_TRUE;
        features.fillModeNonSolid = VK_TRUE;
        #if defined(DISPLAY_LOGS)
        features.geometryShader = VK_TRUE;
        features.wideLines = VK_TRUE;
        #endif

        // Activation de l'extension SwapChain
        std::vector<const char*> device_extension_name = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = nullptr;
        device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
        device_info.pQueueCreateInfos = &queue_infos[0];
        device_info.enabledExtensionCount = static_cast<uint32_t>(device_extension_name.size());
        device_info.ppEnabledExtensionNames = &device_extension_name[0];
        device_info.enabledLayerCount = 0;
        device_info.ppEnabledLayerNames = nullptr;
        device_info.pEnabledFeatures = &features;

        // On récupère les fonctionalités de la carte graphique
        vkGetPhysicalDeviceFeatures(this->physical_device.handle, &this->physical_device.features);

        result = vkCreateDevice(this->physical_device.handle, &device_info, nullptr, &this->device);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateDevice => vkCreateDevice : Failed" << std::endl;
            #endif
            return false;
        }

        // On récupère les propriétés mémoire de la carte graphique
        vkGetPhysicalDeviceMemoryProperties(this->physical_device.handle, &this->physical_device.memory);
        vkGetPhysicalDeviceProperties(this->physical_device.handle, &this->physical_device.properties);

        // Succès
        #if defined(DISPLAY_LOGS)
        std::cout << "CreateDevice : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Renvoie la liste des queue family properties d'un périphérique donné
     */
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

    /**
     * Valide la compatibilité d'un périphérique physique avec les besoins de l'application
     * Dans notre cas, nous voulons un périphérique possédant une graphics queue ainsi que la capacité de présenter une surface
     */
    bool Vulkan::IsDeviceEligible(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties)
    {
        bool present_queue_found = false;
        bool graphics_queue_found = false;

        // On vérifie pour chaque queue family si elle supporte la présentation ou les graphismes
        for(uint32_t i = 0; i < queue_family_properties.size(); i++) {
            VkBool32 support_present;
            vkGetPhysicalDeviceSurfaceSupportKHR(test_physical_device, i, this->presentation_surface, &support_present);
            if(support_present == VK_TRUE) present_queue_found = true;
            if((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) graphics_queue_found = true;
            if(present_queue_found && graphics_queue_found) return true;
        }
        
        return false;
    }

    /**
     * On sélectionne parmi les queue families compatibles avec la présentation, celle qui possède le plus de queues
     */
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

    /**
     * Permet de sélectionner une queue family en suivant ces priorités :
     * 1 - une queue family dédiée
     * 2 - la queue family déjà utilisée pour les graphismes
     * 3 - la queue family qui propose le plus de queues parmi celles restantes
     */
    uint32_t Vulkan::SelectPreferredQueue(VkPhysicalDevice test_physical_device, std::vector<VkQueueFamilyProperties>& queue_family_properties, VkQueueFlagBits queue_type, uint32_t common_queue)
    {
        uint32_t index = UINT32_MAX;

        // On cherche une queue dédiée en priorité
        for(uint32_t i = 0; i < queue_family_properties.size(); i++)
            if((queue_family_properties[i].queueFlags & ~VK_QUEUE_SPARSE_BINDING_BIT) == queue_type)
                return i;

        // Si la common queue est compatible, on prends celle-ci
        if((queue_family_properties[common_queue].queueFlags & queue_type) != 0) return common_queue;

        // En dernier choix, on prend la queue family qui propose le plus de queues parmi celles restantes
        for(uint32_t i = 0; i < queue_family_properties.size(); i++)
            if((queue_family_properties[i].queueFlags & queue_type) != 0
                && (index > queue_family_properties.size() || queue_family_properties[i].queueCount > queue_family_properties[index].queueCount))
                    index = i;

        return index;
    }

    /**
     * Récupère le handle des device queues
     */
    void Vulkan::GetDeviceQueues()
    {
        vkGetDeviceQueue(this->device, this->present_queue.index, 0, &this->present_queue.handle);
        vkGetDeviceQueue(this->device, this->graphics_queue.index, 0, &this->graphics_queue.handle);
        vkGetDeviceQueue(this->device, this->transfer_queue.index, 0, &this->transfer_queue.handle);

        std::vector<VkQueueFamilyProperties> queue_family_properties = this->QueryDeviceProperties(this->physical_device.handle);
    }

    /**
     * Création de la Swap Chain
     */
    bool Vulkan::CreateSwapChain()
    {
        // En cas de reconstruction de la swap chain, on évite que la fonction soit appelée plusieurs fois simultanément
        this->creating_swap_chain = true;

        // En cas de redimensionnement de la fenêtre il faudra recréer la swap chain,
        // c'est pourquoi il faut toujours s'assurer qu'aucun traitement est en cours sur le gpu avant de recréer la swap chain
        vkDeviceWaitIdle(this->device);

        // Récupération des capacités de la surface de présentation
        VkSurfaceCapabilitiesKHR surface_capabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physical_device.handle, this->presentation_surface, &surface_capabilities);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSwapChain => vkGetPhysicalDeviceSurfaceCapabilitiesKHR : Failed" << std::endl;
            #endif
            this->creating_swap_chain = false;
            return false;
        }

        //On apelle tous nos helpers pour récupérer toutes les informations qui constitueront la SwapChain
        VkExtent2D swap_chain_extent = this->GetSwapChainExtent(surface_capabilities);
        this->concurrent_frame_count = static_cast<uint8_t>(this->GetSwapChainNumImages(surface_capabilities));
        this->swap_chain.format = this->GetSurfaceFormat();
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
        swapchain_ci.minImageCount = this->concurrent_frame_count;
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
            // Si les queues family graphique et présentation sont distinctes,
            // on utilise le mode VK_SHARING_MODE_CONCURRENT pour éviter d'avoir
            // à spécifier explicitement à quelle queue est attribuée quelle image
            swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchain_ci.queueFamilyIndexCount = static_cast<uint32_t>(shared_queue_families.size());
            swapchain_ci.pQueueFamilyIndices = &shared_queue_families[0];
        }

        // Création de la nouvelle swap chain
        result = vkCreateSwapchainKHR(this->device, &swapchain_ci, nullptr, &this->swap_chain.handle);

        // destruction de l'ancienne swap chain
        for(int i=0; i<this->swap_chain.images.size(); i++)
            if(this->swap_chain.images[i].view != VK_NULL_HANDLE)
                vkDestroyImageView(this->device, this->swap_chain.images[i].view, nullptr);
        this->swap_chain.images.clear();
        if(old_swap_chain != VK_NULL_HANDLE) vkDestroySwapchainKHR(this->device, old_swap_chain, nullptr);

        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSwapChain => vkCreateSwapchainKHR : Failed" << std::endl;
            #endif
            this->creating_swap_chain = false;
            return false;
        }

        // Création des images de la SwapChain
        uint32_t swap_chain_image_count;
        result = vkGetSwapchainImagesKHR(this->device, this->swap_chain.handle, &swap_chain_image_count, nullptr);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSwapChain => vkGetSwapchainImagesKHR 1/2 : Failed" << std::endl;
            #endif
            this->creating_swap_chain = false;
            return false;
        }

        // Récupération des images
        std::vector<VkImage> swap_chain_image(swap_chain_image_count);
        result = vkGetSwapchainImagesKHR(this->device, this->swap_chain.handle, &swap_chain_image_count, &swap_chain_image[0]);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSwapChain => vkGetSwapchainImagesKHR 2/2 : Failed" << std::endl;
            #endif
            this->creating_swap_chain = false;
            return false;
        }

        // Prepare subresource range for image presentation
        this->image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        this->image_subresource_range.baseMipLevel = 0;
        this->image_subresource_range.levelCount = 1;
        this->image_subresource_range.baseArrayLayer = 0;
        this->image_subresource_range.layerCount = 1;

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
            color_image_view.subresourceRange = this->image_subresource_range;

            VkResult result = vkCreateImageView(this->device, &color_image_view, nullptr, &this->swap_chain.images[i].view);
            if(result != VK_SUCCESS) {
                #if defined(DISPLAY_LOGS)
                std::cout << "CreateSwapChain => vkCreateImageView[" << i << "] : Failed" << std::endl;
                #endif
                this->creating_swap_chain = false;
                return false;
            }
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "CreateSwapChain : Success" << std::endl;
        #endif

        this->creating_swap_chain = false;
        return true;
    }

    /**
     * Détermine la taille des images de la SwapChain
     */
    VkExtent2D Vulkan::GetSwapChainExtent(VkSurfaceCapabilitiesKHR surface_capabilities)
    {
        // Si la taille de la surface est indéfinie (valeur = -1),
        // on la choisi nous-même mais dans les limites acceptées par la surface de présentation
        if(surface_capabilities.currentExtent.width == -1) {
            VkExtent2D swap_chain_extent = {this->draw_surface.width, this->draw_surface.height};
            if(swap_chain_extent.width < surface_capabilities.minImageExtent.width) swap_chain_extent.width = surface_capabilities.minImageExtent.width;
            if(swap_chain_extent.height < surface_capabilities.minImageExtent.height) swap_chain_extent.height = surface_capabilities.minImageExtent.height;
            if(swap_chain_extent.width > surface_capabilities.maxImageExtent.width) swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
            if(swap_chain_extent.height > surface_capabilities.maxImageExtent.height) swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
            return swap_chain_extent;
        }

        // La plupart du temps, la taille des images de la SwapChaine sera égale à la taille de la fenêtre
        return surface_capabilities.currentExtent;
    }

    /**
     * Détermine le nombre d'images utilisées par la SwapChain
     */
    uint32_t Vulkan::GetSwapChainNumImages(VkSurfaceCapabilitiesKHR surface_capabilities)
    {
        uint32_t image_count = surface_capabilities.minImageCount + 1;
        if(surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) image_count = surface_capabilities.maxImageCount;
        return image_count;
    }

    /**
     * Récupère le meilleurs format de surface compatible avec la fenêtre
     */
    VkFormat Vulkan::GetSurfaceFormat()
    {
        // Récupère la liste des VkFormats supportés
        uint32_t formatCount = UINT32_MAX;
        VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(this->physical_device.handle, this->presentation_surface, &formatCount, nullptr);
        if(res != VK_SUCCESS || formatCount == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "GetSurfaceFormat => vkGetPhysicalDeviceSurfaceFormatsKHR 1/2 : Failed" << std::endl;
            #endif
            return VK_FORMAT_UNDEFINED;
        }

        std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(this->physical_device.handle, this->presentation_surface, &formatCount, &surfFormats[0]);
        if(res != VK_SUCCESS || formatCount == 0) {
            #if defined(DISPLAY_LOGS)
            std::cout << "GetSurfaceFormat => vkGetPhysicalDeviceSurfaceFormatsKHR 2/2 : Failed" << std::endl;
            #endif
            return VK_FORMAT_UNDEFINED;
        }

        // Si la liste de formats inclue uniquement VK_FORMAT_UNDEFINED,
        // c'est qu'il n'y a pas de format préféré. Sinon, au moins un format sera renvoyé.
        if(formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) return VK_FORMAT_B8G8R8A8_UNORM;
        else return surfFormats[0].format;
    }

    /**
     * Détermine le type de transformation utilisée pour présenter l'image
     */
    VkSurfaceTransformFlagBitsKHR Vulkan::GetSwapChainTransform(VkSurfaceCapabilitiesKHR surface_capabilities)
    {
        // Parfois les images peuvent subir des transformations avant d'être présentées, par exemple lors de la rotation d'un appareil mobile.
        // Cette transformation peut avoir un impact sur les performances, dans notre cas, nous n'appliquerons aucune transformation.
        if(surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        else return surface_capabilities.currentTransform;
    }

    /**
     * Détermine le mode composite alpha supporté par la SwapChain
     */
    VkCompositeAlphaFlagBitsKHR Vulkan::GetSwapChainCompositeAlpha(VkSurfaceCapabilitiesKHR surface_capabilities)
    {
        if(surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;    // Laisse Vulkan traiter la composante alpha et non-alpha
        if(surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;      // Vulkan ne prend en chaque que la composante alpha
        if(surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                      // Vulkan ignore la composante alpha
        if(surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;                    // Hérite de la méthode utilisée par la surface
        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    /**
     * Détermine le mode de présentation utilisé pour la swap chain, de préférence on choisira le mode Mailbox
     * En cas d'échec, c'est le mode FIFO qui sera choisi car il est guaranti de fonctionner selon les specs Vulkan
     */
    VkPresentModeKHR Vulkan::GetSwapChainPresentMode()
    {
        // Mais dans notre cas on voudrait utiliser le mode MAILBOX
        // Donc on établit la liste des modes compatibles
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

        // On valide la disponibilité du mode MAILBOX
        for(VkPresentModeKHR &present_mode : present_modes)
            if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return VK_PRESENT_MODE_MAILBOX_KHR;

        // Le mode MAILBOX est indisponible
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /**
     * Recherche d'un format adapté pour le depth buffer
     */
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
        std::cout << "FindDepthFormat => Format : VK_FORMAT_UNDEFINED" << std::endl;
        #endif
        return VK_FORMAT_UNDEFINED;
    }

    /**
     * Création de la render pass
     */
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

        VkSubpassDependency dependency[2];

        dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	    dependency[0].dstSubpass = 0;
	    dependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    dependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	    dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	    dependency[1].srcSubpass = 0;
	    dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	    dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    dependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    dependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    dependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	    dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = nullptr;
        rp_info.attachmentCount = 2;
        rp_info.pAttachments = attachment;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 2;
        rp_info.pDependencies = dependency;

        VkResult result = vkCreateRenderPass(this->device, &rp_info, nullptr, &this->render_pass);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateRenderPass => vkCreateRenderPass : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "CreateRenderPass : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Chargement d'un shader
     */
    VkPipelineShaderStageCreateInfo Vulkan::LoadShaderModule(std::string const& filename, VkShaderStageFlagBits type)
    {
        std::vector<char> code = Tools::GetBinaryFileContents(filename);
        if(!code.size()) {
            #if defined(DISPLAY_LOGS)
            std::cout << "LoadShaderModule => GetBinaryFileContents : Failed" << std::endl;
            #endif
            return {};
        }

        VkShaderModuleCreateInfo shader_module_create_info;
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.pNext = nullptr;
        shader_module_create_info.flags = 0;
        shader_module_create_info.codeSize = code.size();
        shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule module;
		VkResult result = vkCreateShaderModule(this->device, &shader_module_create_info, nullptr, &module);
		if(result != VK_SUCCESS) {
			#if defined(DISPLAY_LOGS)
            std::cout << "LoadShaderModule => vkCreateShaderModule : Failed" << std::endl;
            #endif
			return {};
		}

        VkPipelineShaderStageCreateInfo shader_stage_create_infos;
        shader_stage_create_infos.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_infos.pNext = nullptr;
        shader_stage_create_infos.flags = 0;
        shader_stage_create_infos.stage = type;
        shader_stage_create_infos.module = module;
        shader_stage_create_infos.pName = "main";
        shader_stage_create_infos.pSpecializationInfo = nullptr;

        return shader_stage_create_infos;
    }

    /**
     * Récupère l'index du type de mémoire qui correspond au flag recherché
     */
    bool Vulkan::MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, uint32_t &type_index)
    {
        // Parcour de la liste de types de mémoire
        for(uint32_t i = 0; i < this->physical_device.memory.memoryTypeCount; i++) {
            if((type_bits & 1) == 1) {

                // Pour chaque type de mémoire on vérifie la présence du flag
                if((this->physical_device.memory.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {

                    // Si le flag est là, on modifie la valeur de "type_index" qui est passé par référence
                    type_index = i;
                    return true;
                }
            }
            type_bits >>= 1;
        }

        // Échec
        return false;
    }

    /**
     * Création du depth buffer
     */
    Vulkan::IMAGE_BUFFER Vulkan::CreateImageBuffer(VkImageUsageFlags usage, VkImageAspectFlags aspect, uint32_t width, uint32_t height, VkFormat format)
    {
        IMAGE_BUFFER image_buffer = {};
        image_buffer.format = format;
        image_buffer.aspect = aspect;

        /////////////////////////
        // Création de l'image //
        /////////////////////////

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = NULL;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = image_buffer.format;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = NULL;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.flags = 0;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

        VkResult result = vkCreateImage(this->device, &image_info, NULL, &image_buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateImageBuffer => vkCreateImage : Failed" << std::endl;
            #endif
            return image_buffer;
        }

        ///////////////////////////////////////////////////////
        //      Réservation d'un segment de mémoire          //
        // dans la carte graphique pour y placer la vue      //
        ///////////////////////////////////////////////////////

        VkMemoryRequirements image_memory_requirements;
        vkGetImageMemoryRequirements(this->device, image_buffer.handle, &image_memory_requirements);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = image_memory_requirements.size;
        
        if(!this->MemoryTypeFromProperties(image_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_alloc.memoryTypeIndex)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateImageBuffer => MemoryType VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : Not found" << std::endl;
            #endif
            return image_buffer;
        }

        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &image_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateImageBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return image_buffer;
        }

        result = vkBindImageMemory(this->device, image_buffer.handle, image_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateImageBuffer => vkBindImageMemory : Failed" << std::endl;
            #endif
            return image_buffer;
        }

        //////////////////////////////
        // Création de l'image view //
        //////////////////////////////

        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.pNext = nullptr;
        view_info.image = VK_NULL_HANDLE;
        view_info.format = image_buffer.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = image_buffer.aspect; // VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.flags = 0;
        view_info.image = image_buffer.handle;

        result = vkCreateImageView(this->device, &view_info, nullptr, &image_buffer.view);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateImageBuffer => vkCreateImageView : Failed" << std::endl;
            #endif
            return image_buffer;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "CreateImageBuffer : Success" << std::endl;
        #endif

        return image_buffer;
    }

    /**
     * Création des frame buffers
     */
    bool Vulkan::CreateFrameBuffer(VkFramebuffer& frame_buffer, VkImageView const view)
    {
        VkImageView pAttachments[2];
        pAttachments[0] = view;
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

        VkResult result = vkCreateFramebuffer(this->device, &framebuffer_create_info, nullptr, &frame_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateFrameBuffer => vkCreateFramebuffer : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "CreateFrameBuffer : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Création d'un command pool
     */
    bool Vulkan::CreateCommandPool(VkCommandPool& pool, uint32_t queue_family_index, VkCommandPoolCreateFlags flags)
    {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.queueFamilyIndex = queue_family_index;
        cmd_pool_info.flags = flags; // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VkResult res = vkCreateCommandPool(this->device, &cmd_pool_info, nullptr, &pool);
        if(res != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateCommandPool => vkCreateCommandPool : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    /**
     * Création d'une liste de command buffers avec leur fence
     */
    bool Vulkan::CreateCommandBuffer(VkCommandPool pool, std::vector<COMMAND_BUFFER>& command_buffers, VkCommandBufferLevel level, bool create_fence)
    {
        // Allocation des command buffers
        std::vector<VkCommandBuffer> output_buffers(command_buffers.size());
        if(!this->AllocateCommandBuffer(pool, output_buffers, level)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateCommandBuffer => AllocateCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }
        for(uint8_t i=0; i<command_buffers.size(); i++) command_buffers[i].handle = output_buffers[i];
        // vkResetCommandPool(this->device, pool, 0);

        // Création des fences
        if(create_fence) {
            for(auto& command_buffer : command_buffers) {

                command_buffer.fence = this->CreateFence();
                if(command_buffer.fence == nullptr) return false;
            }
        }

        return true;
    }

    VkFence Vulkan::CreateFence()
    {
        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = nullptr;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkFence fence;
        VkResult result = vkCreateFence(this->device, &fence_create_info, nullptr, &fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "Vulkan::CreateFence() => vkCreateFence : Failed" << std::endl;
            #endif
            return nullptr;
        }

        return fence;
    }

    /*
     * Libère les ressources alouées à un command buffer ainsi que son command pool
     */
    void Vulkan::ReleaseCommandBuffer(VkCommandPool pool, std::vector<COMMAND_BUFFER>& command_buffers)
    {
        for(auto& buffer : command_buffers)
            if(buffer.fence != VK_NULL_HANDLE)
                vkDestroyFence(this->device, buffer.fence, nullptr);
        if(pool != VK_NULL_HANDLE) vkDestroyCommandPool(this->device, pool, nullptr);
        command_buffers.clear();
    }

    /**
     * Allocation des command buffers
     */
    bool Vulkan::AllocateCommandBuffer(VkCommandPool& pool, std::vector<VkCommandBuffer>& buffers, VkCommandBufferLevel level)
    {
        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = pool;
        cmd.level = level;
        cmd.commandBufferCount = static_cast<uint32_t>(buffers.size());

        VkResult result = vkAllocateCommandBuffers(this->device, &cmd, buffers.data());
        return result == VK_SUCCESS;
    }

    /**
     * Création d'un sémaphore
     */
    bool Vulkan::CreateSemaphore(VkSemaphore &semaphore)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = nullptr;
        semaphore_create_info.flags = 0;

        VkResult result = vkCreateSemaphore(this->device, &semaphore_create_info, nullptr, &semaphore);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateSemaphore => vkCreateSemaphore : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    void Vulkan::ReleaseDepthBuffer()
    {
        // Depth buffer
        if(this->depth_buffer.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->depth_buffer.memory, nullptr);
        if(this->depth_buffer.view != VK_NULL_HANDLE) vkDestroyImageView(this->device, this->depth_buffer.view, nullptr);
        if(this->depth_buffer.handle != VK_NULL_HANDLE) vkDestroyImage(this->device, this->depth_buffer.handle, nullptr);
    }

    bool Vulkan::AllocateTransferResources()
    {
        // Transfert Command Pool
        if(!this->CreateCommandPool(this->transfert_command_pool, this->transfer_queue.index)) return false;

        // Transfert Command buffer
        std::vector<COMMAND_BUFFER> buffers(1);
        if(!this->CreateCommandBuffer(this->transfert_command_pool, buffers)) return false;
        this->transfer_command_buffer = buffers[0];
        buffers.clear();

        // Staging buffer
        std::vector<uint32_t> queue_families = {Vulkan::GetGraphicsQueue().index};
        if(Vulkan::GetGraphicsQueue().index != Vulkan::GetTransferQueue().index) queue_families.push_back(Vulkan::GetTransferQueue().index);
        if(!Vulkan::GetInstance().CreateDataBuffer(this->transfer_staging, 1024 * 1024 * 20,
                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                   queue_families)) return false;

        constexpr auto offset = 0;
        VkResult result = vkMapMemory(Vulkan::GetDevice(), this->transfer_staging.memory, offset, 1024 * 1024 * 5, 0, (void**)&this->transfer_staging.pointer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "AllocateTransferResources => vkMapMemory(staging_buffer) : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    void Vulkan::ReleaseTransferResources()
    {
        std::vector<COMMAND_BUFFER> buffers = {this->transfer_command_buffer};
        this->ReleaseCommandBuffer(this->transfert_command_pool, buffers);
        this->transfer_command_buffer = {};

        this->transfer_staging.Clear();
    }

    /**
     * Détruit tous les buffers principaux
     */
    /*void Vulkan::ReleaseMainBuffers()
    {
        // Command buffers
        std::vector<COMMAND_BUFFER> buffers = {this->main_command_buffer};
        this->ReleaseCommandBuffer(this->main_command_pool, buffers);
        this->main_command_buffer = {};

        if(this->graphics_queue.index != this->transfer_queue.index) {
            buffers = {this->transfer_command_buffer};
            this->ReleaseCommandBuffer(this->transfert_command_pool, buffers);
            this->transfer_command_buffer = {};
        }

        // Staging buffer
        this->ReleaseDataBuffer(this->staging_buffer);
    }*/

    /**
     * Création du command buffer principal
     */
    /*bool Vulkan::InitMainBuffers()
    {
        // Command Pool principal
        if(!this->CreateCommandPool(this->main_command_pool, this->graphics_queue.index)) return false;

        // Command buffer principal
        std::vector<COMMAND_BUFFER> buffers(1);
        if(!this->CreateCommandBuffer(this->main_command_pool, buffers)) return false;
        this->main_command_buffer = buffers[0];
        buffers.clear();

        // Si on a une queue dédiée aux transferts, on créé un command buffer dédié
        if(this->graphics_queue.index != this->transfer_queue.index) {

            // Transfert Command Pool
            if(!this->CreateCommandPool(this->transfert_command_pool, this->transfer_queue.index)) return false;

            // Transfert Command buffer
            buffers.resize(1);
            if(!this->CreateCommandBuffer(this->transfert_command_pool, buffers)) return false;
            this->transfer_command_buffer = buffers[0];
            buffers.clear();
        }else{
            
            // Si la transfer queue est identique à la graphics queue, on utilise le même command buffer
            this->transfer_command_buffer = this->main_command_buffer;
        }

        // Staging buffer
        std::vector<uint32_t> queue_families = {this->graphics_queue.index};
        if(this->graphics_queue.index != this->transfer_queue.index) queue_families.push_back(this->transfer_queue.index);
        if(!this->CreateDataBuffer(this->staging_buffer, STAGING_BUFFER_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, queue_families)) return false;

        constexpr auto offset = 0;
        VkResult result = vkMapMemory(this->device, this->staging_buffer.memory, offset, STAGING_BUFFER_SIZE, 0, &this->staging_pointer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "InitMainBuffers => vkMapMemory(staging_buffer) : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "InitMainBuffers : Success" << std::endl;
        #endif

        // Succès
        return true;
    }*/

    /**
     * Reconstruit le staging buffer avec une nouvelle taille
     */
    /*bool Vulkan::ResizeStagingBuffer(VkDeviceSize size)
    {
        // Destruction du staging buffer
        this->ReleaseDataBuffer(this->staging_buffer);

        // Création du staging buffer à la taille souhaitée
        std::vector<uint32_t> queue_families = {this->graphics_queue.index};
        if(this->graphics_queue.index != this->transfer_queue.index) queue_families.push_back(this->transfer_queue.index);
        if(!this->CreateDataBuffer(this->staging_buffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, queue_families)) return false;

        constexpr auto offset = 0;
        VkResult result = vkMapMemory(this->device, this->staging_buffer.memory, offset, size, 0, &this->staging_pointer);
        return result == VK_SUCCESS;
    }*/

    /**
     * Destruction d'un buffer de données
     */
    void Vulkan::ReleaseDataBuffer(DATA_BUFFER& buffer)
    {
        if(buffer.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, buffer.memory, nullptr); // vkUnmapMemory est implicite
        if(buffer.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, buffer.handle, nullptr);
        buffer = {};
    }

    /**
     * Création d'un buffer de données
     */
    bool Vulkan::CreateDataBuffer(DATA_BUFFER& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkFlags requirement, std::vector<uint32_t> const& queue_families)
    {
        ////////////////////////////////
        //     Création du buffer     //
        ////////////////////////////////

        buffer.size = size;

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = buffer.size;
        buffer_create_info.usage = usage; // VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size());
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.pQueueFamilyIndices = queue_families.data();
        if(queue_families.size() > 1) buffer_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;

        VkResult result = vkCreateBuffer(Vulkan::vulkan->device, &buffer_create_info, nullptr, &buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateDataBuffer => vkCreateBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(Vulkan::vulkan->device, buffer.handle, &mem_reqs);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;
        mem_alloc.allocationSize = mem_reqs.size;
        
        // requirement = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if(!Vulkan::vulkan->MemoryTypeFromProperties(mem_reqs.memoryTypeBits, requirement, mem_alloc.memoryTypeIndex)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateDataBuffer => MemoryType VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : Not found" << std::endl;
            #endif
            return false;
        }

        // Allocation de la mémoire pour le buffer
        result = vkAllocateMemory(Vulkan::vulkan->device, &mem_alloc, nullptr, &buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreateDataBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindBufferMemory(Vulkan::vulkan->device, buffer.handle, buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout <<"CreateDataBuffer => vkBindBufferMemory : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "CreateDataBuffer : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Change le layout d'un buffer d'image
     */
    bool Vulkan::TransitionImageLayout(IMAGE_BUFFER& buffer,
                                       VkAccessFlags source_mask, VkAccessFlags dest_mask,
                                       VkImageLayout old_layout, VkImageLayout new_layout,
                                       VkPipelineStageFlags source_stage, VkPipelineStageFlags dest_stage)
    {
        // On évite que plusieurs opérations aient lieu en même temps en utilisant une fence
        VkResult result = vkWaitForFences(this->device, 1, &this->transfer_command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "TransitionImageLayout => vkWaitForFences : Timeout" << std::endl;
            #endif
            return false;
        }
        vkResetFences(this->device, 1, &this->transfer_command_buffer.fence);

        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(this->transfer_command_buffer.handle, &command_buffer_begin_info);

        VkImageMemoryBarrier image_memory_barrier;
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = source_mask;
        image_memory_barrier.dstAccessMask = dest_mask;
        image_memory_barrier.oldLayout = old_layout;
        image_memory_barrier.newLayout = new_layout;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = buffer.handle;
        image_memory_barrier.subresourceRange.aspectMask = buffer.aspect;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            this->transfer_command_buffer.handle,
            source_stage, dest_stage,
            0, 0, nullptr, 0, nullptr, 1,
            &image_memory_barrier
        );

        vkEndCommandBuffer(this->transfer_command_buffer.handle);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->transfer_command_buffer.handle;

        result = vkQueueSubmit(this->graphics_queue.handle, 1, &submit_info, this->transfer_command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "TransitionImageLayout => vkQueueSubmit : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "TransitionImageLayout : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Application d'une barrière de transition pour indiquer
     * un changement de queue family lors des accès au buffer.
     * Cela est notament le cas pour les transferts de données qui pasent que la transfer queue à la graphics queue
     */
    /*bool Vulkan::TransitionBufferQueueFamily(
            DATA_BUFFER& buffer, COMMAND_BUFFER command_buffer, DEVICE_QUEUE queue,
            VkAccessFlags source_mask, VkAccessFlags dest_mask,
            uint32_t source_queue_index, uint32_t dest_queue_index,
            VkPipelineStageFlags source_stage, VkPipelineStageFlags dest_stage)
    {
        // On évite que plusieurs exécutions aient lieu en même temps en utilisant une fence
        VkResult result = vkWaitForFences(this->device, 1, &command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "TransitionBufferQueueFamily => vkWaitForFences : Timeout" << std::endl;
            #endif
            return {};
        }
        vkResetFences(this->device, 1, &command_buffer.fence);

        // Initilisation du command buffer
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(command_buffer.handle, &command_buffer_begin_info);

        // Préparation de la barrière de transition
        VkBufferMemoryBarrier buffer_memory_barrier = {};
        buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.pNext = nullptr;
        buffer_memory_barrier.srcAccessMask = source_mask;
        buffer_memory_barrier.dstAccessMask = dest_mask;
        buffer_memory_barrier.srcQueueFamilyIndex = source_queue_index; 
        buffer_memory_barrier.dstQueueFamilyIndex = dest_queue_index;
        buffer_memory_barrier.buffer = buffer.handle;
        buffer_memory_barrier.offset = 0;
        buffer_memory_barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(command_buffer.handle, source_stage, dest_stage, 0, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        vkEndCommandBuffer(command_buffer.handle);

        // Exécution de la commande
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer.handle;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(queue.handle, 1, &submit_info, command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "TransitionBufferQueueFamily => vkQueueSubmit : Failed" << std::endl;
            #endif
            return {};
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "TransitionBufferQueueFamily : Success" << std::endl;
        #endif

        return true;
    }*/

    /**
     * Calcule le segment de mémoire occupé par une donnée en tenant compte du nonCoherentAtomSize
     */
    VkDeviceSize Vulkan::ComputeRawDataAlignment(size_t data_size)
    {
        VkDeviceSize nonCoherentAtomSize = this->physical_device.properties.limits.nonCoherentAtomSize;
        size_t multiple = static_cast<size_t>(data_size / nonCoherentAtomSize);
        if(nonCoherentAtomSize * multiple == data_size) return data_size;
        else return nonCoherentAtomSize * (multiple + 1);
    }

    bool Vulkan::MoveData(DATA_BUFFER& buffer, COMMAND_BUFFER const& command_buffer, VkDeviceSize data_size, VkDeviceSize source_offset, VkDeviceSize destination_offset)
    {
        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        VkResult result = vkWaitForFences(this->device, 1, &command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "MoveData => vkWaitForFences : Timeout" << std::endl;
            #endif
            return 0;
        }
        vkResetFences(this->device, 1, &command_buffer.fence);

        ///////////////////////////////////////////
        //       Envoi des données vers          //
        //   la mémoire de la carte graphique    //
        ///////////////////////////////////////////

        // Préparation de la copie de l'image depuis le staging buffer vers un vertex buffer
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(command_buffer.handle, &command_buffer_begin_info);

        VkBufferCopy buffer_copy_info = {};
        buffer_copy_info.srcOffset = source_offset;
        buffer_copy_info.dstOffset = destination_offset;
        buffer_copy_info.size = data_size;

        vkCmdCopyBuffer(command_buffer.handle, buffer.handle, buffer.handle, 1, &buffer_copy_info);
        vkEndCommandBuffer(command_buffer.handle);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer.handle;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->transfer_queue.handle, 1, &submit_info, command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            switch(result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY : std::cout << "MoveData => vkQueueSubmit : VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY : std::cout << "MoveData => vkQueueSubmit : VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
                case VK_ERROR_DEVICE_LOST : std::cout << "MoveData => vkQueueSubmit : VK_ERROR_DEVICE_LOST" << std::endl;
            }
            #endif
            return false;
        }

        return true;
    }

    /**
     * Envoi de données vers un buffer de la carte graphique en passant par un staging buffer
     * Renvoie la taille du segment de données mis à jour,
     * valeure multiple de nonCoherentAtomSize pour respectier les bonnes pratiques
     */
    size_t Vulkan::SendToBuffer(DATA_BUFFER& buffer, COMMAND_BUFFER const& command_buffer, STAGING_BUFFER staging_buffer, VkDeviceSize data_size, VkDeviceSize destination_offset)
    {
        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        VkResult result = vkWaitForFences(this->device, 1, &command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SendToBuffer[data] => vkWaitForFences : Timeout" << std::endl;
            #endif
            return 0;
        }
        vkResetFences(this->device, 1, &command_buffer.fence);

        ////////////////////////////////
        //   Copie des données vers   //
        //      le staging buffer     //
        ////////////////////////////////

        VkDeviceSize nonCoherentAtomSize = this->physical_device.properties.limits.nonCoherentAtomSize;
        size_t multiple = static_cast<size_t>(data_size / nonCoherentAtomSize);
        VkDeviceSize flush_size;
        if(nonCoherentAtomSize * multiple == data_size) flush_size = data_size;
        else flush_size = nonCoherentAtomSize * (multiple + 1);

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = staging_buffer.memory;
        flush_range.offset = destination_offset;
        flush_range.size = flush_size;
        vkFlushMappedMemoryRanges(this->device, 1, &flush_range);

        ///////////////////////////////////////////
        //       Envoi des données vers          //
        //   la mémoire de la carte graphique    //
        ///////////////////////////////////////////

        // Préparation de la copie de l'image depuis le staging buffer vers un vertex buffer
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(command_buffer.handle, &command_buffer_begin_info);

        VkBufferCopy buffer_copy_info = {};
        buffer_copy_info.srcOffset = destination_offset;
        buffer_copy_info.dstOffset = destination_offset;
        buffer_copy_info.size = data_size;

        vkCmdCopyBuffer(command_buffer.handle, staging_buffer.handle, buffer.handle, 1, &buffer_copy_info);
        vkEndCommandBuffer(command_buffer.handle);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer.handle;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->transfer_queue.handle, 1, &submit_info, command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            switch(result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY : std::cout << "SendToBuffer[data] => vkQueueSubmit : VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY : std::cout << "SendToBuffer[data] => vkQueueSubmit : VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
                case VK_ERROR_DEVICE_LOST : std::cout << "SendToBuffer[data] => vkQueueSubmit : VK_ERROR_DEVICE_LOST" << std::endl;
            }
            #endif
            return 0;
        }

        return flush_size;
    }

    size_t Vulkan::SendToBuffer(DATA_BUFFER& buffer, COMMAND_BUFFER const& command_buffer, STAGING_BUFFER staging_buffer, std::vector<Chunk> chunks)
    {
        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        VkResult result = vkWaitForFences(this->device, 1, &command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SendToBuffer[data] => vkWaitForFences : Timeout" << std::endl;
            #endif
            return 0;
        }
        vkResetFences(this->device, 1, &command_buffer.fence);

        ////////////////////////////////
        //   Copie des données vers   //
        //      le staging buffer     //
        ////////////////////////////////

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = staging_buffer.memory;
        flush_range.offset = 0;
        flush_range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(this->device, 1, &flush_range);

        ///////////////////////////////////////////
        //       Envoi des données vers          //
        //   la mémoire de la carte graphique    //
        ///////////////////////////////////////////

        // Préparation de la copie de l'image depuis le staging buffer vers un vertex buffer
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(command_buffer.handle, &command_buffer_begin_info);

        size_t bytes_sent = 0;
        for(auto& chunk : chunks) {
            VkBufferCopy buffer_copy_info = {};
            buffer_copy_info.srcOffset = chunk.offset;
            buffer_copy_info.dstOffset = chunk.offset;
            buffer_copy_info.size = chunk.range;

            vkCmdCopyBuffer(command_buffer.handle, staging_buffer.handle, buffer.handle, 1, &buffer_copy_info);
            bytes_sent += chunk.range;
        }

        vkEndCommandBuffer(command_buffer.handle);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer.handle;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->transfer_queue.handle, 1, &submit_info, command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            switch(result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY : std::cout << "SendToBuffer[data] => vkQueueSubmit : VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY : std::cout << "SendToBuffer[data] => vkQueueSubmit : VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
                case VK_ERROR_DEVICE_LOST : std::cout << "SendToBuffer[data] => vkQueueSubmit : VK_ERROR_DEVICE_LOST" << std::endl;
            }
            #endif
            return 0;
        }

        return bytes_sent;
    }

    /**
     * Envoi d'une image vers un buffer de la carte graphique en passant par un staging buffer
     */
    // bool Vulkan::SendToBuffer(IMAGE_BUFFER& buffer, const void* data, VkDeviceSize data_size, uint32_t width, uint32_t height)
    size_t Vulkan::SendToBuffer(IMAGE_BUFFER& buffer, Tools::IMAGE_MAP const& image)
    {
        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        VkResult result = vkWaitForFences(this->device, 1, &this->transfer_command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SendToBuffer[image] => vkWaitForFences : Timeout" << std::endl;
            #endif
            return false;
        }
        vkResetFences(this->device, 1, &this->transfer_command_buffer.fence);

        ////////////////////////////////
        //   Copie des données vers   //
        //      le staging buffer     //
        ////////////////////////////////

        std::memcpy(this->transfer_staging.pointer, image.data.data(), image.data.size());

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->transfer_staging.memory;
        flush_range.offset = 0;
        flush_range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(this->device, 1, &flush_range);

        ///////////////////////////////////////////
        //       Envoi des données vers          //
        //   la mémoire de la carte graphique    //
        ///////////////////////////////////////////

        // Préparation de la copie de l'image depuis le staging buffer vers un vertex buffer
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(this->transfer_command_buffer.handle, &command_buffer_begin_info);

        VkImageMemoryBarrier image_memory_barrier_from_undefined_to_transfer_dst = {};
        image_memory_barrier_from_undefined_to_transfer_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier_from_undefined_to_transfer_dst.pNext = nullptr;
        image_memory_barrier_from_undefined_to_transfer_dst.srcAccessMask = 0;
        image_memory_barrier_from_undefined_to_transfer_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier_from_undefined_to_transfer_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier_from_undefined_to_transfer_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier_from_undefined_to_transfer_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier_from_undefined_to_transfer_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier_from_undefined_to_transfer_dst.image = buffer.handle;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.baseMipLevel = 0;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.levelCount = 1;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(this->transfer_command_buffer.handle,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_undefined_to_transfer_dst);

        VkBufferImageCopy buffer_image_copy_info = {};
        buffer_image_copy_info.bufferOffset = 0;
        buffer_image_copy_info.bufferRowLength = 0;
        buffer_image_copy_info.bufferImageHeight = 0;
        buffer_image_copy_info.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_image_copy_info.imageSubresource.mipLevel = 0;
        buffer_image_copy_info.imageSubresource.baseArrayLayer = 0;
        buffer_image_copy_info.imageSubresource.layerCount = 1;
        buffer_image_copy_info.imageOffset.x = 0;
        buffer_image_copy_info.imageOffset.y = 0;
        buffer_image_copy_info.imageOffset.z = 0;
        buffer_image_copy_info.imageExtent.width = image.width;
        buffer_image_copy_info.imageExtent.height = image.height;
        buffer_image_copy_info.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(this->transfer_command_buffer.handle,
            this->transfer_staging.handle, buffer.handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy_info);

        vkEndCommandBuffer(this->transfer_command_buffer.handle);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->transfer_command_buffer.handle;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->transfer_queue.handle, 1, &submit_info, this->transfer_command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SendToBuffer[image] => vkQueueSubmit : Failed" << std::endl;
            #endif
            return false;
        }

        if(!this->TransitionImageLayout(buffer,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SendToBuffer[image] => TransitionImageLayout : Failed" << std::endl;
            #endif
            return 0;
        }

        // Succès
        return true;
    }

    /**
     * Calcule l'alignement correspondant à un Uniform Buffer
     */
    uint32_t Vulkan::ComputeUniformBufferAlignment(uint32_t buffer_size)
    {
        size_t min_ubo_alignement = this->physical_device.properties.limits.minUniformBufferOffsetAlignment;
        if(min_ubo_alignement > 0) buffer_size = static_cast<uint32_t>((buffer_size + min_ubo_alignement - 1) & ~(min_ubo_alignement - 1));
        return buffer_size;
    }

    uint32_t Vulkan::ComputeStorageBufferAlignment(uint32_t buffer_size)
    {
        size_t min_ubo_alignement = this->physical_device.properties.limits.minStorageBufferOffsetAlignment;
        if(min_ubo_alignement > 0) buffer_size = static_cast<uint32_t>((buffer_size + min_ubo_alignement - 1) & ~(min_ubo_alignement - 1));
        return buffer_size;
    }

    /**
    * Lorsque la fenêtre est redimensionnée la swap chain doit être reconstruite
    */
    bool Vulkan::OnWindowSizeChanged()
    {
        if(this->creating_swap_chain) return true;

        // On attend que le GPU soit disponible
        vkDeviceWaitIdle(this->device);

        // On récupère la nouvelle taille de la fenêtre
        this->draw_surface = this->draw_window->GetSurface();

        // Recréation du Depth Buffer
        this->ReleaseDepthBuffer();

        this->depth_buffer = this->CreateImageBuffer(
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                this->draw_surface.width,
                this->draw_surface.height,
                this->depth_format);

        // Recréation de la Swap Chain
        this->CreateSwapChain();

        // Le layout des images de la Swap Chain doit être en mode VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        /*for(uint8_t i=0; i<this->swap_chain.images.size(); i++) {

            IMAGE_BUFFER buffer;
            buffer.handle = this->swap_chain.images[i].handle;
            buffer.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            this->TransitionImageLayout(
                    buffer,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        }*/

        return true;
    }

    /**
     * Acquisition d'une image de la Swap Chain
     */
    bool Vulkan::AcquireNextImage(uint32_t& swapchain_image_index, VkSemaphore semaphore)
    {
        // On attend la find de la génération de l'image avant de la présenter
        /*VkResult result = vkWaitForFences(this->device, 1, &rendering_resource.graphics_command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            #if defined(DISPLAY_LOGS)
            std::cout << "DrawScene => vkWaitForFences : Timeout" << std::endl;
            #endif
            return false;
        }
        vkResetFences(this->device, 1, &rendering_resource.graphics_command_buffer.fence);*/

        VkResult result = vkAcquireNextImageKHR(this->device, this->swap_chain.handle, UINT64_MAX, semaphore, VK_NULL_HANDLE, &swapchain_image_index);
        switch(result) {
            #if defined(DISPLAY_LOGS)
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                std::cout << "DrawScene => vkAcquireNextImageKHR : VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
                return false;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                std::cout << "DrawScene => vkAcquireNextImageKHR : VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
                return false;
            case VK_ERROR_DEVICE_LOST:
                std::cout << "DrawScene => vkAcquireNextImageKHR : VK_ERROR_DEVICE_LOST" << std::endl;
                return false;
            case VK_ERROR_SURFACE_LOST_KHR:
                std::cout << "DrawScene => vkAcquireNextImageKHR : VK_ERROR_SURFACE_LOST_KHR" << std::endl;
                return false;
            case VK_TIMEOUT:
                std::cout << "DrawScene => vkAcquireNextImageKHR : VK_TIMEOUT" << std::endl;
                return false;
            case VK_NOT_READY:
                std::cout << "DrawScene => vkAcquireNextImageKHR : VK_NOT_READY" << std::endl;
                return false;
            case VK_ERROR_VALIDATION_FAILED_EXT:
                std::cout << "DrawScene => vkAcquireNextImageKHR : VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
                return false;
            #endif

            case VK_SUCCESS:
                break;
            case VK_SUBOPTIMAL_KHR:
                break;
            case VK_ERROR_OUT_OF_DATE_KHR:
                this->OnWindowSizeChanged();
            default:
                #if defined(DISPLAY_LOGS)
                std::cout << "DrawScene => vkAcquireNextImageKHR : Failed" << std::endl;
                #endif
                return false;
        }

        return true;
    }

    bool Vulkan::PresentImage(RENDERING_RESOURCES& resources, VkSemaphore semaphore, uint32_t swap_chain_image_index)
    {
        VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.commandBufferCount = static_cast<uint32_t>(resources.comand_buffers.size());
        submit_info.waitSemaphoreCount = 1;
        submit_info.pCommandBuffers = resources.comand_buffers.data();
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &resources.semaphore;
        submit_info.pWaitSemaphores = &semaphore;
        submit_info.pWaitDstStageMask = &wait_dst_stage_mask;

        VkResult result = vkQueueSubmit(this->graphics_queue.handle, 1, &submit_info, resources.fence);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "DrawScene => vkQueueSubmit : Failed" << std::endl;
            #endif
            return false;
        }

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = nullptr;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &resources.semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &this->swap_chain.handle;
        present_info.pImageIndices = &swap_chain_image_index;
        present_info.pResults = nullptr;

        result = vkQueuePresentKHR(this->present_queue.handle, &present_info);

        switch(result)
        {
            case VK_SUCCESS:
                break;
            case VK_ERROR_OUT_OF_DATE_KHR:
            case VK_SUBOPTIMAL_KHR:
                this->OnWindowSizeChanged();
                return false;
            default:
                #if defined(DISPLAY_LOGS)
                std::cout << "DrawScene => vkQueuePresentKHR : Failed" << std::endl;
                #endif
                return false;
        }

        return true;
    }

    std::vector<VkVertexInputAttributeDescription> Vulkan::CreateVertexInputDescription(
                std::vector<std::vector<VERTEX_BINDING_ATTRIBUTE>> attributes,
                std::vector<VkVertexInputBindingDescription>& descriptions)
    {
        std::vector<VkVertexInputAttributeDescription> output;
        uint32_t location = 0;
        descriptions.clear();

        for(int32_t i=0; i<attributes.size(); i++) {

            uint32_t offset = 0;
            for(auto type : attributes[i]) {

                VkVertexInputAttributeDescription attribute;
                attribute.offset = offset;
                attribute.location = location;
                attribute.binding = i;
                location++;

                switch(type) {

                    case VERTEX_BINDING_ATTRIBUTE::POSITION :
                    case VERTEX_BINDING_ATTRIBUTE::NORMAL :
                    case VERTEX_BINDING_ATTRIBUTE::COLOR :
                        attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
                        offset += sizeof(Maths::Vector3);
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::POSITION_2D :
                    case VERTEX_BINDING_ATTRIBUTE::UV :
                        attribute.format = VK_FORMAT_R32G32_SFLOAT;
                        offset += sizeof(Maths::Vector2);
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::BONE_WEIGHTS :
                        attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        offset += sizeof(Maths::Vector4);
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::BONE_IDS :
                        attribute.format = VK_FORMAT_R32G32B32A32_SINT;
                        offset += sizeof(int) * 4;
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::MATRIX :
                        attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        offset += sizeof(Maths::Matrix4x4);
                        location += 3;
                        break;

                    case VERTEX_BINDING_ATTRIBUTE::UINT_ID :
                        attribute.format = VK_FORMAT_R32_UINT;
                        offset += sizeof(uint32_t);
                        break;

                    default :
                        attribute.format = VK_FORMAT_UNDEFINED;
                }

                output.push_back(attribute);
            }

            VkVertexInputBindingDescription description;
            description.binding = i;
            description.stride = offset;
            if(i == 0) description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            else description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
            descriptions.push_back(description);
        }

        return output;
    }
   

    /**
     * Création d'une pipeline
     */
    bool Vulkan::CreatePipeline(bool dynamic_viewport,
                                std::vector<VkDescriptorSetLayout> const& descriptor_set_layouts,
                                std::vector<VkPipelineShaderStageCreateInfo> const& shader_stages,
                                std::vector<VkVertexInputBindingDescription> const& vertex_binding_description,
                                std::vector<VkVertexInputAttributeDescription> const& vertex_attribute_descriptions,
                                std::vector<VkPushConstantRange> const& push_constant_rages,
                                PIPELINE& pipeline,
                                VkPolygonMode polygon_mode,
                                VkPrimitiveTopology topology,
                                bool blend_state)
    {
        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(push_constant_rages.size());
        pPipelineLayoutCreateInfo.pPushConstantRanges = push_constant_rages.data();
        pPipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
        pPipelineLayoutCreateInfo.pSetLayouts = descriptor_set_layouts.data();

        VkResult result = vkCreatePipelineLayout(this->device, &pPipelineLayoutCreateInfo, nullptr, &pipeline.layout);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreatePipeline => vkCreatePipelineLayout : Failed" << std::endl;
            #endif
            return false;
        }
        
        VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
        viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.pNext = nullptr;
        viewport_state_create_info.flags = 0;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = nullptr;
        viewport_state_create_info.pViewports = nullptr;

        VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState = {};
        VkViewport viewport = {};
        VkRect2D scissor = {};

        if(dynamic_viewport) {
            
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.pNext = nullptr;
            dynamicState.dynamicStateCount = 2;
            dynamicState.pDynamicStates = dynamic_states;
            
        }else{

            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(Vulkan::GetDrawSurface().width);
            viewport.height = static_cast<float>(Vulkan::GetDrawSurface().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            scissor.offset.x = 0;
            scissor.offset.y = 0;
            scissor.extent.width = Vulkan::GetDrawSurface().width;
            scissor.extent.height = Vulkan::GetDrawSurface().height;

            viewport_state_create_info.pScissors = &scissor;
            viewport_state_create_info.pViewports = &viewport;
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state;
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state.pNext = nullptr;
        vertex_input_state.flags = 0;
        vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_description.size());
        vertex_input_state.pVertexBindingDescriptions = vertex_binding_description.data();
        vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size());
        vertex_input_state.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.pNext = nullptr;
        input_assembly_state.flags = 0;
        input_assembly_state.primitiveRestartEnable = VK_FALSE;
        input_assembly_state.topology = topology;

        if(polygon_mode == VK_POLYGON_MODE_LINE) input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterization_state;
        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.pNext = nullptr;
        rasterization_state.flags = 0;
        rasterization_state.polygonMode = polygon_mode;
        rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state.depthClampEnable = VK_FALSE;
        rasterization_state.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state.depthBiasEnable = VK_FALSE;
        rasterization_state.depthBiasConstantFactor = 0.0f;
        rasterization_state.depthBiasClamp = 0.0f;
        rasterization_state.depthBiasSlopeFactor = 0.0f;
        rasterization_state.lineWidth = 1.0f;

        if(polygon_mode == VK_POLYGON_MODE_LINE) {
            rasterization_state.cullMode = VK_CULL_MODE_NONE;
            rasterization_state.lineWidth = 2.0f;
        }

        VkPipelineColorBlendAttachmentState blend_attachment;
        blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        if(blend_state) {
            blend_attachment.blendEnable = VK_TRUE;
		    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        }else{
            blend_attachment.blendEnable = VK_FALSE;
		    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        }

        VkPipelineColorBlendStateCreateInfo color_blend_state;
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.pNext = nullptr;
        color_blend_state.flags = 0;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &blend_attachment;
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.logicOp = VK_LOGIC_OP_COPY;
        color_blend_state.blendConstants[0] = 0.0f;
        color_blend_state.blendConstants[1] = 0.0f;
        color_blend_state.blendConstants[2] = 0.0f;
        color_blend_state.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = nullptr;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.minDepthBounds = 0.0f;
        depth_stencil_state.maxDepthBounds = 1.0f;
        depth_stencil_state.stencilTestEnable = VK_FALSE;
        depth_stencil_state.back.failOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.passOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depth_stencil_state.back.compareMask = 0;
        depth_stencil_state.back.reference = 0;
        depth_stencil_state.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.writeMask = 0;
        depth_stencil_state.front = depth_stencil_state.back;

        VkPipelineMultisampleStateCreateInfo multi_samples_state;
        multi_samples_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multi_samples_state.pNext = nullptr;
        multi_samples_state.flags = 0;
        multi_samples_state.pSampleMask = nullptr;
        multi_samples_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multi_samples_state.sampleShadingEnable = VK_FALSE;
        multi_samples_state.alphaToCoverageEnable = VK_FALSE;
        multi_samples_state.alphaToOneEnable = VK_FALSE;
        multi_samples_state.minSampleShading = 1.0f;

        VkGraphicsPipelineCreateInfo pipeline_create_info;
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = nullptr;
        pipeline_create_info.layout = pipeline.layout;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;
        pipeline_create_info.flags = 0;
        pipeline_create_info.pVertexInputState = &vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state;
        pipeline_create_info.pRasterizationState = &rasterization_state;
        pipeline_create_info.pColorBlendState = &color_blend_state;
        pipeline_create_info.pTessellationState = nullptr;
        pipeline_create_info.pMultisampleState = &multi_samples_state;
        pipeline_create_info.pDynamicState = (dynamic_viewport) ? &dynamicState : nullptr;
        pipeline_create_info.pViewportState = &viewport_state_create_info;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pStages = shader_stages.data();
        pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipeline_create_info.renderPass = Vulkan::GetRenderPass();
        pipeline_create_info.subpass = 0;

        result = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline.handle);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "CreatePipeline => vkCreateGraphicsPipelines : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "CreatePipeline : Success" << std::endl;
        #endif
        return true;
    }
}