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

        VkResult result = vkCreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &this->report_callback);
        createInfo = {};
    }
    #endif

    /**
     * Permet d'accéder à l'instance du singleton
     */
    Vulkan* Vulkan::GetInstance()
    {
        if(Vulkan::vulkan == nullptr) Vulkan::vulkan = new Vulkan();
        return Vulkan::vulkan;
    }

    /**
     * Constructeur
     */
    Vulkan::Vulkan()
    {
        this->library                   = nullptr;
        this->creating_swap_chain       = false;
        this->keep_thread_alive         = false;
        this->frame_ready.signaled      = false;
        this->instance                  = VK_NULL_HANDLE;
        this->presentation_surface      = VK_NULL_HANDLE;
        this->device                    = VK_NULL_HANDLE;
        this->descriptor_set_layout     = VK_NULL_HANDLE;
        this->descriptor_pool           = VK_NULL_HANDLE;
        this->graphics_command_pool     = VK_NULL_HANDLE;
        this->transfer_command_pool     = VK_NULL_HANDLE;
        this->render_pass               = VK_NULL_HANDLE;
        this->ubo_alignement            = 0;
        this->current_frame_index       = 0;
        this->last_texture_index        = 0;
        this->last_mesh_index           = 0;
        this->last_vbo_index            = 0;
        this->last_bone_tree_index      = 0;
        this->concurrent_frame_count    = 0;
        this->thread_count              = 0;
        this->pick_index                = 0;
        this->job_done_count            = 0;
    }

    /**
     * Destructeur
     */
    Vulkan::~Vulkan()
    {
        vkDeviceWaitIdle(this->device);

        // Threads
        this->ReleaseThreads();

        // Vertex Buffers
        for(std::pair<uint32_t, VERTEX_BUFFER> element : this->vertex_buffers) {
            if(element.second.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, element.second.memory, nullptr);
            if(element.second.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, element.second.handle, nullptr);
        }

        // Index Buffers
        for(std::pair<uint32_t, INDEX_BUFFER> element : this->index_buffers) {
            if(element.second.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, element.second.memory, nullptr);
            if(element.second.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, element.second.handle, nullptr);
        }

        // Textures
        for(auto iterator = this->textures.begin(); iterator!=this->textures.end(); iterator++) {
            if(iterator->second.view != VK_NULL_HANDLE) vkDestroyImageView(this->device, iterator->second.view, nullptr);
            if(iterator->second.image != VK_NULL_HANDLE) vkDestroyImage(this->device, iterator->second.image, nullptr);
            if(iterator->second.sampler != VK_NULL_HANDLE) vkDestroySampler(this->device, iterator->second.sampler, nullptr);
            if(iterator->second.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, iterator->second.memory, nullptr);
        }

        // Depth Buffer
        if(this->depth_buffer.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->depth_buffer.memory, nullptr);
        if(this->depth_buffer.view != VK_NULL_HANDLE) vkDestroyImageView(this->device, this->depth_buffer.view, nullptr);
        if(this->depth_buffer.image != VK_NULL_HANDLE) vkDestroyImage(this->device, this->depth_buffer.image, nullptr);

        // Resources de la création de textures
        this->ReleaseBackgroundResources();

        // Resources partagées
        this->ReleaseThreadSharedResources();

        // Thread Principal
        this->ReleaseMainThread();

        // Pipeline
        if(this->mesh_pipeline.handle != VK_NULL_HANDLE) vkDestroyPipeline(this->device, this->mesh_pipeline.handle, nullptr);
        if(this->normals_pipeline.handle != VK_NULL_HANDLE) vkDestroyPipeline(this->device, this->normals_pipeline.handle, nullptr);
        if(this->mesh_pipeline.layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(this->device, this->mesh_pipeline.layout, nullptr);

        // Descriptor Set
        if(this->descriptor_set_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(this->device, this->descriptor_set_layout, nullptr);
        // if(this->color_descriptor_set_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(this->device, this->color_descriptor_set_layout, nullptr);
        if(this->descriptor_pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(this->device, this->descriptor_pool, nullptr);
        // if(this->color_descriptor_pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(this->device, this->color_descriptor_pool, nullptr);

        // Render Pass
        if(this->render_pass != VK_NULL_HANDLE) vkDestroyRenderPass(this->device, this->render_pass, nullptr);

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
        //if(this->report_callback != VK_NULL_HANDLE) vkDestroyDebugReportCallbackEXT(this->instance, this->report_callback, nullptr);
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
        if(Vulkan::vulkan == nullptr) return;
        delete Vulkan::vulkan;
        Vulkan::vulkan = nullptr;
    }

    /**
     * Initlisation de vulkan
     */
    Vulkan::RETURN_CODE Vulkan::Initialize(Engine::Window* draw_window, uint32_t application_version, std::string aplication_name)
    {
        //////////////////////
        ///   PREPARATION  ///
        //////////////////////

        // On récupère la fenêtre et la surface d'affichage
        this->draw_window = draw_window;
        this->draw_surface = draw_window->GetSurface();

        // Clalcul de la matrice de projection par défaut
        this->base_projection = PerspectiveProjectionMatrix(
            4.0f/3.0f,                                                                                      // aspect_ration
            //static_cast<float>(this->draw_surface.width) / static_cast<float>(this->draw_surface.height),   
            60.0f,                                                                                          // field_of_view
            0.1f,                                                                                           // near_clip
            2000.0f                                                                                         // far_clip
        );

        // Valeur de retourn de la fonction
        RETURN_CODE init_status = RETURN_CODE::SUCCESS;

        #if defined(_DEBUG)
        std::string vulkan_version;
        this->GetVersion(vulkan_version);
        std::cout << "Version de vulkan : " << vulkan_version << std::endl;
        #endif

        // Chargement de la librairie vulkan
        if(!this->LoadPlatformLibrary())
            init_status = RETURN_CODE::LOAD_LIBRARY_FAILURE;
         
        ////////////////////
        // INITIALISATION //
        ////////////////////

        // Chargement de l'exporteur de fonctions vulkan
        if(init_status == RETURN_CODE::SUCCESS && !this->LoadExportedEntryPoints())
            init_status = RETURN_CODE::LOAD_EXPORTED_FUNCTIONS_FAILURE;

        // Chargement des fonctions vulkan globales
        if(init_status == RETURN_CODE::SUCCESS && !this->LoadGlobalLevelEntryPoints())
            init_status = RETURN_CODE::LOAD_GLOBAL_FUNCTIONS_FAILURE;

        // Création de l'instance vulkan
        if(init_status == RETURN_CODE::SUCCESS && !this->CreateVulkanInstance(application_version, aplication_name))
            init_status = RETURN_CODE::VULKAN_INSTANCE_CREATION_FAILURE;

        // Chargement des fonctions portant sur l'instance
        if(init_status == RETURN_CODE::SUCCESS && !this->LoadInstanceLevelEntryPoints())
            init_status = RETURN_CODE::LOAD_INSTANCE_FUNCTIONS_FAILURE;

        #if defined(_DEBUG)
        this->report_callback = VK_NULL_HANDLE;
        this->CreateDebugReportCallback();
        #endif

        // Création de la surface de présentation
        if(init_status == RETURN_CODE::SUCCESS && !this->CreatePresentationSurface())
            init_status = RETURN_CODE::DEVICE_CREATION_FAILURE;

        // Création du logical device
        if(init_status == RETURN_CODE::SUCCESS && !this->CreateDevice())
            init_status = RETURN_CODE::DEVICE_CREATION_FAILURE;

        // Chargement des fonctions portant sur le logical device
        if(init_status == RETURN_CODE::SUCCESS && !this->LoadDeviceLevelEntryPoints())
            init_status = RETURN_CODE::LOAD_DEVICE_FUNCTIONS_FAILURE;

        // Récupère le handle des device queues
        this->GetDeviceQueues();

        // Création de la swap chain
        if(init_status == RETURN_CODE::SUCCESS && !this->CreateSwapChain())
            init_status = RETURN_CODE::SWAP_CHAIN_CREATION_FAILURE;

        // Initialisation de la boucle principale
        if(init_status == RETURN_CODE::SUCCESS && !this->InitMainThread())
            init_status = RETURN_CODE::MAIN_THREAD_INITIALIZATION_FAILURE;

        // Initialisation des resources d'arrière plan
        if(init_status == RETURN_CODE::SUCCESS && !this->InitBackgroundResources())
            init_status = RETURN_CODE::BACKGROUND_INITIALIZATION_FAILURE;

        // Création du depth buffer
        if(init_status == RETURN_CODE::SUCCESS && !this->CreateDepthBuffer())
            init_status = RETURN_CODE::DEPTH_BUFFER_CREATION_FAILURE;

        // Création de la render pass
        if(init_status == RETURN_CODE::SUCCESS && !this->CreateRenderPass())
            init_status = RETURN_CODE::RENDER_PASS_CREATION_FAILURE;

        // Préparation des descriptor sets pour objects texturés
        if(init_status == RETURN_CODE::SUCCESS && !this->PrepareTextureDescriptorSets())
            init_status = RETURN_CODE::DESCRIPTOR_SETS_PREPARATION_FAILURE;

        // Création du pipeline graphique
        if(init_status == RETURN_CODE::SUCCESS && !this->CreatePipeline())
            init_status = RETURN_CODE::GRAPHICS_PIPELINE_CREATION_FAILURE;

        // Création des frame buffers
        if(init_status == RETURN_CODE::SUCCESS && !this->CreateFrameBuffers())
            init_status = RETURN_CODE::FRAME_BUFFERS_CREATION_FAILURE;
        
        // Initialisation des resources partagées
        if(init_status == RETURN_CODE::SUCCESS && !this->InitThreadSharedResources())
            init_status = RETURN_CODE::SHARED_RESOURCES_INITIALIZATION_FAILURE;

        // Initialisation des threads
        if(init_status == RETURN_CODE::SUCCESS && !this->InitThreads())
            init_status = RETURN_CODE::THREAD_INITIALIZATION_FAILURE;

        // En cas d'échecx d'initialisation, on libère les ressources alouées
        if(init_status != RETURN_CODE::SUCCESS) Vulkan::DestroyInstance();

        // Renvoi du résultat
        return init_status;
    }

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
        instance_extension_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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

    /**
     * Récupère la version de Vulkan
     */
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
        this->present_queue.index = this->SelectPresentQueue(this->physical_device.handle, queue_family_properties);

        // Sélection des autres queues
        this->graphics_queue.index = this->SelectPreferredQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_GRAPHICS_BIT, this->present_queue.index);
        this->transfer_queue.index = this->SelectPreferredQueue(this->physical_device.handle, queue_family_properties, VK_QUEUE_TRANSFER_BIT, this->present_queue.index);
        // this->transfer_queue.index = this->graphics_queue.index;
        this->background.transfer_queue.index = this->transfer_queue.index;
        this->background.graphics_queue.index = this->graphics_queue.index;
       
        #if defined(_DEBUG)
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

        VkPhysicalDeviceFeatures features = {};
        features.geometryShader = VK_TRUE;

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
        this->ubo_alignement = sizeof(UBO);
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
    std::vector<VkQueueFamilyProperties> Vulkan::QueryDeviceProperties(VkPhysicalDevice physical_device)
    {
        // On récupère les propriétés des queue families de la carte
        uint32_t queue_family_count = UINT32_MAX;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        if(queue_family_count == 0) {
            #if defined(_DEBUG)
            std::cout << "QueryDeviceProperties => vkGetPhysicalDeviceQueueFamilyProperties 1/2 : Failed" << std::endl;
            #endif
            return std::vector<VkQueueFamilyProperties>();
        }

        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, &queue_family_properties[0]);
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
        vkGetDeviceQueue(this->device, this->present_queue.index, 0, &this->present_queue.handle);
        vkGetDeviceQueue(this->device, this->graphics_queue.index, 0, &this->graphics_queue.handle);
        vkGetDeviceQueue(this->device, this->transfer_queue.index, 0, &this->transfer_queue.handle);

        this->background.graphics_queue = this->graphics_queue;
        this->background.transfer_queue = this->transfer_queue;

        std::vector<VkQueueFamilyProperties> queue_family_properties = this->QueryDeviceProperties(this->physical_device.handle);
        if(queue_family_properties[this->graphics_queue.index].queueCount > 1)
            vkGetDeviceQueue(this->device, this->background.graphics_queue.index, 1, &this->background.graphics_queue.handle);

        if(this->graphics_queue.index != this->transfer_queue.index && queue_family_properties[this->transfer_queue.index].queueCount > 1)
            vkGetDeviceQueue(this->device, this->background.transfer_queue.index, 1, &this->background.transfer_queue.handle);
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
        attachment[1].format = depth_buffer.format;
        attachment[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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
        //subpass.pDepthStencilAttachment = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.pNext = nullptr;
        rp_info.attachmentCount = 2;
        rp_info.pAttachments = attachment;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        rp_info.dependencyCount = 1;
        rp_info.pDependencies = &dependency;

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

    VkPipelineShaderStageCreateInfo Vulkan::LoadShaderModule(std::string const& filename, VkShaderStageFlagBits type)
    {
        std::vector<char> code = Engine::Tools::GetBinaryFileContents(filename);
        if(!code.size()) {
            #if defined(_DEBUG)
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
			#if defined(_DEBUG)
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
        pPipelineLayoutCreateInfo.pSetLayouts = &this->descriptor_set_layout;

        VkResult result = vkCreatePipelineLayout(this->device, &pPipelineLayoutCreateInfo, nullptr, &this->mesh_pipeline.layout);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreatePipeline => vkCreatePipelineLayout : Failed" << std::endl;
            #endif
            return false;
        }

        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
        shader_stages[0] = this->LoadShaderModule("./Shaders/textured_model.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        shader_stages[1] = this->LoadShaderModule("./Shaders/textured_model.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        
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

        VkVertexInputAttributeDescription vertex_attribute_descriptions[5] = {};

        // Position
        vertex_attribute_descriptions[0].location = 0;
        vertex_attribute_descriptions[0].binding = vertex_binding_description.binding;
        vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_attribute_descriptions[0].offset = 0;

        // Normal
        vertex_attribute_descriptions[1].location = 1;
        vertex_attribute_descriptions[1].binding = vertex_binding_description.binding;
        vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_attribute_descriptions[1].offset = offsetof(struct VERTEX, normal);

        // UV
        vertex_attribute_descriptions[2].location = 2;
        vertex_attribute_descriptions[2].binding = vertex_binding_description.binding;
        vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_attribute_descriptions[2].offset = offsetof(struct VERTEX, texture_coordinates);

        // Bone Weights
        vertex_attribute_descriptions[3].location = 3;
        vertex_attribute_descriptions[3].binding = vertex_binding_description.binding;
        vertex_attribute_descriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertex_attribute_descriptions[3].offset = offsetof(struct VERTEX, bone_weights);

        // Bone IDs
        vertex_attribute_descriptions[4].location = 4;
        vertex_attribute_descriptions[4].binding = vertex_binding_description.binding;
        vertex_attribute_descriptions[4].format = VK_FORMAT_R32G32B32A32_SINT;
        vertex_attribute_descriptions[4].offset = offsetof(struct VERTEX, bone_ids);

        VkPipelineVertexInputStateCreateInfo vi;
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.pNext = nullptr;
        vi.flags = 0;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &vertex_binding_description;
        vi.vertexAttributeDescriptionCount = 5;
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
        rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

        VkPipelineDepthStencilStateCreateInfo ds;
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
        ds.front = ds.back;

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
        pipeline.layout = this->mesh_pipeline.layout;
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
        pipeline.pDepthStencilState = &ds;
        pipeline.pStages = shader_stages.data();
        pipeline.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipeline.renderPass = this->render_pass;
        pipeline.subpass = 0;

        result = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &this->mesh_pipeline.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreatePipeline => vkCreateGraphicsPipelines : Failed" << std::endl;
            #endif
            return false;
        }

        std::array<VkPipelineShaderStageCreateInfo, 3> base_shaders;
        base_shaders[0] = this->LoadShaderModule("./Shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        base_shaders[1] = this->LoadShaderModule("./Shaders/base.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        base_shaders[2] = this->LoadShaderModule("./Shaders/normal_debug.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT);
        pipeline.pStages = base_shaders.data();
        pipeline.stageCount = static_cast<uint32_t>(base_shaders.size());

        result = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &this->normals_pipeline.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreatePipeline => vkCreateGraphicsPipelines : Failed" << std::endl;
            #endif
            return false;
        }

        vkDestroyShaderModule(this->device, shader_stages[0].module, nullptr);
        vkDestroyShaderModule(this->device, shader_stages[1].module, nullptr);

        vkDestroyShaderModule(this->device, base_shaders[0].module, nullptr);
        vkDestroyShaderModule(this->device, base_shaders[1].module, nullptr);
        vkDestroyShaderModule(this->device, base_shaders[2].module, nullptr);

        #if defined(_DEBUG)
        std::cout << "CreatePipeline : Success" << std::endl;
        #endif
        return true;
    }

    bool Vulkan::CreateDepthBuffer()
    {
        //////////////////////////////////
        // Recherche d'un format adapté //
        //    pour le depth buffer      //
        //////////////////////////////////

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
			if(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) this->depth_buffer.format = format;
		}

        if(this->depth_buffer.format == VK_FORMAT_UNDEFINED) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => Format : VK_FORMAT_UNDEFINED" << std::endl;
            #endif
            return false;
        }

        /////////////////////////
        // Création de l'image //
        /////////////////////////

        VkImageCreateInfo image_info = {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = NULL;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = this->depth_buffer.format;
        image_info.extent.width = this->draw_surface.width;
        image_info.extent.height = this->draw_surface.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = NULL;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.flags = 0;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

        VkResult result = vkCreateImage(this->device, &image_info, NULL, &depth_buffer.image);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkCreateImage : Failed" << std::endl;
            #endif
            return false;
        }

        ///////////////////////////////////////////////////////
        //      Réservation d'un segment de mémoire          //
        // dans la carte graphique pour y placer la vue      //
        ///////////////////////////////////////////////////////

        VkMemoryRequirements image_memory_requirements;
        vkGetImageMemoryRequirements(this->device, depth_buffer.image, &image_memory_requirements);

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

        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &depth_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindImageMemory(this->device, depth_buffer.image, depth_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkBindImageMemory : Failed" << std::endl;
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
        view_info.format = this->depth_buffer.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.flags = 0;
        view_info.image = this->depth_buffer.image;

        result = vkCreateImageView(this->device, &view_info, nullptr, &depth_buffer.view);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkCreateImageView : Failed" << std::endl;
            #endif

            return false;
        }

        ///////////////////////////////////////////////////
        //      On change le layout de notre image       //
        // pour la préparer aux opérations du depth test //
        ///////////////////////////////////////////////////

        // On évite que plusieurs opérations aient lieu en même temps en utilisant une fence
        result = vkWaitForFences(this->device, 1, &this->background.graphics_command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateDepthBuffer => vkWaitForFences : Timeout" << std::endl;
            #endif
            return false;
        }
        vkResetFences(this->device, 1, &this->background.graphics_command_buffer.fence);

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(this->background.graphics_command_buffer.handle, &command_buffer_begin_info);

        VkImageMemoryBarrier image_memory_barrier = {};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = this->depth_buffer.image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(this->background.graphics_command_buffer.handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

        vkEndCommandBuffer(this->background.graphics_command_buffer.handle);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->background.graphics_command_buffer.handle;

        result = vkQueueSubmit(this->background.graphics_queue.handle, 1, &submit_info, this->background.graphics_command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkQueueSubmit : Failed" << std::endl;
            #endif
            return false;
        }

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
    bool Vulkan::CreateStagingBuffer(STAGING_BUFFER& staging_buffer, VkDeviceSize size)
    {
        staging_buffer.size = size;

        std::vector<uint32_t> shared_queue_families = {this->graphics_queue.index};

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = size;
        buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if(this->transfer_queue.index == this->graphics_queue.index) {
            buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }else{
            shared_queue_families.push_back(this->transfer_queue.index);
            buffer_create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        }

        buffer_create_info.queueFamilyIndexCount = static_cast<uint32_t>(shared_queue_families.size());
        buffer_create_info.pQueueFamilyIndices = &shared_queue_families[0];

        VkResult result = vkCreateBuffer(this->device, &buffer_create_info, nullptr, &staging_buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateStaginBuffer => vkCreateBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(this->device, staging_buffer.handle, &mem_reqs);

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
        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &staging_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateStaginBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindBufferMemory(this->device, staging_buffer.handle, staging_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateStaginBuffer => vkBindBufferMemory : Failed" << std::endl;
            #endif
            return false;
        }

        constexpr auto offset = 0;
        result = vkMapMemory(this->device, staging_buffer.memory, offset, size, 0, &staging_buffer.pointer);
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
     * Création d'un uniform buffer pour y mettre la matrice de projection
     */
    bool Vulkan::CreateUniformBuffer(UNIFORM_BUFFER& uniform_buffer, VkDeviceSize size)
    {
        ////////////////////////////////
        //     Création du buffer     //
        ////////////////////////////////

        uniform_buffer.size = size;

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = uniform_buffer.size;
        buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;

        VkResult result = vkCreateBuffer(this->device, &buffer_create_info, nullptr, &uniform_buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateUniformBuffer => vkCreateBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(this->device, uniform_buffer.handle, &mem_reqs);

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
        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &uniform_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateUniformBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return false;
        }

        result = vkBindBufferMemory(this->device, uniform_buffer.handle, uniform_buffer.memory, 0);
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
     * Création d'une liste de command buffers avec leur fence
     */
    bool Vulkan::CreateCommandBuffer(VkCommandPool pool, std::vector<COMMAND_BUFFER>& command_buffers, VkCommandBufferLevel level)
    {
        // Allocation des command buffers
        std::vector<VkCommandBuffer> output_buffers(command_buffers.size());
        if(!this->AllocateCommandBuffer(pool, static_cast<uint32_t>(command_buffers.size()), output_buffers, level)) {
            #if defined(_DEBUG)
            std::cout << "CreateCommandBuffer => AllocateCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }
        for(uint8_t i=0; i<command_buffers.size(); i++) command_buffers[i].handle = output_buffers[i];

        // Création des fences
        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = nullptr;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i=0; i<command_buffers.size(); i++) {

            VkResult result = vkCreateFence(this->device, &fence_create_info, nullptr, &command_buffers[i].fence);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateCommandBuffer => vkCreateFence : Failed" << std::endl;
                #endif
                return false;
            }
        }

        return true;
    }

    /**
     * Allocation des command buffers
     */
    bool Vulkan::AllocateCommandBuffer(VkCommandPool& pool, uint32_t count, std::vector<VkCommandBuffer>& buffer, VkCommandBufferLevel level)
    {
        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = pool;
        cmd.level = level;
        cmd.commandBufferCount = count;

        VkResult result = vkAllocateCommandBuffers(this->device, &cmd, buffer.data());
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
            #if defined(_DEBUG)
            std::cout << "CreateSemaphore => vkCreateSemaphore : Failed" << std::endl;
            #endif
            return false;
        }

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

        return true;
    }
    
    /**
     * Allocation des resources de la boucle principale
     */
    bool Vulkan::InitMainThread()
    {
        // Command Pool
        if(!this->CreateCommandPool(this->graphics_command_pool, this->graphics_queue.index)) return false;

        // On veut autant de ressources qu'il y a d'images dans la Swap Chain
        this->main.resize(this->concurrent_frame_count);

        // Semaphores
        for(uint32_t i=0; i<this->main.size(); i++) {
            if(!this->CreateSemaphore(this->main[i].renderpass_semaphore)) return false;
            if(!this->CreateSemaphore(this->main[i].swap_chain_semaphore)) return false;
        }

        // Command Buffers
        std::vector<COMMAND_BUFFER> graphics_buffer(this->concurrent_frame_count);
        if(!this->CreateCommandBuffer(this->graphics_command_pool, graphics_buffer)) return false;
        for(uint32_t i=0; i<this->main.size(); i++) this->main[i].graphics_command_buffer = graphics_buffer[i];

        // Si on a une queue dédiée eux transferts, on alloue des ressources en conséquence
        if(this->graphics_queue.index != this->transfer_queue.index) {

            // Command Pool
            if(!this->CreateCommandPool(this->transfer_command_pool, this->transfer_queue.index)) return false;

            // Command Buffers
            std::vector<COMMAND_BUFFER> transfer_buffer(this->concurrent_frame_count);
            if(!this->CreateCommandBuffer(this->transfer_command_pool, transfer_buffer)) return false;
            for(uint32_t i=0; i<this->main.size(); i++) this->main[i].transfer_command_buffer = transfer_buffer[i];

            // Semaphores
            for(uint32_t i=0; i<this->main.size(); i++) if(!this->CreateSemaphore(this->main[i].transfer_semaphore)) return false;
        }

        #if defined(_DEBUG)
        std::cout << "InitMainThread : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Allocation des ressources partagées par tous les threads
     */
    bool Vulkan::InitThreadSharedResources()
    {
        // Taille des uniform et staging buffers
        uint32_t count = 50;
        VkDeviceSize buffer_size = this->ubo_alignement * count;

        // On veut autant de ressources qu'il y a d'images dans la Swap Chain
        this->shared.resize(this->concurrent_frame_count);

        // On alloue les ressources pour chaque image
        for(uint32_t i=0; i<this->shared.size(); i++) {
            if(!this->CreateStagingBuffer(this->shared[i].staging, buffer_size)) return false;
            if(!this->CreateUniformBuffer(this->shared[i].uniform, buffer_size)) return false;
        }

        #if defined(_DEBUG)
        std::cout << "InitThreadSharedResources : Success" << std::endl;
        #endif

        return true;
    }

    /**
     * Allocation des ressources utilisées pour les tâches d'arrière plan,
     * telles que le chargement de textures ou de vertext buffers
     */
    bool Vulkan::InitBackgroundResources()
    {
        // Transfer Command Pool
        if(!this->CreateCommandPool(this->background.transfer_command_pool, this->background.transfer_queue.index)) return false;

        // Transfer Command Buffer
        std::vector<COMMAND_BUFFER> transfer_buffers(1);
        if(!this->CreateCommandBuffer(this->background.transfer_command_pool, transfer_buffers)) return false;
        this->background.transfer_command_buffer = transfer_buffers[0];
        transfer_buffers.clear();

        // Staging buffer
        if(!this->CreateStagingBuffer(this->background.staging, BACKGROUND_STAGING_BUFFER_SIZE)) return false;

        if(this->background.transfer_queue.index != this->background.graphics_queue.index) {
            // Graphics Command Pool
            if(!this->CreateCommandPool(this->background.graphics_command_pool, this->background.graphics_queue.index)) return false;

            // Graphics Command Buffer
            std::vector<COMMAND_BUFFER> graphics_buffers(1);
            if(!this->CreateCommandBuffer(this->background.graphics_command_pool, graphics_buffers)) return false;
            this->background.graphics_command_buffer = graphics_buffers[0];
            graphics_buffers.clear();

            // Semaphore
            if(!this->CreateSemaphore(this->background.semaphore)) return false;
        }else{
            
            this->background.graphics_command_buffer = this->background.transfer_command_buffer;
            this->background.graphics_queue = this->background.transfer_queue;
        }

        #if defined(_DEBUG)
        std::cout << "InitTextureCreationResources : Success" << std::endl;
        #endif
        
        return true;
    }

    /**
     * Libère les ressources utilisées lors de la création des textures
     */
    void Vulkan::ReleaseBackgroundResources()
    {
        // Semaphore
        if(this->background.semaphore != VK_NULL_HANDLE) vkDestroySemaphore(this->device, this->background.semaphore, nullptr);

        // Fence
        if(this->background.transfer_command_buffer.fence != VK_NULL_HANDLE) vkDestroyFence(this->device, this->background.transfer_command_buffer.fence, nullptr);

        // Command Pool
        if(this->background.transfer_command_pool != VK_NULL_HANDLE) vkDestroyCommandPool(this->device, this->background.transfer_command_pool, nullptr);

        if(this->background.transfer_queue.index != this->background.graphics_queue.index) {
            // Fence
            if(this->background.graphics_command_buffer.fence != VK_NULL_HANDLE) vkDestroyFence(this->device, this->background.graphics_command_buffer.fence, nullptr);
            
            // Command Pool
            if(this->background.graphics_command_pool != VK_NULL_HANDLE) vkDestroyCommandPool(this->device, this->background.graphics_command_pool, nullptr);
        }

        // Staging Buffer
        if(this->background.staging.pointer != nullptr) vkUnmapMemory(this->device, this->background.staging.memory);
        if(this->background.staging.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->background.staging.memory, nullptr);
        if(this->background.staging.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, this->background.staging.handle, nullptr);
    }

    /**
     * Libère les ressources de la boucle principale
     */
    void Vulkan::ReleaseMainThread()
    {
        for(uint32_t i=0; i<this->main.size(); i++) {

            // Frame Buffer
            if(this->main[i].framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(this->device, this->main[i].framebuffer, nullptr);

            // Semaphores
            if(this->main[i].renderpass_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(this->device, this->main[i].renderpass_semaphore, nullptr);
            if(this->main[i].swap_chain_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(this->device, this->main[i].swap_chain_semaphore, nullptr);
            if(this->main[i].transfer_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(this->device, this->main[i].transfer_semaphore, nullptr);

            // Fences
            if(this->main[i].graphics_command_buffer.fence != VK_NULL_HANDLE) vkDestroyFence(this->device, this->main[i].graphics_command_buffer.fence, nullptr);
            if(this->main[i].transfer_command_buffer.fence != VK_NULL_HANDLE) vkDestroyFence(this->device, this->main[i].transfer_command_buffer.fence, nullptr);
        }

        // Command Pool
        if(this->graphics_command_pool != VK_NULL_HANDLE) vkDestroyCommandPool(this->device, this->graphics_command_pool, nullptr);
        if(this->transfer_command_pool != VK_NULL_HANDLE) vkDestroyCommandPool(this->device, this->transfer_command_pool, nullptr);

        this->main.clear();
    }

    /**
     * Libère les ressources partagées
     */
    void Vulkan::ReleaseThreadSharedResources()
    {
        for(uint32_t i=0; i<this->shared.size(); i++) {

            // Uniform Buffer
            if(this->shared[i].uniform.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->shared[i].uniform.memory, nullptr);
            if(this->shared[i].uniform.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, this->shared[i].uniform.handle, nullptr);

            // Staging Buffer
            if(this->shared[i].staging.pointer != nullptr) vkUnmapMemory(this->device, this->shared[i].staging.memory);
            if(this->shared[i].staging.memory != VK_NULL_HANDLE) vkFreeMemory(this->device, this->shared[i].staging.memory, nullptr);
            if(this->shared[i].staging.handle != VK_NULL_HANDLE) vkDestroyBuffer(this->device, this->shared[i].staging.handle, nullptr);
        }

        this->shared.clear();
    }

    /**
    * Lorsque la fenêtre est redimensionnée la swap chain doit être reconstruite
    */
    bool Vulkan::OnWindowSizeChanged()
    {
        if(this->creating_swap_chain) return true;
        this->draw_surface = this->draw_window->GetSurface();
        return this->CreateSwapChain();
    }

    uint32_t Vulkan::CreateTexture(ColorRGB const& color)
    {
        Tools::IMAGE_MAP pixel;
        pixel.width = 1;
        pixel.height = 1;
        pixel.format = sizeof(ColorRGB);
        pixel.data = {color[0], color[1], color[2]};

        return this->CreateTexture(pixel);
    }

    uint32_t Vulkan::CreateTexture(Tools::IMAGE_MAP& image)
    {
        if(!image.height || !image.width) return UINT32_MAX;

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
        image_info.extent.width = image.width;
        image_info.extent.height = image.height;
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
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        
        VkResult result = vkCreateImage(this->device, &image_info, nullptr, &texture.image);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkCreateImage : Failed" << std::endl;
            #endif
            return UINT32_MAX;
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
            return UINT32_MAX;
        }

        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &texture.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkAllocateMemory : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        result = vkBindImageMemory(this->device, texture.image, texture.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkBindImageMemory : Failed" << std::endl;
            #endif
            return UINT32_MAX;
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

            return UINT32_MAX;
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
            return UINT32_MAX;
        }

        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        result = vkWaitForFences(this->device, 1, &this->background.transfer_command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkWaitForFences : Timeout" << std::endl;
            #endif
            return UINT32_MAX;
        }
        vkResetFences(this->device, 1, &this->background.transfer_command_buffer.fence);

        //////////////////////////////
        // Copie de la texture vers //
        //    le staging buffer     //
        //////////////////////////////

        size_t flush_size = static_cast<size_t>(image.data.size() * sizeof(image.data[0]));
        std::memcpy(this->background.staging.pointer, &image.data[0], flush_size);

        unsigned int multiple = static_cast<unsigned int>(flush_size / this->physical_device.properties.limits.nonCoherentAtomSize);
        flush_size = this->physical_device.properties.limits.nonCoherentAtomSize * (static_cast<uint64_t>(multiple) + 1);

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->background.staging.memory;
        flush_range.offset = 0;
        flush_range.size = flush_size;
        vkFlushMappedMemoryRanges(this->device, 1, &flush_range);

        //////////////////////////////////////
        //     Envoi de la texture vers     //
        // la mémoire de la carte graphique //
        //////////////////////////////////////

        // Préparation de la copie de l'image depuis le staging buffer vers un vertex buffer
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(this->background.transfer_command_buffer.handle, &command_buffer_begin_info);

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

        vkCmdPipelineBarrier(this->background.transfer_command_buffer.handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_undefined_to_transfer_dst);

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

        vkCmdCopyBufferToImage(this->background.transfer_command_buffer.handle, this->background.staging.handle, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy_info);

        if(this->background.transfer_queue.index == this->background.graphics_queue.index) {
            VkImageMemoryBarrier image_memory_barrier_from_transfer_to_shader_read = {};
            image_memory_barrier_from_transfer_to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_memory_barrier_from_transfer_to_shader_read.pNext = nullptr;
            image_memory_barrier_from_transfer_to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier_from_transfer_to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            image_memory_barrier_from_transfer_to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_memory_barrier_from_transfer_to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_memory_barrier_from_transfer_to_shader_read.srcQueueFamilyIndex = this->background.transfer_queue.index;
            image_memory_barrier_from_transfer_to_shader_read.dstQueueFamilyIndex = this->graphics_queue.index;
            image_memory_barrier_from_transfer_to_shader_read.image = texture.image;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.baseMipLevel = 0;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.levelCount = 1;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.baseArrayLayer = 0;
            image_memory_barrier_from_transfer_to_shader_read.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(this->background.transfer_command_buffer.handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_transfer_to_shader_read);
        }

        vkEndCommandBuffer(this->background.transfer_command_buffer.handle);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->background.transfer_command_buffer.handle;

        if(this->background.transfer_queue.index != this->background.graphics_queue.index) {
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &this->background.semaphore;
        }else{
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = nullptr;
        }

        result = vkQueueSubmit(this->background.transfer_queue.handle, 1, &submit_info, this->background.transfer_command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateTexture => vkQueueSubmit : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        if(this->background.transfer_queue.index != this->background.graphics_queue.index) {

            VkResult result = vkWaitForFences(this->device, 1, &this->background.graphics_command_buffer.fence, VK_FALSE, 1000000000);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateTexture => vkWaitForFences : Timeout" << std::endl;
                #endif
                return UINT32_MAX;
            }
            vkResetFences(this->device, 1, &this->background.graphics_command_buffer.fence);

            vkBeginCommandBuffer(this->background.graphics_command_buffer.handle, &command_buffer_begin_info);

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

            vkCmdPipelineBarrier(this->background.graphics_command_buffer.handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_transfer_to_shader_read);

            vkEndCommandBuffer(this->background.graphics_command_buffer.handle);

            VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = nullptr;
            submit_info.pCommandBuffers = &this->background.graphics_command_buffer.handle;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &this->background.semaphore;
            submit_info.pWaitDstStageMask = &wait_dst_stage_mask;

            result = vkQueueSubmit(this->background.graphics_queue.handle, 1, &submit_info, this->background.graphics_command_buffer.fence);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateTexture => vkQueueSubmit(2) : Failed" << std::endl;
                #endif
                return UINT32_MAX;
            }
        }
        
        ////////////////////////////////////
        // Allocation des descriptor Sets //
        ////////////////////////////////////

        std::vector<VkDescriptorSetLayout> layouts(this->concurrent_frame_count, this->descriptor_set_layout);
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = this->descriptor_pool;
        alloc_info.descriptorSetCount = this->concurrent_frame_count;
        alloc_info.pSetLayouts = &layouts[0];
        
        texture.descriptors.resize(this->concurrent_frame_count);
        result = vkAllocateDescriptorSets(this->device, &alloc_info, &texture.descriptors[0]);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout <<"CreateModel => vkAllocateDescriptorSets : Failed" << std::endl;
            #endif
            return false;
        }

        /////////////////////////////////////
        // Mise à jour des descriptor Sets //
        /////////////////////////////////////

        for(uint32_t i=0; i<texture.descriptors.size(); i++) {

            VkWriteDescriptorSet writes[2];

            VkDescriptorImageInfo descriptor_image_info = {};
            descriptor_image_info.sampler = texture.sampler;
            descriptor_image_info.imageView = texture.view;
            descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].pNext = nullptr;
            writes[0].dstSet = texture.descriptors[i];
            writes[0].dstBinding = 0;
            writes[0].dstArrayElement = 0;
            writes[0].descriptorCount = 1;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].pImageInfo = &descriptor_image_info;
            writes[0].pBufferInfo = nullptr;
            writes[0].pTexelBufferView = nullptr;

            VkDescriptorBufferInfo buffer_info = {};
            buffer_info.buffer = this->shared[i].uniform.handle;
            buffer_info.offset = 0;
            buffer_info.range = static_cast<VkDeviceSize>(sizeof(UBO));

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].pNext = nullptr;
            writes[1].dstSet = texture.descriptors[i];
            writes[1].dstBinding = 1;
            writes[1].dstArrayElement = 0;
            writes[1].descriptorCount = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            writes[1].pImageInfo = nullptr;
            writes[1].pBufferInfo = &buffer_info;
            writes[1].pTexelBufferView = nullptr;
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

            vkUpdateDescriptorSets(this->device, 2, writes, 0, nullptr);
        }

        #if defined(_DEBUG)
        std::cout << "CreateTexture : Success" << std::endl;
        #endif

        // Ajout de la texture dans la mémoire
        uint32_t texture_index = this->last_texture_index;
        this->textures[texture_index] = texture;
        this->last_texture_index++;

        // On renvoie l'indice de la texture dans le 
        return texture_index;
    }

    /**
     * Création du vertex buffer
     */
    uint32_t Vulkan::CreateVertexBuffer(std::vector<Vulkan::VERTEX>& data)
    {
        VERTEX_BUFFER vertex_buffer;

        vertex_buffer.size = sizeof(data[0]) * data.size();
        vertex_buffer.count = static_cast<uint32_t>(data.size());

        ////////////////////////////////
        //     Création du buffer     //
        ////////////////////////////////

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
            return UINT32_MAX;
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
            return UINT32_MAX;
        }

        // Allocation de la mémoire pour le vertex buffer
        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &vertex_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        result = vkBindBufferMemory(this->device, vertex_buffer.handle, vertex_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => vkBindBufferMemory : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        result = vkWaitForFences(this->device, 1, &this->background.transfer_command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => vkWaitForFences : Timeout" << std::endl;
            #endif
            return UINT32_MAX;
        }
        vkResetFences(this->device, 1, &this->background.transfer_command_buffer.fence);

        ////////////////////////////////
        //   Copie des données vers   //
        //      le staging buffer     //
        ////////////////////////////////

        size_t flush_size = static_cast<size_t>(vertex_buffer.size);
        unsigned int multiple = static_cast<unsigned int>(flush_size / this->physical_device.properties.limits.nonCoherentAtomSize);
        flush_size = this->physical_device.properties.limits.nonCoherentAtomSize * (static_cast<uint64_t>(multiple) + 1);

        std::memcpy(this->background.staging.pointer, data.data(), vertex_buffer.size);

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->background.staging.memory;
        flush_range.offset = 0;
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

        vkBeginCommandBuffer(this->background.transfer_command_buffer.handle, &command_buffer_begin_info);

        VkBufferCopy buffer_copy_info = {};
        buffer_copy_info.srcOffset = 0;
        buffer_copy_info.dstOffset = 0;
        buffer_copy_info.size = vertex_buffer.size;

        vkCmdCopyBuffer(this->background.transfer_command_buffer.handle, this->background.staging.handle, vertex_buffer.handle, 1, &buffer_copy_info);

        VkBufferMemoryBarrier buffer_memory_barrier = {};
        buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.pNext = nullptr;
        buffer_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        buffer_memory_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        buffer_memory_barrier.srcQueueFamilyIndex = this->background.transfer_queue.index;
        buffer_memory_barrier.dstQueueFamilyIndex = this->background.graphics_queue.index;
        buffer_memory_barrier.buffer = vertex_buffer.handle;
        buffer_memory_barrier.offset = 0;
        buffer_memory_barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(this->background.transfer_command_buffer.handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        
        vkEndCommandBuffer(this->background.transfer_command_buffer.handle);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->background.transfer_command_buffer.handle;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->background.transfer_queue.handle, 1, &submit_info, this->background.transfer_command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => vkQueueSubmit : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }
        
        uint32_t buffer_index = this->last_vbo_index;
        this->vertex_buffers[buffer_index] = vertex_buffer;
        this->last_vbo_index++;

        return buffer_index;
    }

    uint32_t Vulkan::CreateIndexBuffer(std::vector<uint32_t>& data)
    {
        // Valeur de retour
        INDEX_BUFFER index_buffer;
        index_buffer.size = sizeof(uint32_t) * data.size();
        index_buffer.count = static_cast<uint32_t>(data.size());

        ////////////////////////////////
        //     Création du buffer     //
        ////////////////////////////////

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = nullptr;
        buffer_create_info.flags = 0;
        buffer_create_info.size = index_buffer.size;
        buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = nullptr;

        VkResult result = vkCreateBuffer(this->device, &buffer_create_info, nullptr, &index_buffer.handle);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateIndexBuffer => vkCreateBuffer : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(this->device, index_buffer.handle, &mem_reqs);

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;
        mem_alloc.allocationSize = mem_reqs.size;
        
        if(!this->MemoryTypeFromProperties(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_alloc.memoryTypeIndex)) {
            #if defined(_DEBUG)
            std::cout << "CreateIndexBuffer => MemoryType VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : Not found" << std::endl;
            #endif
            return UINT32_MAX;
        }

        // Allocation de la mémoire pour le vertex buffer
        result = vkAllocateMemory(this->device, &mem_alloc, nullptr, &index_buffer.memory);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateIndexBuffer => vkAllocateMemory : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        result = vkBindBufferMemory(this->device, index_buffer.handle, index_buffer.memory, 0);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateIndexBuffer => vkBindBufferMemory : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
        result = vkWaitForFences(this->device, 1, &this->background.transfer_command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateVertexBuffer => vkWaitForFences : Timeout" << std::endl;
            #endif
            return UINT32_MAX;
        }
        vkResetFences(this->device, 1, &this->background.transfer_command_buffer.fence);

        ////////////////////////////////
        //   Copie des données vers   //
        //      le staging buffer     //
        ////////////////////////////////

        size_t flush_size = static_cast<size_t>(index_buffer.size);
        unsigned int multiple = static_cast<unsigned int>(flush_size / this->physical_device.properties.limits.nonCoherentAtomSize);
        flush_size = this->physical_device.properties.limits.nonCoherentAtomSize * (static_cast<uint64_t>(multiple) + 1);

        std::memcpy(this->background.staging.pointer, &data[0], index_buffer.size);

        // Flush staging buffer
        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->background.staging.memory;
        flush_range.offset = 0;
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

        vkBeginCommandBuffer(this->background.transfer_command_buffer.handle, &command_buffer_begin_info);

        VkBufferCopy buffer_copy_info = {};
        buffer_copy_info.srcOffset = 0;
        buffer_copy_info.dstOffset = 0;
        buffer_copy_info.size = index_buffer.size;

        vkCmdCopyBuffer(this->background.transfer_command_buffer.handle, this->background.staging.handle, index_buffer.handle, 1, &buffer_copy_info);

        VkBufferMemoryBarrier buffer_memory_barrier = {};
        buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.pNext = nullptr;
        buffer_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        buffer_memory_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        buffer_memory_barrier.srcQueueFamilyIndex = this->background.transfer_queue.index;
        buffer_memory_barrier.dstQueueFamilyIndex = this->background.graphics_queue.index;
        buffer_memory_barrier.buffer = index_buffer.handle;
        buffer_memory_barrier.offset = 0;
        buffer_memory_barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(this->background.transfer_command_buffer.handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        
        vkEndCommandBuffer(this->background.transfer_command_buffer.handle);

        // Exécution de la commande de copie
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &this->background.transfer_command_buffer.handle;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(this->background.transfer_queue.handle, 1, &submit_info, this->background.transfer_command_buffer.fence);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "CreateIndexBuffer => vkQueueSubmit : Failed" << std::endl;
            #endif
            return UINT32_MAX;
        }

        uint32_t buffer_index = this->last_ibo_index;
        this->index_buffers[buffer_index] = index_buffer;
        this->last_ibo_index++;

        return buffer_index;
    }

    /**
     * Création du descriptor pool pour objet texturé
     */
    bool Vulkan::PrepareTextureDescriptorSets()
    {
        uint32_t pool_limit = MAX_CONCURRENT_TEXTURES * this->concurrent_frame_count;

        VkDescriptorPoolSize pool_sizes[2];
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[0].descriptorCount = pool_limit;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pool_sizes[1].descriptorCount = pool_limit;

        VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = nullptr;
        descriptor_pool_create_info.maxSets = pool_limit;
        descriptor_pool_create_info.poolSizeCount = 2;
        descriptor_pool_create_info.pPoolSizes = pool_sizes;

        VkResult result = vkCreateDescriptorPool(this->device, &descriptor_pool_create_info, nullptr, &this->descriptor_pool);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "PrepareTextureDescriptorSets => vkCreateDescriptorPool : Failed" << std::endl;
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
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
        layout_bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
        descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout.pNext = nullptr;
        descriptor_layout.bindingCount = 2;
        descriptor_layout.pBindings = layout_bindings;

        result = vkCreateDescriptorSetLayout(this->device, &descriptor_layout, nullptr, &this->descriptor_set_layout);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "PrepareTextureDescriptorSets => vkCreateDescriptorSetLayout : Failed" << std::endl;
            #endif
            return false;
        }
        
        // Succès
        #if defined(_DEBUG)
        std::cout << "PrepareTextureDescriptorSets : Success" << std::endl;
        #endif
        return true;
    }

    /**
     * Positionne toutes les valeurs des bones d'une entité sur la transformation de repos
     */
    void Vulkan::InitializeBones(UBO& ubo, BONE& tree, uint32_t vertex_buffer_id, Matrix4x4& parent_transformation)
    {
        Matrix4x4 bone_transfromation = parent_transformation * tree.transformation;

        if(tree.offset.count(vertex_buffer_id) && tree.index < ubo.bones.size())
            ubo.bones[tree.index] = bone_transfromation * tree.offset[vertex_buffer_id];

        for(BONE& child : tree.children) this->InitializeBones(ubo, child, vertex_buffer_id, bone_transfromation);
    }

    void Vulkan::EvalBones(UBO& ubo, BONE& tree, uint32_t vertex_buffer_id, Matrix4x4& parent_transformation, std::chrono::milliseconds& time, std::string const& animation)
    {
        Matrix4x4 bone_transfromation;
        if(tree.animation.count(animation)) {
            Vector3 translation = this->EvalInterpolation(tree.animation[animation].translations, time);
            Vector3 scaling = this->EvalInterpolation(tree.animation[animation].scalings, time, {std::chrono::milliseconds(0), {1,1,1}});
            Vector3 rotation = this->EvalInterpolation(tree.animation[animation].rotations, time) * DEGREES_TO_RADIANS;
            Matrix4x4 anim_transformation = TranslationMatrix(translation) * EulerRotation(IDENTITY_MATRIX, rotation, EULER_ORDER::ZYX) * ScalingMatrix(scaling);
            bone_transfromation = parent_transformation * anim_transformation;
        }else{
            bone_transfromation = parent_transformation * tree.transformation;
        }

        if(tree.offset.count(vertex_buffer_id) && tree.index < ubo.bones.size())
            ubo.bones[tree.index] = bone_transfromation * tree.offset[vertex_buffer_id];

        for(BONE& child : tree.children) this->EvalBones(ubo, child, vertex_buffer_id, bone_transfromation, time, animation);
    }

    Vector3 Vulkan::EvalInterpolation(std::vector<VECTOR_KEYFRAME> const& keyframes, std::chrono::milliseconds const& time, VECTOR_KEYFRAME const& base)
    {
        if(keyframes.size() == 0) return base.value;

        uint32_t source_key = UINT32_MAX;
        uint32_t dest_key = UINT32_MAX;

        for(uint32_t i=0; i<keyframes.size(); i++) {
            auto& keyframe = keyframes[i];
            if(keyframe.time <= time) {
                source_key = i;
            } else {
                dest_key = i;
                break;
            }
        }

        if(dest_key == UINT32_MAX) {

            VECTOR_KEYFRAME const& last_keyframe = keyframes[keyframes.size() - 1];
            return last_keyframe.value;

        }else{

            VECTOR_KEYFRAME const& source_keyframe = (source_key == UINT32_MAX) ? base : keyframes[source_key];
            VECTOR_KEYFRAME const& dest_keyframe = keyframes[dest_key];

            std::chrono::milliseconds current_key_duration = dest_keyframe.time - source_keyframe.time;
            std::chrono::milliseconds key_progression = time - source_keyframe.time;
            float ratio = static_cast<float>(key_progression.count()) / static_cast<float>(current_key_duration.count());

            return Interpolate(source_keyframe.value, dest_keyframe.value, ratio);
        }
    }

    void Vulkan::UpdateSkeleton(uint32_t mesh_id, std::chrono::milliseconds& time, std::string const& animation)
    {
        MESH& mesh = this->meshes[mesh_id];
        Matrix4x4 identity = IDENTITY_MATRIX;

        for(auto& resource : this->shared) {
            ENTITY& entity = resource.entities[mesh_id];
            this->EvalBones(entity.ubo, this->bone_trees[mesh.bone_tree_id], mesh.vertex_buffer_id, identity, time, animation);
        }
    }

    /**
     * Création d'un Mesh
     */
    uint32_t Vulkan::CreateMesh(MESH& mesh)
    {
        uint32_t mesh_index = this->last_mesh_index;

        this->meshes[mesh_index] = mesh;
        this->last_mesh_index++;

        // Création d'une entité à afficher
        for(uint32_t i=0; i<this->concurrent_frame_count; i++) {
            ENTITY entity;
            entity.vertex_buffer = this->vertex_buffers[mesh.vertex_buffer_id].handle;
            entity.vertex_count = this->vertex_buffers[mesh.vertex_buffer_id].count;
            entity.index_buffer = this->index_buffers[mesh.index_buffer_id].handle;
            entity.index_count = this->index_buffers[mesh.index_buffer_id].count;
            entity.descriptor_set = this->textures[mesh.texture_id].descriptors[i];
            entity.ubo_offset = mesh_index * this->ubo_alignement;
            entity.ubo = {};
            entity.ubo.projection = this->base_projection;
            entity.ubo.bone_id = mesh.attached_bone_id;

            Matrix4x4 identity = IDENTITY_MATRIX;
            this->InitializeBones(entity.ubo, this->bone_trees[mesh.bone_tree_id], mesh.vertex_buffer_id, identity);

            this->shared[i].entities[mesh_index] = entity;
        }

        return mesh_index;
    }

    /**
     * Mise à jour des données du uniform buffer
     */
    bool Vulkan::UpdateEntities(uint32_t frame_index)
    {
        // On récupère le commande buffer à utiliser pour les opérations de transfert
        COMMAND_BUFFER& transfer = (this->transfer_queue.index == this->graphics_queue.index) ? this->main[frame_index].graphics_command_buffer : this->main[frame_index].transfer_command_buffer;

        if(this->transfer_queue.index != this->graphics_queue.index) {

            // On évite que plusieurs transferts aient lieu en même temps en utilisant une fence
            VkResult result = vkWaitForFences(this->device, 1, &transfer.fence, VK_FALSE, 1000000000);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "UpdateEntities => vkWaitForFences : Timeout" << std::endl;
                #endif
                return false;
            }
            vkResetFences(this->device, 1, &transfer.fence);
        }

        //////////////////////////////
        //       flush du           //
        //     staging buffer       //
        //////////////////////////////

        uint32_t entity_count = static_cast<uint32_t>(this->shared[frame_index].entities.size());
        size_t flush_size = static_cast<size_t>(this->ubo_alignement * entity_count);
        unsigned int multiple = static_cast<unsigned int>(flush_size / this->physical_device.properties.limits.nonCoherentAtomSize);
        flush_size = this->physical_device.properties.limits.nonCoherentAtomSize * (static_cast<uint64_t>(multiple) + 1);

        // Écriture des données dans le staging buffer
        /*for(uint32_t i=0; i<this->shared[frame_index].entities.size(); i++)
            std::memcpy(reinterpret_cast<char*>(this->shared[frame_index].staging.pointer) + this->shared[frame_index].entities[i].ubo_offset, &this->shared[frame_index].entities[i].transformations, sizeof(Matrix4x4));*/

        VkMappedMemoryRange flush_range = {};
        flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range.pNext = nullptr;
        flush_range.memory = this->shared[frame_index].staging.memory;
        flush_range.offset = 0;
        flush_range.size = flush_size;
        vkFlushMappedMemoryRanges(this->device, 1, &flush_range);

        ///////////////////////////////////////////
        //        Copie du staging buffer        //
        //        vers le uniform buffer         //
        ///////////////////////////////////////////

        if(this->transfer_queue.index != this->graphics_queue.index) {
            VkCommandBufferBeginInfo command_buffer_begin_info = {};
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.pNext = nullptr; 
            command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            command_buffer_begin_info.pInheritanceInfo = nullptr;

            vkBeginCommandBuffer(transfer.handle, &command_buffer_begin_info);
        }

        VkBufferCopy buffer_copy_info = {};
        buffer_copy_info.srcOffset = 0;
        buffer_copy_info.dstOffset = 0;
        buffer_copy_info.size = flush_size;

        vkCmdCopyBuffer(transfer.handle, this->shared[frame_index].staging.handle, this->shared[frame_index].uniform.handle, 1, &buffer_copy_info);

        if(this->transfer_queue.index != this->graphics_queue.index) {

            vkEndCommandBuffer(transfer.handle);

            // Exécution de la commande de copie
            VkSubmitInfo submit_info = {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = nullptr;
            submit_info.waitSemaphoreCount = 0;
            submit_info.pWaitSemaphores = nullptr;
            submit_info.pWaitDstStageMask = nullptr;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &transfer.handle;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &this->main[frame_index].transfer_semaphore;

            VkResult result = vkQueueSubmit(this->transfer_queue.handle, 1, &submit_info, transfer.fence);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "UpdateEntities => vkQueueSubmit : Failed" << std::endl;
                #endif
                return false;
            }
        }

        return true;
    }

    /**
     * Initilisation des threads
     */
    bool Vulkan::InitThreads()
    {
        this->thread_count = std::thread::hardware_concurrency();
        // this->thread_count = 1;

        #if defined(_DEBUG)
        std::cout << "Nombre de threads disponibles : " << this->thread_count << std::endl;
        #endif

        this->threads.resize(this->thread_count);
        this->keep_thread_alive = true;

        for(uint8_t i=0; i<this->thread_count; i++) {
            
            // Création du command pool pour le thread
            if(!this->CreateCommandPool(this->threads[i].command_pool, this->graphics_queue.index)) {
                #if defined(_DEBUG)
                std::cout << "InitThreads => CreateCommandPool[" << i << "] : Failed" << std::endl;
                #endif
                return false;
            }

            // Allocation des command buffers secondaires pour le thread
            this->threads[i].graphics_command_buffer.resize(this->concurrent_frame_count);
            if(!this->CreateCommandBuffer(this->threads[i].command_pool, this->threads[i].graphics_command_buffer, VK_COMMAND_BUFFER_LEVEL_SECONDARY)) {
                #if defined(_DEBUG)
                std::cout << "InitThreads => CreateCommandBuffers[" << i << "] : Failed" << std::endl;
                #endif
                return false;
            }

            // Démarrage du thread
            this->threads[i].handle = std::thread{ThreadJob, i};
        }

        return true;
    }

    /**
     * Arrête les threads et libère les ressources associées
     */
    void Vulkan::ReleaseThreads()
    {
        this->keep_thread_alive = false;
        this->thread_sleep.signaled = true;
        this->thread_sleep.condition.notify_all();

        for(uint8_t i=0; i<this->thread_count; i++) {
            
            // Fin du thread
            this->threads[i].handle.join();

            // Command Pool
            if(this->threads[i].command_pool != VK_NULL_HANDLE) vkDestroyCommandPool(this->device, this->threads[i].command_pool, nullptr);

            // Fences
            for(uint32_t j=0; j<this->threads[i].graphics_command_buffer.size(); j++)
                if(this->threads[i].graphics_command_buffer[j].fence != VK_NULL_HANDLE)
                    vkDestroyFence(this->device, this->threads[i].graphics_command_buffer[j].fence, nullptr);
        }
    }

    /**
     * Création des frame buffers
     */
    bool Vulkan::CreateFrameBuffers()
    {
        for(uint32_t i=0; i<this->concurrent_frame_count; i++) {

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

            VkResult result = vkCreateFramebuffer(this->device, &framebuffer_create_info, nullptr, &this->main[i].framebuffer);
            if(result != VK_SUCCESS) {
                #if defined(_DEBUG)
                std::cout << "CreateFrameBuffers => vkCreateFramebuffer[" << i << "] : Failed" << std::endl;
                #endif
                return false;
            }
        }

        #if defined(_DEBUG)
        std::cout << "CreateFrameBuffers : Success" << std::endl;
        #endif

        return true;
    }

    uint32_t Vulkan::CreateBoneTree(BONE& tree)
    {
        uint32_t bone_tree_index = this->last_bone_tree_index;
        this->bone_trees[bone_tree_index] = tree;
        this->last_bone_tree_index++;
        return bone_tree_index;
    }

    /**
     * Affichage
     */
    void Vulkan::Draw()
    {
        //////////////////////////////
        //     Rotation du pool     //
        //  de ressources de rendu  //
        //////////////////////////////

        this->current_frame_index = (this->current_frame_index + 1) % static_cast<uint8_t>(this->concurrent_frame_count);
        auto current_rendering_resource = this->main[this->current_frame_index];

        // S'il n'y a rien à afficher, on sort...
        if(this->shared[this->current_frame_index].entities.size() == 0) return;

        ////////////////////////////////
        //    Attente de libération   //
        //   des resources de rendu   //
        ////////////////////////////////

        VkResult result = vkWaitForFences(this->device, 1, &current_rendering_resource.graphics_command_buffer.fence, VK_FALSE, 1000000000);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "Draw => vkWaitForFences : Timeout" << std::endl;
            #endif
            return;
        }
        vkResetFences(this->device, 1, &current_rendering_resource.graphics_command_buffer.fence);

        ////////////////////////////////////
        //    Récupération d'une image    //
        //       de la Swap Chain         //
        ////////////////////////////////////

        uint32_t swap_chain_image_index;
        result = vkAcquireNextImageKHR(this->device, this->swap_chain.handle, UINT64_MAX, current_rendering_resource.swap_chain_semaphore, VK_NULL_HANDLE, &swap_chain_image_index);
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
                this->OnWindowSizeChanged();
            default:
                #if defined(_DEBUG)
                std::cout << "Draw => vkAcquireNextImageKHR : Failed" << std::endl;
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

        VkCommandBuffer command_buffer = current_rendering_resource.graphics_command_buffer.handle;
        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

        VkImageSubresourceRange image_subresource_range = {};
        image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_subresource_range.baseMipLevel = 0;
        image_subresource_range.levelCount = 1;
        image_subresource_range.baseArrayLayer = 0;
        image_subresource_range.layerCount = 1;

        uint32_t present_queue_family_index;
        uint32_t graphics_queue_family_index;
        if(this->graphics_queue.index == this->present_queue.index) {
            present_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
            graphics_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
        }else{
            present_queue_family_index = this->present_queue.index;
            graphics_queue_family_index = this->graphics_queue.index;
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
        barrier_from_present_to_draw.image = this->swap_chain.images[swap_chain_image_index].handle;
        barrier_from_present_to_draw.subresourceRange = image_subresource_range;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_present_to_draw);

        std::array<VkClearValue, 2> clear_value = {};
        clear_value[0].color = { 0.1f, 0.2f, 0.3f, 0.0f };
        clear_value[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = this->render_pass;
        render_pass_begin_info.framebuffer = this->main[this->current_frame_index].framebuffer;
        render_pass_begin_info.renderArea.offset.x = 0;
        render_pass_begin_info.renderArea.offset.y = 0;
        render_pass_begin_info.renderArea.extent.width = this->draw_surface.width;
        render_pass_begin_info.renderArea.extent.height = this->draw_surface.height;
        render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_value.size());
        render_pass_begin_info.pClearValues = clear_value.data();

        // On prépare les threads pour le travail
        std::unique_lock<std::mutex> sleep_lock(this->thread_sleep.mutex);

        this->pick_index = 0;
        this->thread_sleep.signaled = true;
        this->job_done_count = 0;

        static auto animation_start = std::chrono::system_clock::now();
        static auto animation_total_duration = std::chrono::milliseconds(15000);
        auto now = std::chrono::system_clock::now();
        auto animation_current_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - animation_start);
        if(animation_current_duration > animation_total_duration) animation_start = now;
        float ratio = static_cast<float>(animation_current_duration.count()) / static_cast<float>(animation_total_duration.count());

        static float angle_y = 0;
        static float angle_x = 0;
        static float translator = 0;

        angle_y = 360.0f * ratio;
        angle_x += 0.01f;

        // Transformations
        // for(uint32_t i=0; i<this->shared[this->current_frame_index].entities.size(); i++) {
        for(auto& entity : this->shared[this->current_frame_index].entities) {

            // grom-hellscream
            // entity.second.ubo.model = TranslationMatrix(0.0f, 2.2f, -5.0f) * RotationMatrix(angle_y, {0.0f, 1.0f, 0.0f}) * RotationMatrix(180.0f, {1.0f, 0.0f, 0.0f}) * ScalingMatrix(scale, scale, scale);
            /*if(entity.first != 0) entity.second.ubo.model = TranslationMatrix(0.0f, -2.0f, 0.0f) * RotationMatrix(180.0f, {1.0f, 0.0f, 0.0f}) * ScalingMatrix(0.01f, 0.01f, 0.01f);
            else entity.second.ubo.model = ScalingMatrix(0.1f, 0.1f, 0.1f);
            entity.second.ubo.view = camera.GetViewMatrix();*/

            // wolf
            // entity.second.ubo.model = TranslationMatrix(0.0f, 0.25f, -1.0f) * RotationMatrix(angle_y, {0.0f, 1.0f, 0.0f}) * RotationMatrix(180.0f, {1.0f, 0.0f, 0.0f}) * ScalingMatrix(scale, scale, scale);

            // Chevalier
            if(entity.first != 0) entity.second.ubo.model = /*RotationMatrix(angle_y, {0.0f, 1.0f, 0.0f}) */ RotationMatrix(-90.0f, {1.0f, 0.0f, 0.0f}) * ScalingMatrix(0.1f, 0.1f, 0.1f);
            else entity.second.ubo.model = ScalingMatrix(0.1f, 0.1f, 0.1f);
            entity.second.ubo.view = camera.GetViewMatrix();

            // Cerberus
            /*if(entity.first != 0) entity.second.ubo.model = TranslationMatrix(0.0f, -2.0f, 0.0f) * RotationMatrix(-90.0f, {1.0f, 0.0f, 0.0f}) * ScalingMatrix(scale, scale, scale);
            else entity.second.ubo.model = ScalingMatrix(0.1f, 0.1f, 0.1f);
            entity.second.ubo.view = camera.GetViewMatrix();*/

            // Cube
            /*if(entity.first != 0) entity.second.ubo.model = TranslationMatrix(0.0f, -2.0f, 0.0f);
            else entity.second.ubo.model = ScalingMatrix(0.1f, 0.1f, 0.1f);
            entity.second.ubo.view = camera.GetViewMatrix();*/

            std::memset(reinterpret_cast<char*>(this->shared[this->current_frame_index].staging.pointer) + entity.second.ubo_offset, 0, this->ubo_alignement);
            std::memcpy(reinterpret_cast<char*>(this->shared[this->current_frame_index].staging.pointer) + entity.second.ubo_offset, &entity.second.ubo, sizeof(UBO));
        }

        // On déclenche le travail des threads
        std::unique_lock<std::mutex> frame_ready_lock(this->frame_ready.mutex);
        this->thread_sleep.condition.notify_all();

        sleep_lock.unlock();

        // On attend la fin du travail
        while(!this->frame_ready.signaled) this->frame_ready.condition.wait(frame_ready_lock);
        this->frame_ready.signaled = false;

        // On envoie le uniform buffer dans la carte graphique
        this->UpdateEntities(this->current_frame_index);

        // Début de la render pass
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        // On ferme les command buffers secondaires, et on exécute les commandes de rendu générées par les threads
        for(uint32_t i=0; i<this->thread_count; i++) {
            THREAD_HANDLER* thread = reinterpret_cast<THREAD_HANDLER*>(&this->threads[i]);
            if(thread->graphics_command_buffer[this->current_frame_index].opened) {
                result = vkEndCommandBuffer(thread->graphics_command_buffer[this->current_frame_index].handle);
                if(result != VK_SUCCESS) {
                    #if defined(_DEBUG)
                    std::cout << "Draw => Thread[" << i << "]->vkEndCommandBuffer : Failed" << std::endl;
                    #endif
                    return;
                }

                vkCmdExecuteCommands(command_buffer, 1, &thread->graphics_command_buffer[this->current_frame_index].handle);
                thread->graphics_command_buffer[this->current_frame_index].opened = false;
            }
        }

        // Fin de la render pass
        vkCmdEndRenderPass(command_buffer);

        VkImageMemoryBarrier barrier_from_draw_to_present;
        barrier_from_draw_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier_from_draw_to_present.pNext = nullptr;
        barrier_from_draw_to_present.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier_from_draw_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier_from_draw_to_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier_from_draw_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier_from_draw_to_present.srcQueueFamilyIndex = graphics_queue_family_index;
        barrier_from_draw_to_present.dstQueueFamilyIndex = present_queue_family_index;
        barrier_from_draw_to_present.image = this->swap_chain.images[swap_chain_image_index].handle;
        barrier_from_draw_to_present.subresourceRange = image_subresource_range;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_draw_to_present);

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(_DEBUG)
            std::cout << "Draw => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return;
        }

        VkSemaphore semaphores[2];
        semaphores[0] = current_rendering_resource.swap_chain_semaphore;

        VkPipelineStageFlags wait_dst_stage_mask[2];
        wait_dst_stage_mask[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pCommandBuffers = &current_rendering_resource.graphics_command_buffer.handle;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &current_rendering_resource.renderpass_semaphore;
        submit_info.pWaitSemaphores = semaphores;
        submit_info.pWaitDstStageMask = wait_dst_stage_mask;

        if(this->transfer_queue.index != this->present_queue.index) {
            submit_info.waitSemaphoreCount = 2;
            semaphores[1] = current_rendering_resource.transfer_semaphore;
            wait_dst_stage_mask[1] = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }

        result = vkQueueSubmit(this->graphics_queue.handle, 1, &submit_info, current_rendering_resource.graphics_command_buffer.fence);
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
        present_info.pWaitSemaphores = &current_rendering_resource.renderpass_semaphore;
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
                break;
            default:
                #if defined(_DEBUG)
                std::cout << "Draw => vkQueuePresentKHR : Failed" << std::endl;
                #endif
                return;
        }
    }

    /**
     * Traitements exécutés par les threads
     */
    void Vulkan::ThreadJob(uint32_t thread_id)
    {
        Vulkan* self = Vulkan::vulkan;
        THREAD_HANDLER* thread = reinterpret_cast<THREAD_HANDLER*>(&self->threads[thread_id]);

        while(self->keep_thread_alive) {

            // Le thread attend qu'on lui donne du travail
            std::unique_lock<std::mutex> sleep_lock(self->thread_sleep.mutex);
            while(!self->thread_sleep.signaled) self->thread_sleep.condition.wait(sleep_lock);
            sleep_lock.unlock();

            // Condition de sortie
            if(!self->keep_thread_alive) return;

            while(true) {

                // On bloque l'accès à la file de travail
                std::unique_lock<std::mutex> pick_lock(self->pick_work);

                // Frame en course de traitement et nombre d'intités à afficher
                uint32_t frameid = self->current_frame_index;
                uint32_t entity_count = static_cast<uint32_t>(self->shared[frameid].entities.size());

                // Il n'y a pas de travail, on repart en sommeil
                if(self->pick_index >= entity_count) {
                    self->thread_sleep.signaled = false;
                    pick_lock.unlock();
                    break;
                }
                
                // On récupère une entité à traiter et on incrémente le compteur de la file de travail
                uint32_t entity_index = self->pick_index;
                ENTITY& entity = self->shared[frameid].entities[entity_index];
                self->pick_index++;

                COMMAND_BUFFER& command_buffer = thread->graphics_command_buffer[frameid];

                // On retire le lock pour que les autres threads récupèrent du travail
                pick_lock.unlock();

                ////////////////////////
                // Traitement du mesh //
                ////////////////////////

                if(!command_buffer.opened) {

                    VkCommandBufferInheritanceInfo inheritance_info = {};
                    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                    inheritance_info.pNext = nullptr;
                    inheritance_info.framebuffer = self->main[frameid].framebuffer;
                    inheritance_info.renderPass = self->render_pass;

                    VkCommandBufferBeginInfo command_buffer_begin_info = {};
                    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    command_buffer_begin_info.pNext = nullptr; 
                    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                    command_buffer_begin_info.pInheritanceInfo = &inheritance_info;

                    std::unique_lock<std::mutex> open_buffer_lock(self->open_buffer);
                    {
                        VkResult result = vkBeginCommandBuffer(command_buffer.handle, &command_buffer_begin_info);
                        if(result != VK_SUCCESS) {
                            #if defined(_DEBUG)
                            std::cout << "Thread[" << thread_id << "] : vkBeginCommandBuffer => Failed" << std::endl;
                            #endif
                            if(entity_index >= entity_count) {
                                std::unique_lock<std::mutex> frame_ready_lock(self->frame_ready.mutex);
                                self->frame_ready.condition.notify_one();
                                frame_ready_lock.unlock();
                            }
                            break;
                        }

                        vkCmdBindPipeline(command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, self->mesh_pipeline.handle);
                    }
                    open_buffer_lock.unlock();
                    thread->graphics_command_buffer[frameid].opened = true;
                }

                // Affichage
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(command_buffer.handle, 0, 1, &entity.vertex_buffer, &offset);
                vkCmdBindIndexBuffer(command_buffer.handle, entity.index_buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, self->mesh_pipeline.layout, 0, 1, &entity.descriptor_set, 1, &entity.ubo_offset);
                vkCmdDrawIndexed(command_buffer.handle, entity.index_count, 1, 0, 0, 0);

                // vkCmdBindPipeline(command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, self->normals_pipeline.handle);
                // vkCmdDrawIndexed(command_buffer.handle, entity.index_count, 1, 0, 0, 0);

                // Le travail est terminé, on notifie la boucle principale
                std::unique_lock<std::mutex> frame_ready_lock(self->frame_ready.mutex);
                self->job_done_count++;
                if(self->job_done_count >= entity_count) {
                    self->frame_ready.signaled = true;
                    self->thread_sleep.signaled = false;
                    self->frame_ready.condition.notify_one();
                }
                frame_ready_lock.unlock();
            }
        }
    }
}