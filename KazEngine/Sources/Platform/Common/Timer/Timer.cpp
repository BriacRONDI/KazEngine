#include "Timer.h"

namespace Engine
{
    std::chrono::system_clock::time_point Timer::engine_start = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point Timer::now = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point Timer::last_frame;
    DescriptorSet Timer::time_descriptor;

    bool Timer::Initialize()
    {
        if(!Timer::time_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t) * 2}
        })) return false;

        Timer::last_frame = Timer::now;
        Timer::now = std::chrono::system_clock::now();

        TIMER_DATA data = {};
        Timer::time_descriptor.WriteData(&data, sizeof(TIMER_DATA), 0);

        return true;
    }
    
    void Timer::Clear()
    {
        Timer::time_descriptor.Clear();
    }

    void Timer::Update(uint8_t instance_id)
    {
        Timer::last_frame = Timer::now;
        Timer::now = std::chrono::system_clock::now();

        TIMER_DATA data = {
            static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(Timer::now - Timer::engine_start).count()),
            static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(Timer::now - Timer::last_frame).count()) / 1000000000.0f
        };

        Timer::time_descriptor.WriteData(&data, sizeof(TIMER_DATA), 0, 0, instance_id);
    }
}