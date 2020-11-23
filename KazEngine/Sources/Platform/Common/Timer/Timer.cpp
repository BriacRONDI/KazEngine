#include "Timer.h"

namespace Engine
{
    std::chrono::system_clock::time_point Timer::engine_start = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point Timer::now = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point Timer::last_frame;

    void Timer::Update(uint8_t instance_id)
    {
        Timer::last_frame = Timer::now;
        Timer::now = std::chrono::system_clock::now();

        TIMER_DATA data = {
            static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(Timer::now - Timer::engine_start).count()),
            static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(Timer::now - Timer::last_frame).count()) / 1000000000.0f
        };

        GlobalData::GetInstance()->time_descriptor.WriteData(&data, sizeof(TIMER_DATA), 0, 0, instance_id);
    }
}