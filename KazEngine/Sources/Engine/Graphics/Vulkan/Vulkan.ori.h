#pragma once

#include <thread>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <chrono>

#include "vulkan/vulkan.h"
#include "../../Platform/Common/Window/Window.h"
#include "../../Common/Tools/Tools.h"
#include "../../Common/Maths/Maths.h"
#include "../../Platform/Inputs/Mouse/Mouse.h"
#include "../Camera/Camera.h"

#define ENGINE_VERSION VK_MAKE_VERSION(0, 0, 1)
#define ENGINE_NAME "KazEngine"

#define BACKGROUND_STAGING_BUFFER_SIZE  1024 * 1024 * 50            // 20 Mo (de quoi accueillir un gros mesh ou une grosse texture)
#define MAX_CONCURRENT_TEXTURES         50                          // Taille du descriptor pool

#define MAX_BONES                       100
#define MAX_BONES_PER_VERTEX            4

#if defined(_DEBUG)
#include <iostream>
#endif

namespace Engine
{
    // Trick utilis� pour d�clarer toutes les fonction Vulkan
    // Source : https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1
    #define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
    #define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
    #define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
    #define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
    #include "ListOfFunctions.inl"

    class Vulkan
    {
        public:

            ////////////////////////////////////////
            //       Code de retour renvoy�       //
            // lors de l'initialisation de vulkan //
            ////////////////////////////////////////
            
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
                FRAME_BUFFERS_CREATION_FAILURE          = 23,
                DEPTH_BUFFER_CREATION_FAILURE           = 24
            };

            ////////////////////////////////
            //   Description d'un vertex  //
            // composant le vertex buffer //
            ////////////////////////////////

            struct VERTEX {
                Vector3 vertex;
                Vector3 normal;
                TexCood texture_coordinates;
                std::array<uint32_t, MAX_BONES_PER_VERTEX> bone_ids;
                std::array<float, MAX_BONES_PER_VERTEX> bone_weights;

                VERTEX() : vertex({}), bone_ids({}), bone_weights({}), texture_coordinates({}), normal({}) {}
                VERTEX(VERTEX const& other) : 
                    vertex(other.vertex), bone_ids(other.bone_ids), bone_weights(other.bone_weights),
                    texture_coordinates(other.texture_coordinates), normal(other.normal) {}
                VERTEX(Vector3 v) : vertex(v), bone_ids({}), bone_weights({}), texture_coordinates({}), normal({}) {}

                void AddBone(uint32_t id, float weight) {
                    for(uint8_t i=0; i<this->bone_weights.size(); i++) {
                        if(this->bone_weights[i] == 0.0f) {
                            this->bone_weights[i] = weight;
                            this->bone_ids[i] = id;
                            return;
                        }
                    }
                }

                void NormalizeWeights() {
                    float total_weight = 0.0f;    
                    for(uint8_t i=0; i<this->bone_weights.size(); i++)
                        total_weight += this->bone_weights[i];

                    if(total_weight != 1.0f)
                        for(uint8_t i=0; i<this->bone_weights.size(); i++)
                            this->bone_weights[i] = this->bone_weights[i] / total_weight;
                }
            };

            ////////////////////////////
            //  Ensemble des structs  //
            //  utilis�s pour d�crire //
            //      un squelette      //
            ////////////////////////////

            struct VECTOR_KEYFRAME {
                std::chrono::milliseconds time;
                Vector3 value;

                VECTOR_KEYFRAME() : time(std::chrono::milliseconds()), value({}) {}
                VECTOR_KEYFRAME(std::chrono::milliseconds t, Vector3 v) : time(t), value(v) {}
                VECTOR_KEYFRAME(VECTOR_KEYFRAME const& key) : time(key.time), value(key.value) {}
                bool operator==(VECTOR_KEYFRAME const& other) { return other.time == this->time && other.value == this->value; }
                void operator=(VECTOR_KEYFRAME const& other) { this->time = other.time; this->value = other.value; }
            };

            struct ANIM_TRANSFORM {
                std::vector<VECTOR_KEYFRAME> translations;
                std::vector<VECTOR_KEYFRAME> rotations;
                std::vector<VECTOR_KEYFRAME> scalings;
            };

            struct BONE {
                uint32_t index;
                std::string name;
                Matrix4x4 transformation;
                std::unordered_map<std::string, ANIM_TRANSFORM> animation;  // Cl� : nom de l'animation
                std::unordered_map<uint32_t, Matrix4x4> offset;             // Cl� : index du vertex_buffer du mesh
                std::vector<BONE> children;

                BONE() : index(UINT32_MAX), transformation(IDENTITY_MATRIX) {}
            };

            //////////////////////////
            // D�finition d'un Mesh //
            //////////////////////////

            struct MESH {
                uint32_t vertex_buffer_id;
                uint32_t texture_id;
                uint32_t index_buffer_id;
                uint32_t bone_tree_id;
                uint32_t attached_bone_id;
                Matrix4x4 transformation;

                MESH() : vertex_buffer_id(UINT32_MAX), texture_id(UINT32_MAX), index_buffer_id(UINT32_MAX), attached_bone_id(UINT32_MAX) {}
            };

            static Vulkan* GetInstance();                                                                                               // R�cup�ration de l'instance du singleton
            static void DestroyInstance();                                                                                              // Lib�ration des ressources allou�es � Vulkan
            RETURN_CODE Initialize(Engine::Window* draw_window, uint32_t application_version, std::string aplication_name);             // Initialisation de Vulkan
            uint32_t CreateTexture(Tools::IMAGE_MAP& image);                                                                            // Copie d'une texture dans la carte graphique
            uint32_t CreateTexture(ColorRGB const& color);                                                                              // Cr�ation d'une texture de couleur unie dans la carte graphique
            uint32_t CreateVertexBuffer(std::vector<VERTEX>& data);                                                                     // Cr�ation d'un vertex buffer
            uint32_t CreateIndexBuffer(std::vector<uint32_t>& data);                                                                    // Cr�ation d'un vertex buffer
            uint32_t CreateBoneTree(BONE& tree);                                                                                        // Cr�ation d'un bone tree
            uint32_t CreateMesh(MESH& mesh);                                                                                            // Cr�ation d'un mesh
            void UpdateSkeleton(uint32_t mesh_id, std::chrono::milliseconds& time, std::string const& animation);                       // �value le squelette d'un mesh pour une animation et un temps donn�
            void Draw();                                                                                                                // Boucle d'affichage principale

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
                uint32_t count;

                VERTEX_BUFFER() : handle(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), size(0), count(0) {}
            };

            typedef struct VERTEX_BUFFER INDEX_BUFFER;

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

            struct DEPTH_BUFFER {
                VkImage image;
                VkDeviceMemory memory;
                VkImageView view;
                VkFormat format;

                DEPTH_BUFFER() : image(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), view(VK_NULL_HANDLE), format(VK_FORMAT_UNDEFINED) {}
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

            // Attention, le uniform buffer contient un un alignement de ses membres bas� sur le membre ayant le plus grand alignement
            // dans notre cas, le uint32_t sera align� sur les matrices 4*4  (16 octets). Il sera donc suivi de 16 - 4 = 12 octets de padding
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#interfaces-resources-layout
            struct UBO {
                Matrix4x4 projection;
	            Matrix4x4 view;
	            Matrix4x4 model;
                uint32_t bone_id;
                std::array<uint8_t, 12> padding;
	            std::array<Matrix4x4, MAX_BONES> bones;

                UBO() : projection(IDENTITY_MATRIX), view(IDENTITY_MATRIX), bone_id(-1), model(IDENTITY_MATRIX), padding({})
                {
                    for(Matrix4x4& bone : this->bones) bone = IDENTITY_MATRIX;
                }
            };

            struct ENTITY {
                UBO ubo;
                uint32_t vertex_count;
                VkBuffer vertex_buffer;
                VkBuffer index_buffer;
                uint32_t index_count;
                VkDescriptorSet descriptor_set;
                uint32_t ubo_offset;

                ENTITY() : vertex_count(0), vertex_buffer(VK_NULL_HANDLE), index_buffer(VK_NULL_HANDLE), descriptor_set(VK_NULL_HANDLE) {}
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
                std::unordered_map<uint32_t, ENTITY> entities;
            };

            struct THREAD_HANDLER {
                std::thread handle;
                VkCommandPool command_pool;
                std::vector<COMMAND_BUFFER> graphics_command_buffer;

                THREAD_HANDLER() : command_pool(VK_NULL_HANDLE) {}
            };

            struct ANIMATION {
                std::string name;
                double duration;
                double ticks_per_seconds;

                ANIMATION() : duration(0), ticks_per_seconds(0) {}
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

            // Fen�tre d'affichage
            Window* draw_window;
            Surface draw_surface;
            VkSurfaceKHR presentation_surface;

            // P�riph�rique physique choisi pour l'affichage
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
            PIPELINE mesh_pipeline;
            PIPELINE normals_pipeline;

            // Depth Buffer
            DEPTH_BUFFER depth_buffer;

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
            BACKGROUD_TASKS_RESOURCES background;                           // Ressources utilis�es pour les t�ches asynchrones comme le chargement de textures ou de vertext buffers

            // Texture 
            uint32_t last_texture_index;                                    // Indice de la prochaine texture � allouer
            std::unordered_map<uint32_t, TEXTURE> textures;                 // Structure de stockage pour les textures cr��es

            // Vertex Buffers
            uint32_t last_vbo_index;                                        // Indice du prochain vertex buffer � allouer
            std::unordered_map<uint32_t, VERTEX_BUFFER> vertex_buffers;     // Structure de stockage pour les vertex buffers cr��s

            // Index Buffers
            uint32_t last_ibo_index;                                        // Indice du prochain index buffer � allouer
            std::unordered_map<uint32_t, INDEX_BUFFER> index_buffers;       // Structure de stockage pour les index buffers cr��s

            // Bone Trees
            uint32_t last_bone_tree_index;                                  // Indice du prochain bone tree � allouer
            std::unordered_map<uint32_t, BONE> bone_trees;                  // Ensemble de bone trees

            // Mesh
            uint32_t last_mesh_index;                                       // Indice du prochain mesh � allouer
            std::unordered_map<uint32_t, MESH> meshes;                      // Ensemble de models

            // Rendu
            Matrix4x4 base_projection;                                      // Matrice de projection pr�calcul�e
            Camera camera;                                                  // Cam�ra

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
            bool GetVersion(std::string &version_string);                                                       // R�cup�re la version de vulkan
            bool CreateVulkanInstance(uint32_t application_version, std::string& aplication_name);              // Cr�ation de l'instance vulkan
            bool LoadExportedEntryPoints();                                                                     // Chargement de l'exporteur de fonctions vulkan
            bool LoadGlobalLevelEntryPoints();                                                                  // Chargement des fonctions vulkan de port�e globale
            bool LoadInstanceLevelEntryPoints();                                                                // Chargement des fonctions vulkan portant sur l'instance
            bool LoadDeviceLevelEntryPoints();                                                                  // Chargement des fonctions vulkan portant sur le logical device
            bool CreatePresentationSurface();                                                                   // Cr�ation de la surface de pr�sentation
            bool CreateDevice(char preferred_device_index = -1);                                                // Cr�ation du logical device
            void GetDeviceQueues();                                                                             // R�cup�re le handle des device queues
            bool CreateSwapChain();                                                                             // Cr�ation de la swap chain
            bool CreateRenderPass();                                                                            // Cr�ation de la render pass
            bool CreatePipeline(bool dynamic_viewport = false);                                                 // Cr�ation du pipeline
            bool CreateStagingBuffer(STAGING_BUFFER& staging_buffer, VkDeviceSize size);                        // Cr�ation d'un staging buffer
            bool CreateUniformBuffer(UNIFORM_BUFFER& uniform_buffer, VkDeviceSize size);                        // Cr�ation d'un uniform buffer
            bool InitMainThread();                                                                              // Allocation des resources de la boucle principale
            void ReleaseMainThread();                                                                           // Lib�re les ressources de la boucle principale
            bool InitThreadSharedResources();                                                                   // Allocation des resources partag�es
            void ReleaseThreadSharedResources();                                                                // Lib�re les ressources partag�es
            bool OnWindowSizeChanged();                                                                         // Callback appel�e lors du redimensiontionnement de la fen�tre
            bool InitBackgroundResources();                                                                     // Allocation des ressources d'arri�re plan
            void ReleaseBackgroundResources();                                                                  // Lib�re les ressources d'arri�re plan
            bool PrepareTextureDescriptorSets();                                                                // Cr�ation du descriptor pool pour objet textur�
            // bool PrepareColorDescriptorSets();                                                                  // Cr�ation du descriptor pool pour objet color�
            bool UpdateEntities(uint32_t frame_index);                                                          // Mise � jour des transformations des entit�s
            bool InitThreads();                                                                                 // Initialisation des threads
            static void ThreadJob(uint32_t thread_id);                                                          // Fonction de traitement ex�cut�e par les threads
            void ReleaseThreads();                                                                              // Arr�te les threads et lib�re les ressources associ�es
            bool CreateFrameBuffers();                                                                          // Cr�ation des frame buffers
            bool CreateDepthBuffer();                                                                           // Depth Buffer

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

            // Animations
            void InitializeBones(UBO& ubo, BONE& tree, uint32_t vertex_buffer_id, Matrix4x4& parent_transformation);
            void EvalBones(UBO& ubo, BONE& tree, uint32_t vertex_buffer_id, Matrix4x4& parent_transformation, std::chrono::milliseconds& time, std::string const& animation);
            Vector3 EvalInterpolation(std::vector<VECTOR_KEYFRAME> const& keyframes, std::chrono::milliseconds const& time, VECTOR_KEYFRAME const& base = {});

            // Others
            bool MemoryTypeFromProperties(uint32_t type_bits, VkFlags requirements_mask, uint32_t &type_index);
            bool AllocateCommandBuffer(VkCommandPool& pool, uint32_t count, std::vector<VkCommandBuffer>& buffer, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            bool CreateCommandBuffer(VkCommandPool pool, std::vector<COMMAND_BUFFER>& command_buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            bool CreateSemaphore(VkSemaphore &semaphore);
            bool CreateCommandPool(VkCommandPool& pool, uint32_t queue_family_index);
            VkPipelineShaderStageCreateInfo LoadShaderModule(std::string const& filename, VkShaderStageFlagBits type);

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