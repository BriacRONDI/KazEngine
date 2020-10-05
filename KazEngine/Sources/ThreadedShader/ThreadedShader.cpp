#include "TheadedShader.h"
#include "IComputeResult.h"

namespace Engine
{
    bool ThreadedShader::Init(std::string shader_name, std::vector<const DescriptorSet*> descriptor_sets, bool enable_thread)
    {
        this->command_pool          = nullptr;
        this->job_command_buffer    = nullptr;
        this->write_command_buffer  = nullptr;
        this->read_command_buffer   = nullptr;
        this->write_semaphore       = nullptr;
        this->job_semaphore         = nullptr;
        this->fence                 = nullptr;

        this->write_job             = false;
        this->read_job              = false;
        this->write_once            = false;
        this->running               = false;

        this->descriptor_sets = descriptor_sets;
        this->enable_thread = enable_thread;

        auto shader_module = Vulkan::GetInstance().LoadShaderModule("./Shaders/" + shader_name, VK_SHADER_STAGE_COMPUTE_BIT);

        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        for(auto& set : descriptor_sets) descriptor_set_layouts.push_back(set->GetLayout());
        bool success = Vulkan::GetInstance().CreateComputePipeline(shader_module, descriptor_set_layouts, {}, this->pipeline);

        vkDestroyShaderModule(Vulkan::GetDevice(), shader_module.module, nullptr);

        if(!success) return false;

        if(!Vulkan::GetInstance().CreateCommandPool(this->command_pool, Vulkan::GetComputeQueue().index)) return false;
        if(!Vulkan::GetInstance().CreateCommandBuffer(this->command_pool, this->job_command_buffer)) return false;
        if(!Vulkan::GetInstance().CreateCommandBuffer(this->command_pool, this->write_command_buffer)) return false;
        if(!Vulkan::GetInstance().CreateCommandBuffer(this->command_pool, this->read_command_buffer)) return false;
        if(!Vulkan::GetInstance().CreateSemaphore(this->job_semaphore)) return false;
        if(!Vulkan::GetInstance().CreateSemaphore(this->write_semaphore)) return false;
        this->fence = Vulkan::GetInstance().CreateFence();
        if(this->fence == nullptr) return false;

        vkResetFences(Vulkan::GetDevice(), 1, &this->fence);

        return true;
    }

    void ThreadedShader::Clear()
    {
        this->Stop();

        if(this->fence != nullptr) vkDestroyFence(Vulkan::GetDevice(), this->fence, nullptr);
        if(this->job_semaphore != nullptr) vkDestroySemaphore(Vulkan::GetDevice(), this->job_semaphore, nullptr);
        if(this->write_semaphore != nullptr) vkDestroySemaphore(Vulkan::GetDevice(), this->write_semaphore, nullptr);
        if(this->command_pool != nullptr) vkDestroyCommandPool(Vulkan::GetDevice(), this->command_pool, nullptr);

        this->pipeline.Clear();
        this->descriptor_sets.clear();

        this->command_pool          = nullptr;
        this->job_command_buffer    = nullptr;
        this->write_command_buffer  = nullptr;
        this->read_command_buffer   = nullptr;
        this->write_semaphore       = nullptr;
        this->job_semaphore         = nullptr;
        this->fence                 = nullptr;

        this->write_job             = false;
        this->read_job              = false;
        this->write_once            = false;
        this->enable_thread         = false;
        this->running               = false;
    }

    bool ThreadedShader::BuildCommandBuffer(uint32_t x, uint32_t y, uint32_t z)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr; 
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(this->job_command_buffer, &command_buffer_begin_info);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "ThreadedShader::BuildComputeCommandBuffer => vkBeginCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        std::vector<VkDescriptorSet> sets;
        for(auto& set : this->descriptor_sets) sets.push_back(set->Get());

		vkCmdBindPipeline(this->job_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline.handle);

        vkCmdBindDescriptorSets(this->job_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline.layout, 0,
                                static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
            
		vkCmdDispatch(this->job_command_buffer, x, y, z);

		result = vkEndCommandBuffer(this->job_command_buffer);
        if(result != VK_SUCCESS) {
            #if defined(DISPLAY_LOGS)
            std::cout << "ThreadedShader::BuildComputeCommandBuffer => vkEndCommandBuffer : Failed" << std::endl;
            #endif
            return false;
        }

        return true;
    }

    void ThreadedShader::Run(bool loop)
    {
        this->loop_condition = loop;
        this->running = true;

        if(this->enable_thread) this->thread = std::thread(ThreadJob, this);
    }

    void ThreadedShader::Stop()
    {
        this->loop_condition = false;
        this->running = false;

        if(this->enable_thread && this->thread.joinable()) this->thread.join();
    }

    void ThreadedShader::UpdateWriteCommandBuffer(bool enable, bool write_once)
    {
        this->write_job = enable;
        this->write_once = write_once;
    }

    void ThreadedShader::UpdateReadCommandBuffer(bool enable)
    {
        this->read_job = enable;
    }

    void ThreadedShader::Update()
    {
        if(this->running && !this->enable_thread) {
            ThreadedShader::ThreadJob(this);
            if(!this->loop_condition) this->running = false;
        }
    }

    void ThreadedShader::ThreadJob(ThreadedShader* self)
    {
        // TODO Get the secondary compute queue index at vulkan initilization
        VkQueue queue;
        if(self->enable_thread) vkGetDeviceQueue(Vulkan::GetDevice(), Vulkan::GetComputeQueue().index, 1, &queue);
        else queue = Vulkan::GetComputeQueue().handle;

        while(self->running) {

            VkSubmitInfo submit_info;
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = nullptr;
            submit_info.commandBufferCount = 1;

            ///////////////////////
            // WRITE DATA TO GPU //
            ///////////////////////

            if(self->write_job) {
                submit_info.pCommandBuffers = &self->write_command_buffer;

                submit_info.waitSemaphoreCount = 0;
                submit_info.pWaitSemaphores = nullptr;
                submit_info.pWaitDstStageMask = nullptr;

                submit_info.signalSemaphoreCount = 1;
                submit_info.pSignalSemaphores = &self->write_semaphore;

                VkResult result = vkQueueSubmit(queue, 1, &submit_info, nullptr);
                if(result != VK_SUCCESS) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "ThreadedShader::ThreadJob => vkQueueSubmit [write] : Failed" << std::endl;
                    #endif
                    return;
                }
            }

            ////////////////////////
            // RUN COMPUTE SHADER //
            ////////////////////////

            submit_info.pCommandBuffers = &self->job_command_buffer;

            if(self->write_job) {
                VkPipelineStageFlags mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                submit_info.waitSemaphoreCount = 1;
                submit_info.pWaitSemaphores = &self->write_semaphore;
                submit_info.pWaitDstStageMask = &mask;
            }else{
                submit_info.waitSemaphoreCount = 0;
                submit_info.pWaitSemaphores = nullptr;
                submit_info.pWaitDstStageMask = nullptr;
            }

            VkFence job_fence = nullptr;
            if(self->read_job) {
                submit_info.signalSemaphoreCount = 1;
                submit_info.pSignalSemaphores = &self->job_semaphore;
            }else{
                submit_info.signalSemaphoreCount = 0;
                submit_info.pSignalSemaphores = nullptr;
                job_fence = self->fence;
            }

            VkResult result = vkQueueSubmit(queue, 1, &submit_info, job_fence);
            if(result != VK_SUCCESS) {
                #if defined(DISPLAY_LOGS)
                std::cout << "ThreadedShader::ThreadJob => vkQueueSubmit [job] : Failed" << std::endl;
                #endif
                return;
            }

            ////////////////////////
            // READ DATA FROM GPU //
            ////////////////////////

            if(self->read_job) {

                submit_info.pCommandBuffers = &self->read_command_buffer;

                VkPipelineStageFlags mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                submit_info.waitSemaphoreCount = 1;
                submit_info.pWaitSemaphores = &self->job_semaphore;
                submit_info.pWaitDstStageMask = &mask;

                submit_info.signalSemaphoreCount = 0;
                submit_info.pSignalSemaphores = nullptr;

                result = vkQueueSubmit(queue, 1, &submit_info, self->fence);
                if(result != VK_SUCCESS) {
                    #if defined(DISPLAY_LOGS)
                    std::cout << "ThreadedShader::ThreadJob => vkQueueSubmit [read] : Failed" << std::endl;
                    #endif
                    return;
                }
            }

            result = vkWaitForFences(Vulkan::GetDevice(), 1, &self->fence, VK_FALSE, 1000000000);
            if(result != VK_SUCCESS) {
                #if defined(DISPLAY_LOGS)
                std::cout << "ThreadedShader::ThreadJob => vkWaitForFences : Timeout" << std::endl;
                #endif
                return;
            }
            vkResetFences(Vulkan::GetDevice(), 1, &self->fence);

            if(self->write_job && self->write_once) self->write_job = false;

            /////////////////////////////////
            // COMPUTE SHADER JOB FINISHED //
            /////////////////////////////////

            self->Listeners[0]->ComputeFinished(self);

            if(self->enable_thread) {
                if(!self->loop_condition) {
                    self->running = false;
                    return;
                }
            }else{
                return;
            }
        }
    }
}