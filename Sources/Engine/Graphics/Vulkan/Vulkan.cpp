#include "Vulkan.h"

namespace Engine
{
    #define VK_EXPORTED_FUNCTION( fun ) PFN_##fun fun;
    #define VK_GLOBAL_LEVEL_FUNCTION( fun) PFN_##fun fun;
    #define VK_INSTANCE_LEVEL_FUNCTION( fun ) PFN_##fun fun;
    #define VK_DEVICE_LEVEL_FUNCTION( fun ) PFN_##fun fun;
    #include "ListOfFunctions.inl"

    // Initialisation du singleton
    Vulkan* Vulkan::global_instance = nullptr;

    /**
     * Permet d'accéder à l'instance du singleton
     */
    Vulkan* Vulkan::GetInstance()
    {
        if(Vulkan::global_instance == nullptr) Vulkan::global_instance = new Vulkan();
        return Vulkan::global_instance;
    }

    /**
     * Constructeur
     */
    Vulkan::Vulkan()
    {
        this->library                   = nullptr;
        this->instance                  = VK_NULL_HANDLE;
        this->presentation_surface      = VK_NULL_HANDLE;
        this->present_stack_index       = UINT32_MAX;
        this->graphics_stack_index      = UINT32_MAX;
        this->transfer_stack_index      = UINT32_MAX;
        this->device                    = VK_NULL_HANDLE;
        this->creating_swap_chain       = false;
        this->render_pass               = VK_NULL_HANDLE;
        this->graphics_pool             = VK_NULL_HANDLE;
        this->last_texture_index        = 0;
        this->last_vbo_index            = 0;
        this->last_mesh_index           = 0;
        this->keep_runing               = false;
        this->ubo_alignement            = 0;

        #if defined(_DEBUG)
        std::cout << "Nombre de threads disponibles : " << std::thread::hardware_concurrency() << std::endl;
        #endif
    }

    /**
     * Destructeur
     */
    Vulkan::~Vulkan()
    {
        // Fences
        for(auto resource : this->rendering)
            if(resource.fence != VK_NULL_HANDLE)
                vkDestroyFence(this->device, resource.fence, nullptr);
        if(this->transfer.fence != VK_NULL_HANDLE) vkDestroyFence(this->device, this->transfer.fence, nullptr);

        // Semaphores
        for(auto resource : this->rendering) {
            if(resource.render_pass_semaphore != VK_NULL_HANDLE)
                vkDestroySemaphore(this->device, resource.render_pass_semaphore, nullptr);
            if(resource.swap_chain_semaphore != VK_NULL_HANDLE)
                vkDestroySemaphore(this->device, resource.swap_chain_semaphore, nullptr);
        }

        // Graphics Command Buffers
        for(auto resource : this->rendering)
            if(resource.command_buffer != VK_NULL_HANDLE)
                vkFreeCommandBuffers(this->device, this->graphics_pool, 1, &resource.command_buffer);
        if(this->graphics_pool != VK_NULL_HANDLE) vkDestroyCommandPool(this->device, this->graphics_pool, nullptr);

        // Transfer Command Buffers
        if(this->transfer.command_buffer != VK_NULL_HANDLE) vkFreeCommandBuffers(this->device, this->transfer.command_pool, 1, &this->transfer.command_buffer);
        if(this->transfer.command_pool != VK_NULL_HANDLE && this->transfer.command_pool != this->graphics_pool) vkDestroyCommandPool(this->device, this->transfer.command_pool, nullptr);

        // Pipeline
        if(this->graphics_pipeline.handle != VK_NULL_HANDLE) vkDestroyPipeline(this->device, this->graphics_pipeline.handle, nullptr);
        if(this->graphics_pipeline.layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(this->device, this->graphics_pipeline.layout, nullptr);

        // Descriptor Sets (Base)
        if(this->descriptor_pool.pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(this->device, this->descriptor_pool.pool, nullptr);
        if(this->descriptor_pool.layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(this->device, this->descriptor_pool.layout, nullptr);

        // Render Pass
        if(this->render_pass != VK_NULL_HANDLE) vkDestroyRenderPass(this->device, this->render_pass, nullptr);

        // Uniform Buffer
        if(this->uniform_buffer.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->uniform_buffer.memory, nullptr);
        if(this->uniform_buffer.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, this->uniform_buffer.handle, nullptr);

        // Vertex Buffers
        for(std::pair<uint32_t, VULKAN_BUFFER> element : this->vertex_buffers) {
            if(element.second.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, element.second.memory, nullptr);
            if(element.second.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, element.second.handle, nullptr);
        }
        
        // Textures
        for(std::pair<uint32_t, TEXTURE> element : this->textures) {
            if(element.second.view != VK_NULL_HANDLE) vkDestroyImageView(this->device, element.second.view, nullptr);
            if(element.second.image != VK_NULL_HANDLE) vkDestroyImage(this->device, element.second.image, nullptr);
            if(element.second.sampler != VK_NULL_HANDLE) vkDestroySampler(this->device, element.second.sampler, nullptr);
            if(element.second.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, element.second.memory, nullptr);
        }

        // Stagin Buffer
        if(this->staging_buffer.pointer != nullptr) vkUnmapMemory(this->device, this->staging_buffer.memory);
        if(this->staging_buffer.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->staging_buffer.memory, nullptr);
        if(this->staging_buffer.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, this->staging_buffer.handle, nullptr);

        // Depth buffer
        if(this->depth_buffer.view != VK_NULL_HANDLE) vkDestroyImageView(this->device, this->depth_buffer.view, nullptr);
        if(this->depth_buffer.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->depth_buffer.memory, nullptr);
        if(this->depth_buffer.image != VK_NULL_HANDLE) vkDestroyImage(this->device, this->depth_buffer.image, nullptr);

        // Image Views de la Swap Chain
        for(int i=0; i<this->swap_chain.images.size(); i++)
            if(this->swap_chain.images[i].view != VK_NULL_HANDLE)
                vkDestroyImageView(this->device, this->swap_chain.images[i].view, nullptr);
        this->swap_chain.images.clear();

        // Swap Chain
        if(this->swap_chain.handle != VK_NULL_HANDLE) vkDestroySwapchainKHR(this->device, this->swap_chain.handle, nullptr);

        // Device
        if(this->device != VK_NULL_HANDLE && vkDestroyDevice) vkDestroyDevice(this->device, nullptr);

        // Surface
        if(this->presentation_surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(this->instance, this->presentation_surface, nullptr);

        // Validation Layers
        #if defined(_DEBUG)
        if(this->report_callback != VK_NULL_HANDLE) vkDestroyDebugUtilsMessengerEXT(this->instance, this->report_callback, nullptr);
        #endif

        // Instance
        vkDestroyInstance(this->instance, nullptr);

        // Librairie
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        if(this->library != nullptr) FreeLibrary(this->library);
        #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        if(this->library != nullptr) dlclose(this->library);
        #endif
    }

    /**
     * Libération des resources
     */
    void Vulkan::DestroyInstance()
    {
        if(Vulkan::global_instance == nullptr) return;
        Vulkan::global_instance->Stop();
        delete Vulkan::global_instance;
        Vulkan::global_instance = nullptr;
    }

    /**
     * Initlisation de vulkan
     */
    Vulkan::ERROR_MESSAGE Vulkan::Initialize(Engine::Window* draw_window, uint32_t application_version, std::string aplication_name)
    {
        //////////////////////
        ///   PREPARATION  ///
        //////////////////////

        // On récupère la fenêtre et la surface d'affichage
        this->draw_window = draw_window;
        this->draw_surface = draw_window->GetSurface();

        // Valeur de retourn de la fonction
        ERROR_MESSAGE init_status = ERROR_MESSAGE::SUCCESS;

        #if defined(_DEBUG)
        std::string vulkan_version;
        this->GetVersion(vulkan_version);
        std::cout << "Version de vulkan : " << vulkan_version << std::endl;
        #endif

        // Chargement de la librairie vulkan
        if(!this->LoadPlatformLibrary())
            init_status = ERROR_MESSAGE::LOAD_LIBRARY_FAILURE;
         
        ////////////////////
        // INITIALISATION //
        ////////////////////

        // Chargement de l'exporteur de fonctions vulkan
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->LoadExportedEntryPoints())
            init_status = ERROR_MESSAGE::LOAD_EXPORTED_FUNCTIONS_FAILURE;

        // Chargement des fonctions vulkan globales
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->LoadGlobalLevelEntryPoints())
            init_status = ERROR_MESSAGE::LOAD_GLOBAL_FUNCTIONS_FAILURE;

        // Création de l'instance vulkan
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateVulkanInstance(application_version, aplication_name))
            init_status = ERROR_MESSAGE::VULKAN_INSTANCE_CREATION_FAILURE;

        // Chargement des fonctions portant sur l'instance
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->LoadInstanceLevelEntryPoints())
            init_status = ERROR_MESSAGE::LOAD_INSTANCE_FUNCTIONS_FAILURE;

        #if defined(_DEBUG)
        this->report_callback = VK_NULL_HANDLE;
        this->CreateDebugReportCallback();
        #endif

        // Création de la surface de présentation
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreatePresentationSurface())
            init_status = ERROR_MESSAGE::DEVICE_CREATION_FAILURE;

        // Création du logical device
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateDevice())
            init_status = ERROR_MESSAGE::DEVICE_CREATION_FAILURE;

        // Chargement des fonctions portant sur le logical device
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->LoadDeviceLevelEntryPoints())
            init_status = ERROR_MESSAGE::LOAD_DEVICE_FUNCTIONS_FAILURE;

        // Récupère le handle des device queues
        this->GetDeviceQueues();

        // Création de la swap chain
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateSwapChain())
            init_status = ERROR_MESSAGE::SWAP_CHAIN_CREATION_FAILURE;

        // Allocation des threads utilisés pour le rendu
        Vulkan::global_instance->rendering.resize(this->swap_chain.images.size());

        // Création du depth buffer
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateDepthBuffer())
            init_status = ERROR_MESSAGE::DEPTH_BUFFER_CREATION_FAILURE;

        // Création du staging buffer
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateStagingBuffer(STAGING_BUFFER_SIZE))
            init_status = ERROR_MESSAGE::STAGING_BUFFER_CREATION_FAILURE;

        // Création du uniform buffer
        uint64_t uniform_buffer_size = 1024 * 1024 * 20; // 20 Mo
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateUniformBuffer())
            init_status = ERROR_MESSAGE::UNIFORM_BUFFER_CREATION_FAILURE;

        // Création de la render pass
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateRenderPass())
            init_status = ERROR_MESSAGE::RENDER_PASS_CREATION_FAILURE;

        // Création des descriptor sets
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateDescriptorSets())
            init_status = ERROR_MESSAGE::DESCRIPTOR_SETS_CREATION_FAILURE;

        // Création du pipeline graphique
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreatePipeline())
            init_status = ERROR_MESSAGE::GRAPHICS_PIPELINE_CREATION_FAILURE;

        // Création des command buffers
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateCommandBuffers())
            init_status = ERROR_MESSAGE::COMMAND_BUFFERS_CREATION_FAILURE;

        // Création des sémaphores
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateSemaphores())
            init_status = ERROR_MESSAGE::SEMAPHORES_CREATION_FAILURE;

        // Création des fences
        if(init_status == ERROR_MESSAGE::SUCCESS && !this->CreateFences())
            init_status = ERROR_MESSAGE::FENCES_CREATION_FAILURE;

        // Mise à jour du uniform buffer
        /*if(init_status == ERROR_MESSAGE::SUCCESS && !this->UpdateMeshUniformBuffers())
            init_status = ERROR_MESSAGE::UNIFORM_BUFFER_UPDATE_FAILURE;*/

        // En cas d'échecx d'initialisation, on libère les ressources alouées
        if(init_status != ERROR_MESSAGE::SUCCESS) Vulkan::DestroyInstance();

        // Renvoi du résultat
        return init_status;
    }

    /**
     * Chargement de la librairie vulkan
     */
    bool Vulkan::LoadPlatformLibrary()
    {
        // Chargement de la DLL pour Windows
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        this->library = LoadLibrary("vulkan-1.dll");

        // Chargement de la librairie partagée pour Linux / MACOS
        #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        this->Library = dlopen("libvulkan.so.1", RTLD_NOW);

        #endif

        #if defined(_DEBUG)
        std::cout << "LoadPlatformLibrary : " << ((this->library != nullptr) ? "Success" : "Failure") << std::endl;
        #endif

        // Succès si library n'est pas nul
        return this->library != nullptr;
    }

    bool Vulkan::GetVersion(std::string &version_string)
    {
        bool success = false;

        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        DWORD ver_handle;
        DWORD ver_size = GetFileVersionInfoSize("vulkan-1.dll", &ver_handle);
        if (ver_size > 0) {
            std::vector<char> ver_data(ver_size);
            if (GetFileVersionInfo("vulkan-1.dll", ver_handle, ver_size, ver_data.data())) {
                UINT size = 0;
                LPBYTE buffer = NULL;
                if (VerQueryValue(ver_data.data(), "\\", (VOID FAR * FAR *)&buffer, &size)) {
                    if (size) {
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
     * Affichage des erreurs lorsque les validation layers sont activées
     */
    #if defined(_DEBUG)
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
        createInfo.pUserData = nullptr;

        VkResult result = vkCreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->report_callback);
        createInfo = {};
    }
    #endif

    /**
     * Création de l'instance Vulkan
     */
    bool Vulkan::CreateVulkanInstance(uint32_t application_version, std::string& aplication_name)
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

        #if defined(_DEBUG)
        std::vector<const char*> instance_layer_names = {"VK_LAYER_LUNARG_standard_validation"};
        instance_extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif

        // Initialisation de la structure VkInstanceCreateInfo
        VkInstanceCreateInfo inst_info;
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pNext = nullptr;
        inst_info.flags = 0;
        inst_info.pApplicationInfo = &app_info;
        inst_info.enabledExtensionCount = (uint32_t)instance_extension_names.size();
        inst_info.ppEnabledExtensionNames = &instance_extension_names[0];
        inst_info.enabledLayerCount = 0;
        inst_info.ppEnabledLayerNames = nullptr;

        #if defined(_DEBUG)
        inst_info.enabledLayerCount = 1;
        inst_info.ppEnabledLayerNames = &instance_layer_names[0];
        #endif

        // Création de l'instance Vulkan
        VkResult result = vkCreateInstance(&inst_info, nullptr, &this->instance);

        #if defined(_DEBUG)
        std::cout << "CreateVulkanInstance : " << ((result == VK_SUCCESS) ? "Success" : "Failure") << std::endl;
        #endif

        // Succès
        return result == VK_SUCCESS;
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

        #if defined(_DEBUG)
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

        #if defined(_DEBUG)
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
        #if defined(_DEBUG)
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

        #if defined(_DEBUG)
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
        #if defined(_DEBUG)
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

        #if defined(_DEBUG)
        std::cout << "LoadInstanceLevelEntryPoints : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Chargement des fonctions Vulkan liées au GPU logique
     */
    bool Vulkan::LoadDeviceLevelEntryPoints()
    {
        #if defined(_DEBUG)
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

        #if defined(_DEBUG)
        std::cout << "LoadDeviceLevelEntryPoints : Success" << std::endl;
        #endif

        return true;
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

        #if defined(_DEBUG)
        std::cout << "CreatePresentationSurface : " << ((result == VK_SUCCESS) ? "Success" : "Failure") << std::endl;
        #endif

        return result == VK_SUCCESS;
    }

    /**
     * Création du logical device Vulkan
     */
    bool Vulkan::CreateDevice(char preferred_device_index)
    {
        // En premier lieu on énumère les différents périphériques compatibles avec Vulkan afin d'en trouver un qui soit compatible avec notre application

        // Récupération du nombre de périphériques
        uint32_t physical_device_count = 1;
        VkResult result = vkEnumeratePhysicalDevices(this->instance, &physical_device_count, nullptr);
        if(result != VK_SUCCESS || physical_device_count == 0) {
            #if defined(_DEBUG)
            std::cout << "CreateDevice => vkEnumeratePhysicalDevices 1/2 : Failed" << std::endl;
            #endif
            return false;
        }

        // Liste des périphériques
        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        result = vkEnumeratePhysicalDevices(this->instance, &physical_device_count, &physical_devices[0]);
        if(result != VK_SUCCESS || physical_device_count == 0) {
            #if defined(_DEBUG)
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
                #if defined(_DEBUG)
                std::cout << "CreateDevice : Selected physical device : " << preferred_device_index << std::endl;
                #endif
            }
        }else{
            for(uint32_t i=0; i<physical_devices.size(); i++) {
                queue_family_properties = this->QueryDeviceProperties(physical_devices[i]);
                if(this->IsDeviceEligible(physical_devices[0], queue_family_properties)) {
                    this->physical_device.handle = physical_devices[0];
                    #if defined(_DEBUG)
                    std::cout << "CreateDevice : Selected physical device : " << i << std::endl;
                    #endif
                    break;
                }
            }
        }

        if(this->physical_device.handle == VK_NULL_HANDLE) {
            #if defined(_DEBUG)
            std::cout << "CreateDevice : No eligible device found" << std::endl;
            #endif
            return false;
        }

        // Sélection de la present queue
        uint32_t present_index = this->SelectPresentQueue(this->physical_device.handle, queue_family_properties);

        // Sélection des autres queues
        uint32_t graphics_index = this->SelectPreferredQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_GRAPHICS_BIT, present_index);
        uint32_t transfer_index = this->SelectPreferredQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_TRANSFER_BIT, present_index);
       
        #if defined(_DEBUG)
        std::cout << "CreateDevice : Graphics queue : " << graphics_index << std::endl;
        std::cout << "CreateDevice : Transfert queue : " << transfer_index << std::endl;
        std::cout << "CreateDevice : Present queue : " << present_index << std::endl;
        #endif

        // Préparation de la création des queues
        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        std::vector<uint32_t> queues = {present_index, graphics_index, transfer_index};
        std::vector<float> queue_priorities = {0.0f};
        for(int i=0; i<queues.size(); i++) {

            bool found = false;
            for(int j=0; j<this->queue_stack.size(); j++) if(queue_stack[j].index == queues[i]) {
                found = true;
                break;
            }
            if(found) continue;

            DEVICE_QUEUE queue = {};
            queue.index = queues[i];
            this->queue_stack.push_back(queue);

            if(present_index == queue.index) this->present_stack_index = (uint32_t)this->queue_stack.size() - 1;
            if(graphics_index == queue.index) this->graphics_stack_index = (uint32_t)this->queue_stack.size() - 1;
            if(transfer_index == queue.index) this->transfer_stack_index = (uint32_t)this->queue_stack.size() - 1;

            VkDeviceQueueCreateInfo queue_info = {};
            queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info.pNext = nullptr;
            queue_info.queueCount = (uint32_t)queue_priorities.size();
            queue_info.pQueuePriorities = &queue_priorities[0];
            queue_info.queueFamilyIndex = queue.index;
            queue_infos.push_back(queue_info);
        }

        // Activation de l'extension SwapChain
        std::vector<const char*> device_extension_name = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = nullptr;
        device_info.queueCreateInfoCount = (uint32_t)queue_infos.size();
        device_info.pQueueCreateInfos = &queue_infos[0];
        device_info.enabledExtensionCount = (uint32_t)device_extension_name.size();
        device_info.ppEnabledExtensionNames = &device_extension_name[0];
        device_info.enabledLayerCount = 0;
        device_info.ppEnabledLayerNames = nullptr;
        device_info.pEnabledFeatures = nullptr;

        result = vkCreateDevice(this->physical_device.handle, &device_info, nullptr, &this->device);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDevice => vkCreateDevice : Failed" << std::endl;
            #endif
            return false;
        }

        // On récupère les propriétés memoire
        vkGetPhysicalDeviceMemoryProperties(this->physical_device.handle, &this->physical_device.memory);
        vkGetPhysicalDeviceProperties(this->physical_device.handle, &this->physical_device.properties);

        // On calcule l'alignement des offets des UBO
        size_t min_ubo_alignement = this->physical_device.properties.limits.minUniformBufferOffsetAlignment;
        this->ubo_alignement = sizeof(Matrix4x4);
        if(min_ubo_alignement > 0) this->ubo_alignement = static_cast<uint32_t>((this->ubo_alignement + min_ubo_alignement - 1) & ~(min_ubo_alignement - 1));

        // Succès
        #if defined(_DEBUG)
        std::cout << "CreateDevice : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Renvoie la liste des queue family properties d'un périphérique donné
     */
    std::vector<VkQueueFamilyProperties> Vulkan::QueryDeviceProperties(VkPhysicalDevice test_physical_device)
    {
        // On récupère les propriétés des queue families de la carte
        uint32_t queue_family_count = UINT32_MAX;
        vkGetPhysicalDeviceQueueFamilyProperties(test_physical_device, &queue_family_count, nullptr);
        if(queue_family_count == 0) {
            #if defined(_DEBUG)
            std::cout << "QueryDeviceProperties => vkGetPhysicalDeviceQueueFamilyProperties 1/2 : Failed" << std::endl;
            #endif
            return std::vector<VkQueueFamilyProperties>();
        }

        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(test_physical_device, &queue_family_count, &queue_family_properties[0]);
        if(queue_family_count == 0) {
            #if defined(_DEBUG)
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
        for(uint32_t i = 0; i < queue_family_properties.size(); i++) if(queue_family_properties[i].queueFlags == queue_type) return i;

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
        for(int i=0; i<this->queue_stack.size(); i++) {
            VkQueue handle = VK_NULL_HANDLE;
            vkGetDeviceQueue(this->device, this->queue_stack[i].index, 0, &handle);
            this->queue_stack[i].handle = handle;
        }
        #if defined(_DEBUG)
        std::cout << "GetDeviceQueues : Success" << std::endl;
        #endif
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
            #if defined(_DEBUG)
            std::cout << "CreateSwapChain => vkGetPhysicalDeviceSurfaceCapabilitiesKHR : Failed" << std::endl;
            #endif
            this->creating_swap_chain = false;
            return false;
        }

        //On apelle tous nos helpers pour récupérer toutes les informations qui constitueront la SwapChain
        VkExtent2D swap_chain_extent = this->GetSwapChainExtent(surface_capabilities);
        uint32_t swap_chain_num_images = this->GetSwapChainNumImages(surface_capabilities);
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
        swapchain_ci.minImageCount = swap_chain_num_images;
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
            this->queue_stack[this->graphics_stack_index].index,
            this->queue_stack[this->present_stack_index].index
        };

        if(this->graphics_stack_index != this->present_stack_index) {
            // Si les queues family graphique et présentation sont distinctes,
            // on utilise le mode VK_SHARING_MODE_CONCURRENT pour éviter d'avoir
            // à spécifier explicitement à quelle queue est attribuée quelle image
            swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchain_ci.queueFamilyIndexCount = 2;
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
            #if defined(_DEBUG)
            std::cout << "CreateSwapChain => vkCreateSwapchainKHR : Failed" << std::endl;
            #endif
            this->creating_swap_chain = false;
            return false;
        }

        // Création des images de la SwapChain
        uint32_t swap_chain_image_count;
        result = vkGetSwapchainImagesKHR(this->device, this->swap_chain.handle, &swap_chain_image_count, nullptr);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateSwapChain => vkGetSwapchainImagesKHR 1/2 : Failed" << std::endl;
            #endif
            this->creating_swap_chain = false;
            return false;
        }

        // Récupération des images
        std::vector<VkImage> swap_chain_image(swap_chain_image_count);
        result = vkGetSwapchainImagesKHR(this->device, this->swap_chain.handle, &swap_chain_image_count, &swap_chain_image[0]);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateSwapChain => vkGetSwapchainImagesKHR 2/2 : Failed" << std::endl;
            #endif
            this->creating_swap_chain = false;
            return false;
        }

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
            color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            color_image_view.subresourceRange.baseMipLevel = 0;
            color_image_view.subresourceRange.levelCount = 1;
            color_image_view.subresourceRange.baseArrayLayer = 0;
            color_image_view.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(this->device, &color_image_view, nullptr, &this->swap_chain.images[i].view);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateSwapChain => vkCreateImageView[" << i << "] : Failed" << std::endl;
                #endif
                this->creating_swap_chain = false;
                return false;
            }
        }

        #if defined(_DEBUG)
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
            #if defined(_DEBUG)
            std::cout << "GetSurfaceFormat => vkGetPhysicalDeviceSurfaceFormatsKHR 1/2 : Failed" << std::endl;
            #endif
            return VK_FORMAT_UNDEFINED;
        }

        std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(this->physical_device.handle, this->presentation_surface, &formatCount, &surfFormats[0]);
        if(res != VK_SUCCESS || formatCount == 0) {
            #if defined(_DEBUG)
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
     */
    VkPresentModeKHR Vulkan::GetSwapChainPresentMode()
    {
        // Le mode de présentation FIFO est guaranti de fonctionner selon les sepcs Vulkan
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

        // Mais dans notre cas on voudrait utiliser le mode MAILBOX
        // Donc on établit la liste des modes compatibles
        uint32_t present_modes_count;
        VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(this->physical_device.handle, this->presentation_surface, &present_modes_count, nullptr);
        if(res != VK_SUCCESS || present_modes_count == 0) {
            #if defined(_DEBUG)
            std::cout << "GetSwapCHainPresentMode => vkGetPhysicalDeviceSurfacePresentModesKHR 1/2 : Failed" << std::endl;
            #endif
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        std::vector<VkPresentModeKHR> present_modes(present_modes_count);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(this->physical_device.handle, this->presentation_surface, &present_modes_count, &present_modes[0]);
        if(res != VK_SUCCESS || present_modes_count == 0) {
            #if defined(_DEBUG)
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
     * Création du depth buffer
     */
    bool Vulkan::CreateDepthBuffer()
    {
        // En cas de redimensionnement de la fenêtre, il sera nécessaire de reconstruire le depth buffer,
        // dans ce cas on va supprimer l'ancien depth buffer
        vkDeviceWaitIdle(this->device);
        if(this->depth_buffer.view != VK_NULL_HANDLE) vkDestroyImageView(this->device, this->depth_buffer.view, nullptr);
        if(this->depth_buffer.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->depth_buffer.memory, nullptr);
        if(this->depth_buffer.image != VK_NULL_HANDLE) vkDestroyImage(this->device, this->depth_buffer.image, nullptr);

        /////////////////////////
        // Création de l'image //
        /////////////////////////

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = nullptr;
        image_info.flags = 0;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = VK_FORMAT_D16_UNORM;
        image_info.extent.width = this->draw_surface.width;
        image_info.extent.height = this->draw_surface.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = nullptr;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VkResult result = vkCreateImage(this->device, &image_info, nullptr, &this->depth_buffer.image);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkCreateImage : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////////////////////////
        //      Réservation d'un segment de mémoire          //
        // dans la carte graphique pour y placer l'image     //
        ///////////////////////////////////////////////////////

        VkMemoryRequirements image_memory_requirements;
        vkGetImageMemoryRequirements(this->device, this->depth_buffer.image, &image_memory_requirements);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = image_memory_requirements.size;
        
        if(!this->MemoryTypeFromProperties(image_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_alloc.memoryTypeIndex)) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => MemoryType VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : Not found" << std::endl;
            #endif
            return false;
        }

        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &this->depth_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindImageMemory(this->device, this->depth_buffer.image, this->depth_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkBindImageMemory : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////////////
        // Création de l'image view //
        //////////////////////////////

        VkImageViewCreateInfo depth_image_view = {};
        depth_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depth_image_view.pNext = nullptr;
        depth_image_view.flags = 0;
        depth_image_view.image = this->depth_buffer.image;
        depth_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depth_image_view.format = VK_FORMAT_D16_UNORM;
        depth_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_image_view.subresourceRange.baseMipLevel = 0;
        depth_image_view.subresourceRange.levelCount = 1;
        depth_image_view.subresourceRange.baseArrayLayer = 0;
        depth_image_view.subresourceRange.layerCount = 1;

        result = vkCreateImageView(this->device, &depth_image_view, nullptr, &this->depth_buffer.view);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkCreateImageView : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(_DEBUG)
        std::cout << "CreateDepthBuffer : Success" << std::endl;
        #endif

        return true;
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
     * Création du staging buffer
     */
    bool Vulkan::CreateStagingBuffer(VkDeviceSize size)
    {
        std::vector<uint32_t> shared_queue_families = {
            this->queue_stack[this->transfer_stack_index].index,
            this->queue_stack[this->graphics_stack_index].index
        };

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = size;
        buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        buffer_create_info.queueFamilyIndexCount = 2;
        buffer_create_info.pQueueFamilyIndices = &shared_queue_families[0];

        VkResult result = vkCreateBuffer(this->device, &buffer_create_info, nullptr, &this->staging_buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateStaginBuffer => vkCreateBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(this->device, this->staging_buffer.handle, &mem_reqs);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;
        mem_alloc.allocationSize = mem_reqs.size;
        
        if(!this->MemoryTypeFromProperties(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, mem_alloc.memoryTypeIndex)) {
            #if defined(_DEBUG)
            std::cout << "CreateStaginBuffer => MemoryType VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : Not found" << std::endl;
            #endif
            return false;
        }

        // Allocation de la mémoire pour le staging buffer
        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &this->staging_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateStaginBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindBufferMemory(this->device, this->staging_buffer.handle, this->staging_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateStaginBuffer => vkBindBufferMemory : Failed" << std::endl;
            #endif
            return false;
        }

        constexpr auto offset = 0;
        result = vkMapMemory(this->device, this->staging_buffer.memory, offset, size, 0, &this->staging_buffer.pointer);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateStaginBuffer => vkMapMemory : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(_DEBUG)
        std::cout << "CreateStaginBuffer : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Création du vertex buffer
     */
    uint32_t Vulkan::CreateVertexBuffer(std::vector<VERTEX>& data)
    {
        // Valeur de retour
        VULKAN_BUFFER vertex_buffer;

        ////////////////////////////////
        //     Création du buffer     //
        ////////////////////////////////

        vertex_buffer.size = sizeof(VERTEX) * data.size();

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = vertex_buffer.size;
        buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;

        VkResult result = vkCreateBuffer(this->device, &buffer_create_info, nullptr, &vertex_buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => vkCreateBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(this->device, vertex_buffer.handle, &mem_reqs);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;
        mem_alloc.allocationSize = mem_reqs.size;
        
        if(!this->MemoryTypeFromProperties(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_alloc.memoryTypeIndex)) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => MemoryType VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : Not found" << std::endl;
            #endif
            return false;
        }

        // Allocation de la mémoire pour le vertex buffer
        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &vertex_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindBufferMemory(this->device, vertex_buffer.handle, vertex_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => vkBindBufferMemory : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(_DEBUG)
        std::cout << "CreateVertexBuffer : Success" << std::endl;
        #endif

        if(this->UpdateVertexBuffer(data, vertex_buffer)) {

            // Ajout du buffer dans la mémoire
            uint32_t buffer_index = this->last_vbo_index;
            this->vertex_buffers[buffer_index] = vertex_buffer;
            this->last_vbo_index++;

            return buffer_index;
        
        }else{

            // En cas d'échec, on renvoie UINT32_MAX
            return UINT32_MAX;
        }
    }

    /**
     * Création d'un Mesh
     */
    uint32_t Vulkan::CreateMesh(uint32_t model_id, uint32_t texture_id)
    {
        uint32_t mesh_index = this->last_mesh_index;

        MESH new_mesh;
        new_mesh.offset = this->ubo_alignement * mesh_index;
        new_mesh.vertex_buffer_index = model_id;

        ///////////////////////////////////
        // Allocation des descriptor Set //
        ///////////////////////////////////

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->descriptor_pool.pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &this->descriptor_pool.layout;

        VkResult result = vkAllocateDescriptorSets(this->device, &alloc_info, &new_mesh.descriptor_set);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout <<"CreateMesh => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////
        // Mise à jour du descriptor Set //
        ///////////////////////////////////

        VkWriteDescriptorSet writes[2];

        VkDescriptorImageInfo descriptor_image_info = {};
        descriptor_image_info.sampler = this->textures[texture_id].sampler;
        descriptor_image_info.imageView = this->textures[texture_id].view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].pNext = nullptr;
        writes[0].dstSet = new_mesh.descriptor_set;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &descriptor_image_info;
        writes[0].pBufferInfo = nullptr;
        writes[0].pTexelBufferView = nullptr;

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = this->uniform_buffer.handle;
        buffer_info.offset = 0;
        buffer_info.range = static_cast<VkDeviceSize>(sizeof(Matrix4x4));

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].pNext = nullptr;
        writes[1].dstSet = new_mesh.descriptor_set;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        writes[1].pImageInfo = nullptr;
        writes[1].pBufferInfo = &buffer_info;
        writes[1].pTexelBufferView = nullptr;
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        vkUpdateDescriptorSets(this->device, 2, writes, 0, nullptr);

        this->meshes[mesh_index] = new_mesh;
        this->last_mesh_index++;

        return mesh_index;
    }

    /**
     * Création d'un uniform buffer pour y mettre la matrice de projection
     */
    bool Vulkan::CreateUniformBuffer()
    {
        ////////////////////////////////
        //     Création du buffer     //
        ////////////////////////////////

        // Matrice 4*4 float
        uint32_t mesh_count = 2;
        this->uniform_buffer.size = this->ubo_alignement * (mesh_count - 1) + sizeof(Matrix4x4);

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = this->uniform_buffer.size;
        buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;

        VkResult result = vkCreateBuffer(this->device, &buffer_create_info, nullptr, &this->uniform_buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateUniformBuffer => vkCreateBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(this->device, this->uniform_buffer.handle, &mem_reqs);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;
        mem_alloc.allocationSize = mem_reqs.size;
        
        if(!this->MemoryTypeFromProperties(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_alloc.memoryTypeIndex)) {
            #if defined(_DEBUG)
            std::cout << "CreateUniformBuffer => MemoryType VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : Not found" << std::endl;
            #endif
            return false;
        }

        // Allocation de la mémoire pour le vertex buffer
        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &this->uniform_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateUniformBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindBufferMemory(this->device, this->uniform_buffer.handle, this->uniform_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout <<"CreateUniformBuffer => vkBindBufferMemory : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(_DEBUG)
        std::cout << "CreateUniformBuffer : Success" << std::endl;
        #endif

        return true;
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
        attachment[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment[0].flags = 0;
        attachment[1].format = VK_FORMAT_D16_UNORM;
        attachment[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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
        subpass.pDepthStencilAttachment = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = nullptr;
        rp_info.attachmentCount = 2;
        rp_info.pAttachments = attachment;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 0;
        rp_info.pDependencies = nullptr;

        VkResult result = vkCreateRenderPass(this->device, &rp_info, nullptr, &this->render_pass);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateRenderPass => vkCreateRenderPass : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(_DEBUG)
        std::cout << "CreateRenderPass : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Création du pipeline
     */
    bool Vulkan::CreatePipeline(bool dynamic_viewport)
    {
        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &this->descriptor_pool.layout;

        VkResult result = vkCreatePipelineLayout(this->device, &pPipelineLayoutCreateInfo, nullptr, &this->graphics_pipeline.layout);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreatePipeline => vkCreatePipelineLayout : Failed" << std::endl;
            #endif
            return false;
        }

        // Fragment Sgader
        std::vector<char> code = Engine::Tools::GetBinaryFileContents("./Assets/shader.frag.spv");
        if(!code.size()) {
            #if defined(_DEBUG)
            std::cout << "CreatePipeline => GetBinaryFileContents [fragment] : Failed" << std::endl;
            #endif
            return false;
        }

        VkShaderModuleCreateInfo fragment_shader_module_create_info = {};
        fragment_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragment_shader_module_create_info.pNext = nullptr;
        fragment_shader_module_create_info.flags = 0;
        fragment_shader_module_create_info.codeSize = code.size();
        fragment_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule fragment_shader_module;
		result = vkCreateShaderModule(this->device, &fragment_shader_module_create_info, nullptr, &fragment_shader_module);
		if(result != VK_SUCCESS) {
			#if defined(_DEBUG)
            std::cout << "CreatePipeline => vkCreateShaderModule [fragment] : Failed" << std::endl;
            #endif
			return false;
		}

        // Vertex Shader
        code = Engine::Tools::GetBinaryFileContents("./Assets/shader.vert.spv.07");
        if(!code.size()) {
            #if defined(_DEBUG)
            std::cout << "CreatePipeline => GetBinaryFileContents [vertex] : Failed" << std::endl;
            #endif
            return false;
        }
        VkShaderModuleCreateInfo vertex_shader_module_create_info = {};
        vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertex_shader_module_create_info.pNext = nullptr;
        vertex_shader_module_create_info.flags = 0;
        vertex_shader_module_create_info.codeSize = code.size();
        vertex_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule vertex_shader_module;
		result = vkCreateShaderModule(this->device, &vertex_shader_module_create_info, nullptr, &vertex_shader_module);
		if(result != VK_SUCCESS) {
			#if defined(_DEBUG)
            std::cout << "CreatePipeline => vkCreateShaderModule [vertex] : Failed" << std::endl;
            #endif
			return false;
		}

        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos;
        shader_stage_create_infos.resize(2);
        shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_infos[0].pNext = nullptr;
        shader_stage_create_infos[0].flags = 0;
        shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shader_stage_create_infos[0].module = vertex_shader_module;
        shader_stage_create_infos[0].pName = "main";
        shader_stage_create_infos[0].pSpecializationInfo = nullptr;

        shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_infos[1].pNext = nullptr;
        shader_stage_create_infos[1].flags = 0;
        shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shader_stage_create_infos[1].module = fragment_shader_module;
        shader_stage_create_infos[1].pName = "main";
        shader_stage_create_infos[1].pSpecializationInfo = nullptr;
        
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
            viewport.width = static_cast<float>(this->draw_surface.width);
            viewport.height = static_cast<float>(this->draw_surface.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            scissor.offset.x = 0;
            scissor.offset.y = 0;
            scissor.extent.width = this->draw_surface.width;
            scissor.extent.height = this->draw_surface.height;

            viewport_state_create_info.pScissors = &scissor;
            viewport_state_create_info.pViewports = &viewport;
        }

        VkVertexInputBindingDescription vertex_binding_description = {};
        vertex_binding_description.binding = 0;
        vertex_binding_description.stride = sizeof(VERTEX);
        vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vertex_attribute_descriptions[2] = {};
        vertex_attribute_descriptions[0].location = 0;
        vertex_attribute_descriptions[0].binding = vertex_binding_description.binding;
        vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_attribute_descriptions[0].offset = 0;
        vertex_attribute_descriptions[1].location = 1;
        vertex_attribute_descriptions[1].binding = vertex_binding_description.binding;
        vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_attribute_descriptions[1].offset = offsetof(struct VERTEX, texture_coordinates);

        VkPipelineVertexInputStateCreateInfo vi;
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.pNext = nullptr;
        vi.flags = 0;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &vertex_binding_description;
        vi.vertexAttributeDescriptionCount = 2;
        vi.pVertexAttributeDescriptions = vertex_attribute_descriptions;

        VkPipelineInputAssemblyStateCreateInfo ia;
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.pNext = nullptr;
        ia.flags = 0;
        ia.primitiveRestartEnable = VK_FALSE;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rs;
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.pNext = nullptr;
        rs.flags = 0;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_BACK_BIT;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.depthClampEnable = VK_FALSE;
        rs.rasterizerDiscardEnable = VK_FALSE;
        rs.depthBiasEnable = VK_FALSE;
        rs.depthBiasConstantFactor = 0.0f;
        rs.depthBiasClamp = 0.0f;
        rs.depthBiasSlopeFactor = 0.0f;
        rs.lineWidth = 1.0f;

        VkPipelineColorBlendAttachmentState att_state[1];
        att_state[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        att_state[0].blendEnable = VK_FALSE;
        att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
        att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
        att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        VkPipelineColorBlendStateCreateInfo cb;
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.pNext = nullptr;
        cb.flags = 0;
        cb.attachmentCount = 1;
        cb.pAttachments = att_state;
        cb.logicOpEnable = VK_FALSE;
        cb.logicOp = VK_LOGIC_OP_COPY;
        cb.blendConstants[0] = 0.0f;
        cb.blendConstants[1] = 0.0f;
        cb.blendConstants[2] = 0.0f;
        cb.blendConstants[3] = 0.0f;

        /*VkPipelineDepthStencilStateCreateInfo ds;
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.pNext = nullptr;
        ds.flags = 0;
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = VK_TRUE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        ds.depthBoundsTestEnable = VK_FALSE;
        ds.minDepthBounds = 0.0f;
        ds.maxDepthBounds = 1.0f;
        ds.stencilTestEnable = VK_FALSE;
        ds.back.failOp = VK_STENCIL_OP_KEEP;
        ds.back.passOp = VK_STENCIL_OP_KEEP;
        ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
        ds.back.compareMask = 0;
        ds.back.reference = 0;
        ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
        ds.back.writeMask = 0;
        ds.front = ds.back;*/

        VkPipelineMultisampleStateCreateInfo ms;
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.pNext = nullptr;
        ms.flags = 0;
        ms.pSampleMask = nullptr;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        ms.sampleShadingEnable = VK_FALSE;
        ms.alphaToCoverageEnable = VK_FALSE;
        ms.alphaToOneEnable = VK_FALSE;
        ms.minSampleShading = 1.0f;

        VkGraphicsPipelineCreateInfo pipeline;
        pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline.pNext = nullptr;
        pipeline.layout = this->graphics_pipeline.layout;
        pipeline.basePipelineHandle = VK_NULL_HANDLE;
        pipeline.basePipelineIndex = 0;
        pipeline.flags = 0;
        pipeline.pVertexInputState = &vi;
        pipeline.pInputAssemblyState = &ia;
        pipeline.pRasterizationState = &rs;
        pipeline.pColorBlendState = &cb;
        pipeline.pTessellationState = nullptr;
        pipeline.pMultisampleState = &ms;
        pipeline.pDynamicState = (dynamic_viewport) ? &dynamicState : nullptr;
        pipeline.pViewportState = &viewport_state_create_info;
        pipeline.pDepthStencilState = nullptr;
        pipeline.pStages = &shader_stage_create_infos[0];
        pipeline.stageCount = 2;
        pipeline.renderPass = this->render_pass;
        pipeline.subpass = 0;

        result = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &this->graphics_pipeline.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreatePipeline => vkCreateGraphicsPipelines : Failed" << std::endl;
            #endif
            return false;
        }

        vkDestroyShaderModule(this->device, shader_stage_create_infos[0].module, nullptr);
        vkDestroyShaderModule(this->device, shader_stage_create_infos[1].module, nullptr);

        #if defined(_DEBUG)
        std::cout << "CreatePipeline : Success" << std::endl;
        #endif
        return true;
    }

    bool Vulkan::CreateDescriptorSets()
    {
        VkDescriptorPoolSize pool_sizes[2];
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[0].descriptorCount = 1;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[1].descriptorCount = 1;

        VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = nullptr;
        descriptor_pool_create_info.maxSets = static_cast<uint32_t>(this->swap_chain.images.size());
        descriptor_pool_create_info.poolSizeCount = 2;
        descriptor_pool_create_info.pPoolSizes = pool_sizes;

        VkResult result = vkCreateDescriptorPool(this->device, &descriptor_pool_create_info, nullptr, &this->descriptor_pool.pool);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDescriptorSet => vkCreateDescriptorPool : Failed" << std::endl;
            #endif
            return false;
        }

        VkDescriptorSetLayoutBinding layout_bindings[2];
        layout_bindings[0].binding = 0;
        layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout_bindings[0].descriptorCount = 1;
        layout_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[0].pImmutableSamplers = nullptr;
        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layout_bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 2;
        descriptor_layout.pBindings = layout_bindings;

        result = vkCreateDescriptorSetLayout(this->device, &descriptor_layout, nullptr, &this->descriptor_pool.layout);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDescriptorSet => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }
        
        // Succès
        #if defined(_DEBUG)
        std::cout << "CreateDescriptorSet : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Création des command buffers
     */
    bool Vulkan::CreateCommandBuffers()
    {
        // Création du command pool de la graphics queue
        if(!this->CreateCommandPool(this->graphics_pool, this->queue_stack[this->graphics_stack_index].index)) return false;

        // Création du command pool de la transfer queue
        bool same_queue = this->graphics_stack_index == this->transfer_stack_index;
        if(!same_queue) { if(!this->CreateCommandPool(this->transfer.command_pool, this->queue_stack[this->transfer_stack_index].index)) return false; }
        else this->transfer.command_pool = this->graphics_pool;

        // Allocation des command buffers pour la graphics queue
        std::vector<VkCommandBuffer> graphics_buffers(this->rendering.size());
        if(!this->AllocateCommandBuffer(this->graphics_pool, static_cast<uint32_t>(this->rendering.size()), graphics_buffers)) {
            #if defined(_DEBUG)
            std::cout << "AllocateCommandBuffer [graphics] : Failed" << std::endl;
            #endif
            return false;
        }
        for(uint8_t i=0; i<this->rendering.size(); i++) this->rendering[i].command_buffer = graphics_buffers[i];

        // Allocation du command buffer pour la transfer queue
        std::vector<VkCommandBuffer> transfer_buffers(1);
        if(!this->AllocateCommandBuffer(this->transfer.command_pool, 1, transfer_buffers)) {
        #if defined(_DEBUG)
            std::cout << "AllocateCommandBuffer [transfer] : Failed" << std::endl;
            #endif
            return false;
        }
        this->transfer.command_buffer = transfer_buffers[0];

        // Succès
        #if defined(_DEBUG)
        std::cout << "CreateCommandBuffers : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Création d'un command pool
     */
    bool Vulkan::CreateCommandPool(VkCommandPool& pool, uint32_t queue_family_index)
    {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.queueFamilyIndex = queue_family_index;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VkResult res = vkCreateCommandPool(this->device, &cmd_pool_info, nullptr, &pool);
        if(res != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateCommandPool => vkCreateCommandPool : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(_DEBUG)
        std::cout << "CreateCommandPool [" << queue_family_index << "] : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Allocation des command buffers
     */
    bool Vulkan::AllocateCommandBuffer(VkCommandPool& pool, uint32_t count, std::vector<VkCommandBuffer>& buffer)
    {
        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = pool;
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = count;

        VkResult result = vkAllocateCommandBuffers(this->device, &cmd, buffer.data());
        return result == VK_SUCCESS;
    }

    bool Vulkan::CreateSemaphores()
    {
        VkSemaphoreCreateInfo semaphore_create_info = {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = nullptr;
        semaphore_create_info.flags = 0;

        for(size_t i=0; i<this->rendering.size(); i++) {

            VkResult result = vkCreateSemaphore(this->device, &semaphore_create_info, nullptr, &this->rendering[i].swap_chain_semaphore);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateSemaphores => vkCreateSemaphore [swap_cahin] : Failed" << std::endl;
                #endif
                return false;
            }

            result = vkCreateSemaphore(this->device, &semaphore_create_info, nullptr, &this->rendering[i].render_pass_semaphore);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateSemaphores => vkCreateSemaphore [graphics_queue] : Failed" << std::endl;
                #endif
                return false;
            }
        }

        #if defined(_DEBUG)
        std::cout << "CreateSemaphores : Success" << std::endl;
        #endif
        return true;
    }

    bool Vulkan::CreateFences()
    {

        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = nullptr;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i=0; i<this->rendering.size(); i++) {

            VkResult result = vkCreateFence(this->device, &fence_create_info, nullptr, &this->rendering[i].fence);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateFences => vkCreateFence [graphics] : Failed" << std::endl;
                #endif
                return false;
            }
        }

        VkResult result = vkCreateFence(this->device, &fence_create_info, nullptr, &this->transfer.fence);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateFences => vkCreateFence [transfer] : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(_DEBUG)
        std::cout << "CreateFences : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Mise à jour des données du uniform buffer
     */
    bool Vulkan::UpdateMeshUniformBuffers()
    {
        //////////////////////////////////////
        //   Transformations géométriques   //
        //////////////////////////////////////

        static float angle_y = 0;
        static float angle_x = 0;

        angle_y += 1.0f;
        angle_x += 0.3f;

        Matrix4x4 projection = PerspectiveProjectionMatrix(
            static_cast<float>(this->draw_surface.width) / static_cast<float>(this->draw_surface.height),   // aspect_ration
            45.0f,                                                                                          // field_of_view
            1.0f,                                                                                           // near_clip
            20.0f                                                                                           // far_clip
        );

        Matrix4x4 rotationX = RotationMatrix(angle_x, {1.0f, 0.0f, 0.0f});
        Matrix4x4 rotationY = RotationMatrix(angle_y, {0.0f, 1.0f, 0.0f});
        Matrix4x4 translation = TranslationMatrix(1.8f, 0.0f, -7.0f);
        this->meshes[0].transformations = projection * translation * rotationX * rotationY;

        rotationX = RotationMatrix(angle_x, {-1.0f, 0.0f, 0.0f});
        rotationY = RotationMatrix(angle_y, {0.0f, 1.0f, 0.0f});
        translation = TranslationMatrix(-1.8f, 0.0f, -7.0f);
        this->meshes[1].transformations = projection * translation * rotationX * rotationY;

        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        VkResult result = vkWaitForFences(this->device, 1, &this->transfer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "UpdateUniforBuffer => vkWaitForFences : Timeout" << std::endl;
            #endif
            return false;
        }
        vkResetFences(this->device, 1, &this->transfer.fence);

        ////////////////////////////////
        //   Copie des données vers   //
        //      le staging buffer     //
        ////////////////////////////////

        for(std::pair<uint32_t, MESH> mesh : this->meshes) {
            void* staging_buffer_position = reinterpret_cast<char*>(this->staging_buffer.pointer) + mesh.second.offset;
            std::memcpy(staging_buffer_position, &mesh.second.transformations, sizeof(Matrix4x4));
        }

        size_t flush_size = static_cast<size_t>(this->uniform_buffer.size);
        unsigned int multiple = static_cast<unsigned int>(flush_size / this->physical_device.properties.limits.nonCoherentAtomSize);
        flush_size = this->physical_device.properties.limits.nonCoherentAtomSize * ((uint64_t)multiple + 1);

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->staging_buffer.memory;
        flush_range.offset = 0;
        flush_range.size = flush_size;
        vkFlushMappedMemoryRanges(this->device, 1, &flush_range);

        ///////////////////////////////////////////
        //  Envoi de la matrice de projection    //
        //        vers le uniform buffer         //
        ///////////////////////////////////////////

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(this->transfer.command_buffer, &command_buffer_begin_info);

        VkBufferCopy buffer_copy_info = {};
        buffer_copy_info.srcOffset = 0;
        buffer_copy_info.srcOffset = 0;
        buffer_copy_info.size = this->uniform_buffer.size;

        vkCmdCopyBuffer(this->transfer.command_buffer, this->staging_buffer.handle, this->uniform_buffer.handle, 1, &buffer_copy_info);
        
        vkEndCommandBuffer(this->transfer.command_buffer);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->transfer.command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->queue_stack[this->transfer_stack_index].handle, 1, &submit_info, this->transfer.fence);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "UpdateUniforBuffer => vkQueueSubmit : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    uint32_t Vulkan::CreateTexture(std::vector<unsigned char> data, uint32_t width, uint32_t height)
    {
        TEXTURE texture = {};

        /////////////////////////
        // Création de l'image //
        /////////////////////////

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = nullptr;
        image_info.flags = 0;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = nullptr;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VkResult result = vkCreateImage(this->device, &image_info, nullptr, &texture.image);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkCreateImage : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////////////////////////
        //      Réservation d'un segment de mémoire          //
        // dans la carte graphique pour y placer la texture //
        ///////////////////////////////////////////////////////

        VkMemoryRequirements image_memory_requirements;
        vkGetImageMemoryRequirements(this->device, texture.image, &image_memory_requirements);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = image_memory_requirements.size;
        
        if(!this->MemoryTypeFromProperties(image_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_alloc.memoryTypeIndex)) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => MemoryType VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : Not found" << std::endl;
            #endif
            return false;
        }

        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &texture.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindImageMemory(this->device, texture.image, texture.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkBindImageMemory : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////////////
        // Création de l'image view //
        //////////////////////////////

        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.pNext = nullptr;
        view_info.image = VK_NULL_HANDLE;
        view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.flags = 0;
        view_info.image = texture.image;

        result = vkCreateImageView(this->device, &view_info, nullptr, &texture.view);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkCreateImageView : Failed" << std::endl;
            #endif

            return false;
        }

        /////////////////////////
        // Création du sampler //
        /////////////////////////

        VkSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = nullptr;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = VK_FILTER_LINEAR;
        sampler_create_info.minFilter = VK_FILTER_LINEAR;
        sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_create_info.mipLodBias = 0.0f;
        sampler_create_info.anisotropyEnable = VK_FALSE;
        sampler_create_info.maxAnisotropy = 1.0f;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_create_info.minLod = 0.0f;
        sampler_create_info.maxLod = 0.0f;
        sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;

        result = vkCreateSampler(this->device, &sampler_create_info, nullptr, &texture.sampler);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkCreateSampler : Failed" << std::endl;
            #endif
            return false;
        }

        //////////////////////////////
        // Copie de la texture vers //
        //    le staging buffer     //
        //////////////////////////////

        size_t flush_size = static_cast<size_t>(data.size() * sizeof(data[0]));
        std::memcpy(this->staging_buffer.pointer, &data[0], flush_size);

        unsigned int multiple = static_cast<unsigned int>(flush_size / this->physical_device.properties.limits.nonCoherentAtomSize);
        flush_size = this->physical_device.properties.limits.nonCoherentAtomSize * ((uint64_t)multiple + 1);

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->staging_buffer.memory;
        flush_range.offset = 0;
        flush_range.size = flush_size;
        vkFlushMappedMemoryRanges(this->device, 1, &flush_range);

        //////////////////////////////////////
        //     Envoi de la texture vers     //
        // la mémoire de la carte graphique //
        //////////////////////////////////////

        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        result = vkWaitForFences(this->device, 1, &this->transfer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkWaitForFences : Timeout" << std::endl;
            #endif
            return false;
        }
        vkResetFences(this->device, 1, &this->transfer.fence);

        // Préparation de la copie de l'image depuis le staging buffer vers un vertex buffer
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(this->transfer.command_buffer, &command_buffer_begin_info);

        VkImageMemoryBarrier image_memory_barrier_from_undefined_to_transfer_dst = {};
        image_memory_barrier_from_undefined_to_transfer_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier_from_undefined_to_transfer_dst.pNext = nullptr;
        image_memory_barrier_from_undefined_to_transfer_dst.srcAccessMask = 0;
        image_memory_barrier_from_undefined_to_transfer_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier_from_undefined_to_transfer_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier_from_undefined_to_transfer_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier_from_undefined_to_transfer_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier_from_undefined_to_transfer_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier_from_undefined_to_transfer_dst.image = texture.image;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.baseMipLevel = 0;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.levelCount = 1;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier_from_undefined_to_transfer_dst.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(this->transfer.command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_undefined_to_transfer_dst);

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
        buffer_image_copy_info.imageExtent.width = width;
        buffer_image_copy_info.imageExtent.height = height;
        buffer_image_copy_info.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(this->transfer.command_buffer, this->staging_buffer.handle, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy_info);

        if(this->transfer_stack_index == this->graphics_stack_index) {
            VkImageMemoryBarrier image_memory_barrier_from_transfer_to_shader_read = {};
            image_memory_barrier_from_transfer_to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_memory_barrier_from_transfer_to_shader_read.pNext = nullptr;
            image_memory_barrier_from_transfer_to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier_from_transfer_to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            image_memory_barrier_from_transfer_to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_memory_barrier_from_transfer_to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_memory_barrier_from_transfer_to_shader_read.srcQueueFamilyIndex = this->queue_stack[this->transfer_stack_index].index;
            image_memory_barrier_from_transfer_to_shader_read.dstQueueFamilyIndex = this->queue_stack[this->graphics_stack_index].index;
            image_memory_barrier_from_transfer_to_shader_read.image = texture.image;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.baseMipLevel = 0;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.levelCount = 1;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.baseArrayLayer = 0;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(this->transfer.command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_transfer_to_shader_read);
        }

        vkEndCommandBuffer(this->transfer.command_buffer);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->transfer.command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->queue_stack[this->transfer_stack_index].handle, 1, &submit_info, this->transfer.fence);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkQueueSubmit : Failed" << std::endl;
            #endif
            return false;
        }

        if(this->transfer_stack_index != this->graphics_stack_index) {

            VkResult result = vkWaitForFences(this->device, 1, &this->transfer.fence, VK_FALSE, 1000000000);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateTexture => vkWaitForFences : Timeout" << std::endl;
                #endif
                return false;
            }
            vkResetFences(this->device, 1, &this->transfer.fence);

            vkBeginCommandBuffer(this->rendering[0].command_buffer, &command_buffer_begin_info);

            VkImageMemoryBarrier image_memory_barrier_from_transfer_to_shader_read = {};
            image_memory_barrier_from_transfer_to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_memory_barrier_from_transfer_to_shader_read.pNext = nullptr;
            image_memory_barrier_from_transfer_to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier_from_transfer_to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            image_memory_barrier_from_transfer_to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_memory_barrier_from_transfer_to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_memory_barrier_from_transfer_to_shader_read.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barrier_from_transfer_to_shader_read.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barrier_from_transfer_to_shader_read.image = texture.image;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.baseMipLevel = 0;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.levelCount = 1;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.baseArrayLayer = 0;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(this->rendering[0].command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_transfer_to_shader_read);

            vkEndCommandBuffer(this->rendering[0].command_buffer);

            submit_info.pCommandBuffers = &this->rendering[0].command_buffer;
            result = vkQueueSubmit(this->queue_stack[this->graphics_stack_index].handle, 1, &submit_info, this->transfer.fence);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateTexture => vkQueueSubmit(2) : Failed" << std::endl;
                #endif
                return false;
            }
        }

        // Ajout de la texture dans la mémoire
        uint32_t texture_index = this->last_texture_index;
        this->textures[texture_index] = texture;
        this->last_texture_index++;

        #if defined(_DEBUG)
        std::cout << "CreateTexture : Success" << std::endl;
        #endif

        // On renvoie l'indice de la texture dans le 
        return texture_index;
    }

    /**
     * Mise à jour des données du vertex buffer
     */
    bool Vulkan::UpdateVertexBuffer(std::vector<VERTEX>& data, VULKAN_BUFFER& vertex_buffer)
    {
        ////////////////////////////////
        //   Copie des données vers   //
        //      le staging buffer     //
        ////////////////////////////////

        std::memcpy(this->staging_buffer.pointer, &data[0], vertex_buffer.size);

        size_t flush_size = static_cast<size_t>(vertex_buffer.size);
        unsigned int multiple = static_cast<unsigned int>(flush_size / this->physical_device.properties.limits.nonCoherentAtomSize);
        flush_size = this->physical_device.properties.limits.nonCoherentAtomSize * ((uint64_t)multiple + 1);

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->staging_buffer.memory;
        flush_range.offset = 0;
        flush_range.size = flush_size;
        vkFlushMappedMemoryRanges(this->device, 1, &flush_range);

        ///////////////////////////////////////////
        //  Envoi des données vectorielles vers  //
        //   la mémoire de la carte graphique    //
        ///////////////////////////////////////////

        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        VkResult result = vkWaitForFences(this->device, 1, &this->transfer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "UpdateVertexBuffer => vkWaitForFences : Timeout" << std::endl;
            #endif
            return false;
        }
        vkResetFences(this->device, 1, &this->transfer.fence);

        // Préparation de la copie de l'image depuis le staging buffer vers un vertex buffer
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(this->transfer.command_buffer, &command_buffer_begin_info);

        VkBufferCopy buffer_copy_info = {};
        buffer_copy_info.srcOffset = 0;
        buffer_copy_info.srcOffset = 0;
        buffer_copy_info.size = vertex_buffer.size;

        vkCmdCopyBuffer(this->transfer.command_buffer, this->staging_buffer.handle, vertex_buffer.handle, 1, &buffer_copy_info);

        VkBufferMemoryBarrier buffer_memory_barrier = {};
        buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.pNext = nullptr;
        buffer_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        buffer_memory_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        buffer_memory_barrier.srcQueueFamilyIndex = this->queue_stack[this->transfer_stack_index].index;
        buffer_memory_barrier.dstQueueFamilyIndex = this->queue_stack[this->graphics_stack_index].index;
        buffer_memory_barrier.buffer = vertex_buffer.handle;
        buffer_memory_barrier.offset = 0;
        buffer_memory_barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(this->transfer.command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        
        vkEndCommandBuffer(this->transfer.command_buffer);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->transfer.command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->queue_stack[this->transfer_stack_index].handle, 1, &submit_info, this->transfer.fence);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "UpdateVertexBuffer => vkQueueSubmit : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(_DEBUG)
        std::cout << "UpdateVertexBuffer : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Démarrage des threads d'affichage
     */
    void Vulkan::Start()
    {
        if(this->keep_runing) return;

        this->Stop();
        this->keep_runing = true;
        this->draw_thread = std::thread(ThreadLoop, this);
    }

    /**
     * Arrêt des threads d'affichage
     */
    void Vulkan::Stop()
    {
        this->keep_runing = false;
        if(this->draw_thread.joinable()) this->draw_thread.join();
    }

    /**
     * Affichage
     */
    void Vulkan::ThreadLoop(Vulkan* self)
    {
        uint8_t resource_count = static_cast<uint8_t>(self->swap_chain.images.size());
        uint8_t resource_index = resource_count - 1;
        VkQueue graphics_queue = self->queue_stack[self->graphics_stack_index].handle;
        VkQueue present_queue = self->queue_stack[self->present_stack_index].handle;

        while(self->keep_runing)
        {
            // Pause de 10 ms
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Transformations géométriques
            self->UpdateMeshUniformBuffers();

            //////////////////////////////
            //     Rotation du pool     //
            //  de ressources de rendu  //
            //////////////////////////////

            resource_index = (resource_index + 1) % resource_count;
            auto current_rendering_resource = self->rendering[resource_index];

            ////////////////////////////////
            //    Attente de libération   //
            //   des resources de rendu   //
            ////////////////////////////////

            VkResult result = vkWaitForFences(self->device, 1, &current_rendering_resource.fence, VK_FALSE, 1000000000);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "Draw => vkWaitForFences : Timeout" << std::endl;
                #endif
                return;
            }
            vkResetFences(self->device, 1, &current_rendering_resource.fence);

            ////////////////////////////////////
            //    Récupération d'une image    //
            //       de la Swap Chain         //
            ////////////////////////////////////

            uint32_t swap_chain_image_index;
            result = vkAcquireNextImageKHR(self->device, self->swap_chain.handle, UINT64_MAX, current_rendering_resource.swap_chain_semaphore, VK_NULL_HANDLE, &swap_chain_image_index);
            switch(result) {
                #if defined(_DEBUG)
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    std::cout << "vkAcquireNextImageKHR : VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
                    return;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    std::cout << "vkAcquireNextImageKHR : VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
                    return;
                case VK_ERROR_DEVICE_LOST:
                    std::cout << "vkAcquireNextImageKHR : VK_ERROR_DEVICE_LOST" << std::endl;
                    return;
                case VK_ERROR_SURFACE_LOST_KHR:
                    std::cout << "vkAcquireNextImageKHR : VK_ERROR_SURFACE_LOST_KHR" << std::endl;
                    return;
                case VK_TIMEOUT:
                    std::cout << "vkAcquireNextImageKHR : VK_TIMEOUT" << std::endl;
                    return;
                case VK_NOT_READY:
                    std::cout << "vkAcquireNextImageKHR : VK_NOT_READY" << std::endl;
                    return;
                case VK_ERROR_VALIDATION_FAILED_EXT:
                    std::cout << "vkAcquireNextImageKHR : VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
                    return;
                #endif

                case VK_SUCCESS:
                case VK_SUBOPTIMAL_KHR:
                    break;
                case VK_ERROR_OUT_OF_DATE_KHR:
                    self->OnWindowSizeChanged();
                default:
                    #if defined(_DEBUG)
                    std::cout << "Draw => vkAcquireNextImageKHR : Failed" << std::endl;
                    #endif
                    return;
            }

            ////////////////////////////////////
            //    Création du Frame Buffer    //   
            ////////////////////////////////////
        
            if(current_rendering_resource.framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(self->device, current_rendering_resource.framebuffer, nullptr);
                self->rendering[resource_index].framebuffer = VK_NULL_HANDLE;
            }

            VkImageView pAttachments[2];
            pAttachments[0] = self->swap_chain.images[swap_chain_image_index].view;
            pAttachments[1] = self->depth_buffer.view;

            VkFramebufferCreateInfo framebuffer_create_info = {};
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.pNext = nullptr;
            framebuffer_create_info.flags = 0;
            framebuffer_create_info.renderPass = self->render_pass;
            framebuffer_create_info.attachmentCount = 2;
            framebuffer_create_info.pAttachments = pAttachments;
            framebuffer_create_info.width = self->draw_surface.width;
            framebuffer_create_info.height = self->draw_surface.height;
            framebuffer_create_info.layers = 1;
            
            result = vkCreateFramebuffer(self->device, &framebuffer_create_info, nullptr, &self->rendering[resource_index].framebuffer);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "Draw => vkCreateFramebuffer : Failed" << std::endl;
                #endif
                return;
            }

            /////////////////////////////////
            //    Génération de l'image    //   
            /////////////////////////////////
            
            VkCommandBufferBeginInfo command_buffer_begin_info = {};
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.pNext = nullptr;
            command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            command_buffer_begin_info.pInheritanceInfo = nullptr;

            VkCommandBuffer command_buffer = current_rendering_resource.command_buffer;
            vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

            VkImageSubresourceRange image_subresource_range = {};
            image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_subresource_range.baseMipLevel = 0;
            image_subresource_range.levelCount = 1;
            image_subresource_range.baseArrayLayer = 0;
            image_subresource_range.layerCount = 1;

            uint32_t present_queue_family_index;
            uint32_t graphics_queue_family_index;
            if(self->graphics_stack_index == self->present_stack_index) {
                present_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
                graphics_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
            }else{
                present_queue_family_index = self->queue_stack[self->present_stack_index].index;
                graphics_queue_family_index = self->queue_stack[self->graphics_stack_index].index;
            }
        
            VkImageMemoryBarrier barrier_from_present_to_draw = {};
            barrier_from_present_to_draw.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier_from_present_to_draw.pNext = nullptr;
            barrier_from_present_to_draw.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier_from_present_to_draw.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier_from_present_to_draw.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier_from_present_to_draw.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier_from_present_to_draw.srcQueueFamilyIndex = present_queue_family_index;
            barrier_from_present_to_draw.dstQueueFamilyIndex = graphics_queue_family_index;
            barrier_from_present_to_draw.image = self->swap_chain.images[swap_chain_image_index].handle;
            barrier_from_present_to_draw.subresourceRange = image_subresource_range;

            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_present_to_draw);

            VkClearValue clear_value[2];
            clear_value[0].color = { 0.1f, 0.2f, 0.3f, 0.0f };
            clear_value[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };

            VkRenderPassBeginInfo render_pass_begin_info = {};
            render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_begin_info.pNext = nullptr;
            render_pass_begin_info.renderPass = self->render_pass;
            render_pass_begin_info.framebuffer = self->rendering[resource_index].framebuffer;
            render_pass_begin_info.renderArea.offset.x = 0;
            render_pass_begin_info.renderArea.offset.y = 0;
            render_pass_begin_info.renderArea.extent.width = self->draw_surface.width;
            render_pass_begin_info.renderArea.extent.height = self->draw_surface.height;
            render_pass_begin_info.clearValueCount = 2;
            render_pass_begin_info.pClearValues = clear_value;

            vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->graphics_pipeline.handle);

            for(std::pair<uint32_t, MESH> mesh : self->meshes) {
                VkDeviceSize offset = 0;
                VULKAN_BUFFER vertex_buffer = self->vertex_buffers[mesh.second.vertex_buffer_index];
                vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer.handle, &offset);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->graphics_pipeline.layout, 0, 1, &mesh.second.descriptor_set, 0, &mesh.second.offset);

                uint32_t vertex_count = static_cast<uint32_t>(vertex_buffer.size / sizeof(VERTEX));
                vkCmdDraw(command_buffer, vertex_count, 1, 0, 0);
            }
            

            vkCmdEndRenderPass(command_buffer);

            VkImageMemoryBarrier barrier_from_draw_to_present = {};
            barrier_from_draw_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier_from_draw_to_present.pNext = nullptr;
            barrier_from_draw_to_present.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier_from_draw_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier_from_draw_to_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier_from_draw_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier_from_draw_to_present.srcQueueFamilyIndex = graphics_queue_family_index;
            barrier_from_draw_to_present.dstQueueFamilyIndex = present_queue_family_index;
            barrier_from_draw_to_present.image = self->swap_chain.images[swap_chain_image_index].handle;
            barrier_from_draw_to_present.subresourceRange = image_subresource_range;

            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_draw_to_present);

            result = vkEndCommandBuffer(command_buffer);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "Draw => vkEndCommandBuffer : Failed" << std::endl;
                #endif
                return;
            }

            VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submit_info = {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = nullptr;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &current_rendering_resource.swap_chain_semaphore;
            submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &current_rendering_resource.command_buffer;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &current_rendering_resource.render_pass_semaphore;

            result = vkQueueSubmit(graphics_queue, 1, &submit_info, current_rendering_resource.fence);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "Draw => vkQueueSubmit : Failed" << std::endl;
                #endif
                return;
            }

            VkPresentInfoKHR present_info = {};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.pNext = nullptr;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &current_rendering_resource.swap_chain_semaphore;
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &self->swap_chain.handle;
            present_info.pImageIndices = &swap_chain_image_index;
            present_info.pResults = nullptr;

            result = vkQueuePresentKHR(present_queue, &present_info);

            switch(result)
            {
                case VK_SUCCESS:
                    break;
                case VK_ERROR_OUT_OF_DATE_KHR:
                case VK_SUBOPTIMAL_KHR:
                    self->OnWindowSizeChanged();
                default:
                    #if defined(_DEBUG)
                    std::cout << "Draw => vkQueuePresentKHR : Failed" << std::endl;
                    #endif
                    return;
            }
        }
    }

    /**
    * Lorsque la fenêtre est redimensionnée la swap chain doit être reconstruite
    */
    bool Vulkan::OnWindowSizeChanged()
    {
        if(this->creating_swap_chain) return true;
        this->draw_surface = this->draw_window->GetSurface();
        return this->CreateDepthBuffer() && this->CreateSwapChain();
    }
}