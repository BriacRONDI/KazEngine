#pragma once

#include <thread>
#include <mutex>
#include <string>
#include <EventEmitter.hpp>


#include "../Vulkan/Vulkan.h"
#include "../DescriptorSet/DescriptorSet.h"

namespace Engine
{
    class IComputeResult;
    class ThreadedShader : public Tools::EventEmitter<IComputeResult>
    {
        public :
            bool Init(std::string shader_name, std::vector<const DescriptorSet*> descriptor_sets, bool enable_thread = true);
            inline VkCommandBuffer GetWriteCommandBuffer() const { return this->write_command_buffer; }
            inline VkCommandBuffer GetReadCommandBuffer() const { return this->read_command_buffer; }
            void Run(bool loop = false);
            void Stop();
            void Clear();
            inline ~ThreadedShader() { this->Clear(); }
            bool BuildCommandBuffer(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1);
            void UpdateWriteCommandBuffer(bool enable = true, bool write_once = true);
            void UpdateReadCommandBuffer(bool enable = true);
            void Update();

        private :
            VkCommandPool command_pool;
            VkCommandBuffer job_command_buffer;
            VkCommandBuffer write_command_buffer;
            VkCommandBuffer read_command_buffer;
            VkSemaphore write_semaphore;
            Vulkan::PIPELINE pipeline;
            VkSemaphore job_semaphore;
            VkFence fence;
            std::vector<const DescriptorSet*> descriptor_sets;
            std::thread thread;
            bool loop_condition;
            bool write_job;
            bool read_job;
            bool write_once;
            bool enable_thread;
            bool running;

            static void ThreadJob(ThreadedShader* self);
    };
}