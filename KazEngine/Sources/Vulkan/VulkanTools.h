#pragma once

#include <Tools.h>
#include <Maths.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "../Chunk/Chunk.h"

#define SIZE_KILOBYTE(kb) 1024 * kb
#define SIZE_MEGABYTE(mb) 1024 * SIZE_KILOBYTE(mb)

namespace Engine { namespace vk
{
    struct IMAGE_BUFFER {
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;
        VkFormat format;
        VkImageAspectFlags aspect;

        inline IMAGE_BUFFER() : handle(nullptr), memory(nullptr), view(nullptr), format(VK_FORMAT_UNDEFINED), aspect(0) {}
        void Clear();
    };

    struct DATA_BUFFER {
        VkBuffer handle;
        VkDeviceMemory memory;
        VkDeviceSize size;

        inline DATA_BUFFER() : handle(nullptr), memory(nullptr), size(0) {}
        void Clear();
    };

    struct MAPPED_BUFFER : DATA_BUFFER {
        char* pointer;

        bool Map();
        inline MAPPED_BUFFER() : DATA_BUFFER(), pointer(nullptr) {}
        inline void Clear() { DATA_BUFFER::Clear(); this->pointer = nullptr; }
    };

    struct PIPELINE {
        VkPipelineLayout layout;
        VkPipeline handle;

        inline PIPELINE() : layout(nullptr), handle(nullptr) {}
        void Clear();
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

    IMAGE_BUFFER CreateImageBuffer(VkImageUsageFlags usage, VkImageAspectFlags aspect, uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    bool CreateDataBuffer(DATA_BUFFER& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkFlags requirement, std::vector<uint32_t> const& queue_families = {});
    bool CreateCommandPool(VkCommandPool& pool, uint32_t queue_family_index, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    bool CreateCommandBuffer(VkCommandPool pool, VkCommandBuffer& command_buffer, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    bool CreateFence(VkFence& fence, bool reset = true);
    bool CreateSemaphore(VkSemaphore &semaphore);
    bool SubmitQueue(std::vector<VkCommandBuffer> command_buffers, std::vector<VkSemaphore> wait_semaphores,
                     std::vector<VkPipelineStageFlags> wait_stages, VkSemaphore signal_semaphore,
                     VkQueue queue, VkFence fence);
    bool SubmitQueue(std::vector<VkSubmitInfo> infos, VkQueue queue, VkFence fence);
    size_t SendImageToBuffer(IMAGE_BUFFER& buffer, Tools::IMAGE_MAP const& image, VkCommandBuffer command_buffer, VkFence fence, MAPPED_BUFFER staging);
    size_t SendDataToBuffer(DATA_BUFFER& buffer, VkCommandBuffer command_buffer, MAPPED_BUFFER staging_buffer,
                            std::vector<Chunk> chunks, VkQueue queue, VkSemaphore signal, VkSemaphore wait);
    VkSubmitInfo PrepareQueue(std::vector<VkCommandBuffer> command_buffers, std::vector<VkSemaphore> wait_semaphores,
                     std::vector<VkPipelineStageFlags> wait_stages, VkSemaphore signal_semaphore,
                     VkQueue queue, VkFence fence);
    VkPipelineShaderStageCreateInfo LoadShaderModule(std::string const& filename, VkShaderStageFlagBits type);
    std::vector<VkVertexInputAttributeDescription> CreateVertexInputDescription(
                std::vector<std::vector<VERTEX_BINDING_ATTRIBUTE>> attributes,
                std::vector<VkVertexInputBindingDescription>& descriptions);
    bool WaitForFence(VkFence fence);
    bool CreateComputePipeline(VkPipelineShaderStageCreateInfo stage,
                               std::vector<VkDescriptorSetLayout> descriptor_set_layouts,
                               std::vector<VkPushConstantRange> push_constant_ranges,
                               PIPELINE& pipeline);
    bool CreateGraphicsPipeline(bool dynamic_viewport,
                                std::vector<VkDescriptorSetLayout> descriptor_set_layouts,
                                std::vector<VkPipelineShaderStageCreateInfo> shader_stages,
                                std::vector<VkVertexInputBindingDescription> vertex_binding_description,
                                std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions,
                                std::vector<VkPushConstantRange> push_constant_ranges,
                                PIPELINE& pipeline,
                                VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL,
                                VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                bool blend_state = false);
    bool TransitionImageLayout(
            IMAGE_BUFFER& buffer,
            VkAccessFlags source_mask, VkAccessFlags dest_mask,
            VkImageLayout old_layout, VkImageLayout new_layout,
            VkPipelineStageFlags source_stage, VkPipelineStageFlags dest_stage,
            VkCommandBuffer command_buffer, VkFence fence);
    bool GetVulkanVersion(std::string &version_string);

    void Destroy(VkCommandPool pool);
    void Destroy(VkFence fence);
    void Destroy(VkSemaphore semaphore);
    void Destroy(VkFramebuffer frame_buffer);
    void Destroy(VkDescriptorSetLayout layout);
    void Destroy(VkSampler sampler);
    void Destroy(VkDescriptorPool pool);
    void Destroy(VkPipelineShaderStageCreateInfo stage);
    void Destroy(VkRenderPass render_pass);
    void Destroy(VkImageView view);

    inline void Destroy(IMAGE_BUFFER& image) { image.Clear(); }
    inline void Destroy(DATA_BUFFER& buffer) { buffer.Clear(); }
    inline void Destroy(MAPPED_BUFFER& buffer) { buffer.Clear(); }
    inline void Destroy(PIPELINE& pipeline) { pipeline.Clear(); }
}}