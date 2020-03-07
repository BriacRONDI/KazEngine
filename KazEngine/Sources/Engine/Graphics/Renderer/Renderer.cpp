#include "Renderer.h"

namespace Engine
{
    /**
     * Création de la pipeline et du descriptor set
     */
    bool Renderer::Initialize(const uint16_t schema,
                              std::array<std::string, 3> const& shaders,
                              std::vector<VkDescriptorSetLayout> const& descriptor_set_layouts,
                              VkPolygonMode polygon_mode)
    {
        // Capacités de rendu des shaders
        this->render_mask = schema;

        //////////////////////////////////
        // Description du Vertex Buffer //
        //////////////////////////////////

        uint32_t location = 0;
        uint32_t offset = 0;
        std::vector<VkVertexInputAttributeDescription> vertex_attribute_description;
        VkVertexInputAttributeDescription attribute;

        // Position
        if(schema & SCHEMA_PRIMITIVE::POSITION_VERTEX) {
            attribute.location = location;
            attribute.binding = 0;
            attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            attribute.offset = offset;
            vertex_attribute_description.push_back(attribute);
            offset += sizeof(Vector3);
        }

        // Normal
        if(schema & SCHEMA_PRIMITIVE::NORMAL_VERTEX) {
            attribute.location = ++location;
            attribute.binding = 0;
            attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            attribute.offset = offset;
            vertex_attribute_description.push_back(attribute);
            offset += sizeof(Vector3);
        }

        // UV
        if(schema & SCHEMA_PRIMITIVE::UV_VERTEX) {
            attribute.location = ++location;
            attribute.binding = 0;
            attribute.format = VK_FORMAT_R32G32_SFLOAT;
            attribute.offset = offset;
            vertex_attribute_description.push_back(attribute);
            offset += sizeof(Vector2);
        }

        // Color
        if(schema & SCHEMA_PRIMITIVE::COLOR_VERTEX) {
            attribute.location = ++location;
            attribute.binding = 0;
            attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            attribute.offset = offset;
            vertex_attribute_description.push_back(attribute);
            offset += sizeof(Vector3);
        }

        // Squelette
        if(schema & SCHEMA_PRIMITIVE::SKELETON) {
            // Bone Weights
            attribute.location = ++location;
            attribute.binding = 0;
            attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attribute.offset = offset;
            vertex_attribute_description.push_back(attribute);
            offset += sizeof(Vector4);

            // Bone IDs
            attribute.location = ++location;
            attribute.binding = 0;
            attribute.format = VK_FORMAT_R32G32B32A32_SINT;
            attribute.offset = offset;
            vertex_attribute_description.push_back(attribute);
            offset += sizeof(int) * 4;
        }

        VkVertexInputBindingDescription vertex_binding_description = {};
        vertex_binding_description.binding = 0;
        vertex_binding_description.stride = offset;
        vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        ///////////////////////////////////
        // Description des Shader Stages //
        ///////////////////////////////////

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = Vulkan::GetInstance().LoadShaderModule(shaders[0], VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = Vulkan::GetInstance().LoadShaderModule(shaders[1], VK_SHADER_STAGE_FRAGMENT_BIT);
        if(!shaders[2].empty()) {
            shader_stages.resize(3);
            shader_stages[2] = Vulkan::GetInstance().LoadShaderModule(shaders[2], VK_SHADER_STAGE_GEOMETRY_BIT);
        }

        std::vector<VkPushConstantRange> push_constant_ranges;
        if(schema & SCHEMA_PRIMITIVE::MATERIAL) {
            VkPushConstantRange range;
            range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            range.offset = 0;
            range.size = Mesh::MATERIAL::Size();
            push_constant_ranges.push_back(range);
        }
        if(schema & SCHEMA_PRIMITIVE::SINGLE_BONE) {
            VkPushConstantRange range;
            range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            range.offset = Mesh::MATERIAL::Size();
            range.size = sizeof(uint32_t);
            push_constant_ranges.push_back(range);
        }

        bool success = Vulkan::GetInstance().CreatePipeline(
            true,
            descriptor_set_layouts,
            shader_stages, vertex_binding_description,
            vertex_attribute_description,
            push_constant_ranges,
            this->pipeline,
            polygon_mode
        );

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        #if defined(DISPLAY_LOGS)
        std::cout << "Renderer::Initialize [" << schema << "] : ";
        if(success) std::cout << "OK" << std::endl;
        else std::cout << "Failed" << std::endl;
        #endif

        return success;
    }

    /**
     * Libération des ressources
     */
    void Renderer::Release()
    {
        // Pipeline
        if(this->pipeline.handle != VK_NULL_HANDLE) vkDestroyPipeline(Vulkan::GetDevice(), this->pipeline.handle, nullptr);
        if(this->pipeline.layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(Vulkan::GetDevice(), this->pipeline.layout, nullptr);
        this->pipeline = {};
    }
}