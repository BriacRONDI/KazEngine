#pragma once

#include <chrono>
#include <EventEmitter.hpp>
#include "ITimerListener.h"
#include "../../../GlobalData/GlobalData.h"

#define DESCRIPTOR_BIND_NOW     static_cast<uint8_t>(0)
#define DESCRIPTOR_BIND_DELTA   static_cast<uint8_t>(1)

namespace Engine
{
    class Timer : public Tools::EventEmitter<ITimerListener>
    {
        public :

            static void Update(uint8_t instance_id);
            static std::chrono::milliseconds EngineStartDuration() { return std::chrono::duration_cast<std::chrono::milliseconds>(Timer::now - Timer::engine_start); }
            std::chrono::system_clock::time_point GetTime() { return Timer::now; }
            void Start(std::chrono::milliseconds total_duration = {}) { this->start = Timer::now; this->total_duration = total_duration; }
            std::chrono::milliseconds GetDuration() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->start); }
            float GetProgression() { return static_cast<float>(this->GetDuration().count()) / static_cast<float>(this->total_duration.count()); }

        private :

            struct TIMER_DATA {
                uint32_t now;   // Current time since engine start
                float delta;    // Seconds since last frame (may be < 1)
            };

            static std::chrono::system_clock::time_point engine_start;
            static std::chrono::system_clock::time_point now;
            static std::chrono::system_clock::time_point last_frame;
            std::chrono::system_clock::time_point start;
            std::chrono::milliseconds total_duration;
    };
}