#include "UserInterface.h"

namespace Engine
{
    void UserInterface::Clear()
    {
        this->need_update.clear();
        this->update_selection_square.clear();
        this->selection_square.display = false;

        Mouse::GetInstance().RemoveListener(this);

        if(Vulkan::HasInstance()) {

            vkDeviceWaitIdle(Vulkan::GetDevice());

            // Chunks
            DataBank::GetManagedBuffer().FreeChunk(this->ui_vbo_chunk);

            // Pipeline
            this->pipeline.Clear();

            // Secondary graphics command buffers
            for(auto command_buffer : this->command_buffers)
                if(command_buffer != nullptr) vkFreeCommandBuffers(Vulkan::GetDevice(), this->command_pool, 1, &command_buffer);

            // Render Pass
            if(this->render_pass != nullptr) vkDestroyRenderPass(Vulkan::GetDevice(), this->render_pass, nullptr);
        }
    }

    UserInterface::UserInterface(VkCommandPool command_pool) : command_pool(command_pool)
    {
        this->Clear();

        Mouse::GetInstance().AddListener(this);

        uint32_t frame_count = Vulkan::GetConcurrentFrameCount();
        this->need_update.resize(frame_count, true);
        this->update_selection_square.resize(frame_count, true);

        // Allocate draw command buffers
        this->command_buffers.resize(frame_count);
        if(!Vulkan::GetInstance().AllocateCommandBuffer(command_pool, this->command_buffers, VK_COMMAND_BUFFER_LEVEL_PRIMARY)) {
            #if defined(DISPLAY_LOGS)
            std::cout << "UserInterface::UserInterface() => AllocateCommandBuffer : Failed" << std::endl;
            #endif
            this->Clear();
            return;
        }

        if(!this->selection_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(MOUSE_SELECTION_SQUARE)}
        }, frame_count)) {
            this->Clear();
            return;
        }

        if(!this->SetupRenderPass() || !this->SetupPipeline()) this->Clear();
    }

    bool UserInterface::SetupPipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(2);
        shader_stages[0] = Vulkan::GetInstance().LoadShaderModule("./Shaders/interface.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = Vulkan::GetInstance().LoadShaderModule("./Shaders/interface.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        size_t chunk_size = Vulkan::GetInstance().ComputeUniformBufferAlignment(SIZE_KILOBYTE(5));
        this->ui_vbo_chunk = DataBank::GetManagedBuffer().ReserveChunk(chunk_size);

        VkVertexInputBindingDescription vertex_binding_description;
        auto vertex_attribute_description = Vulkan::CreateVertexInputDescription({Vulkan::POSITION_2D, Vulkan::UV}, vertex_binding_description);

        bool success = Vulkan::GetInstance().CreatePipeline(
            true, {this->selection_descriptor.GetLayout()},
            shader_stages, vertex_binding_description, vertex_attribute_description, {}, this->pipeline,
            VK_POLYGON_MODE_FILL, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, true
        );

        for(auto& stage : shader_stages)
            vkDestroyShaderModule(Vulkan::GetDevice(), stage.module, nullptr);

        return true;
    }

    bool UserInterface::SetupRenderPass()
    {
		VkAttachmentDescription attachment[2];

        attachment[0].format = Vulkan::GetSwapChain().format;
        attachment[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment[0].flags = 0;

        attachment[1].format = Vulkan::GetDepthFormat();
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

        VkResult result = vkCreateRenderPass(Vulkan::GetDevice(), &rp_info, nullptr, &this->render_pass);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "UserInterface::SetupRenderPass() => vkCreateRenderPass : Failed" << std::endl;
            #endif
            return false;
        }

        #if defined(DISPLAY_LOGS)
        std::cout << "UserInterface::SetupRenderPass() : Success" << std::endl;
        #endif
        return true;
    }

    bool UserInterface::UpdateVertexBuffer(uint8_t frame_index)
    {
        struct SHADER_INPUT {
            Maths::Vector2 position;
            Maths::Vector2 uv;
        };

        std::vector<SHADER_INPUT> vertex_data(4);
        vertex_data[0].position = {-1.0f, -1.0f};
        vertex_data[0].uv = { 0.0f, 0.0f };
        vertex_data[1].position = {1.0f, -1.0f};
        vertex_data[1].uv = { 1.0f, 0.0f };
        vertex_data[2].position = {-1.0f, 1.0f};
        vertex_data[2].uv = { 0.0f, 1.0f };
        vertex_data[3].position = {1.0f, 1.0f};
        vertex_data[3].uv = { 1.0f, 1.0f };

        DataBank::GetManagedBuffer().WriteData(vertex_data.data(), static_cast<uint32_t>(vertex_data.size()) * sizeof(SHADER_INPUT),
                                               this->ui_vbo_chunk->offset, frame_index);

        return true;
    }

    void UserInterface::Update(uint8_t frame_index)
    {
        if(this->update_selection_square[frame_index]) {
            this->selection_descriptor.WriteData(&this->selection_square, sizeof(MOUSE_SELECTION_SQUARE), 0, frame_index);
            this->update_selection_square[frame_index] = false;
        }
    }

    void UserInterface::Refresh(uint8_t frame_index)
    {
        this->need_update[frame_index] = true;
        vkResetCommandBuffer(this->command_buffers[frame_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    VkCommandBuffer UserInterface::BuildCommandBuffer(uint8_t frame_index, VkFramebuffer framebuffer)
    {
        VkCommandBuffer command_buffer = this->command_buffers[frame_index];

        if(!this->need_update[frame_index]) return command_buffer;
        this->UpdateVertexBuffer(frame_index);
        this->need_update[frame_index] = false;

        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "UserInterface::GetCommandBuffer() => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        VkClearValue clear_values[2];
        clear_values[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = this->render_pass;
        render_pass_begin_info.framebuffer = framebuffer;
        render_pass_begin_info.renderArea.offset.x = 0;
        render_pass_begin_info.renderArea.offset.y = 0;
        render_pass_begin_info.renderArea.extent.width = Vulkan::GetDrawSurface().width;
        render_pass_begin_info.renderArea.extent.height = Vulkan::GetDrawSurface().height;
        render_pass_begin_info.clearValueCount = 2;
        render_pass_begin_info.pClearValues = clear_values;

        // Début de la render pass primaire
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        auto& surface = Vulkan::GetDrawSurface();
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(surface.width);
        viewport.height = static_cast<float>(surface.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = surface.width;
        scissor.extent.height = surface.height;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        if(this->selection_square.display > 0 && Camera::GetInstance().IsRtsMode()) {
            VkDescriptorSet set = this->selection_descriptor.Get(frame_index);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &set, 0, nullptr);
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.handle);
            vkCmdBindVertexBuffers(command_buffer, 0, 1, &DataBank::GetManagedBuffer().GetBuffer(frame_index).handle, &this->ui_vbo_chunk->offset);
            vkCmdDraw(command_buffer, 4, 1, 0, 0);
        }

        vkCmdEndRenderPass(command_buffer);

        result = vkEndCommandBuffer(command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "UserInterface::GetCommandBuffer() => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return nullptr;
        }

        // Succès
        return command_buffer;
    }

    void UserInterface::MouseMove(unsigned int x, unsigned int y)
    {
        if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_LEFT)) {
            this->selection_square.end = {x,y};
            this->update_selection_square = {true, true, true};
            if(this->selection_square.display == 0) {
                this->selection_square.display = 1;
                Camera::GetInstance().Freeze();
                this->need_update = {true, true, true};
            }
        }
    }

    void UserInterface::MouseButtonDown(MOUSE_BUTTON button)
    {
        if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT)
            this->selection_square.start = Mouse::GetInstance().GetPosition();
    }

    void UserInterface::MouseButtonUp(MOUSE_BUTTON button)
    {
        if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT) {
            if(this->selection_square.display > 0 && Camera::GetInstance().IsRtsMode()) {
                this->selection_square.display = 0;
                this->update_selection_square = {true, true, true};
                this->need_update = {true, true, true};
                Camera::GetInstance().UnFreeze();
                for(auto& listener : this->Listeners)
                    listener->SquareSelection(this->selection_square.start, this->selection_square.end);
            }else{
                for(auto& listener : this->Listeners)
                    listener->ToggleSelection(Mouse::GetInstance().GetPosition());
            }
        }
    }
}