#pragma once

#include <chrono>
#include <EventEmitter.hpp>
#include "ITimerListener.h"

namespace Engine
{
    class Timer : public Tools::EventEmitter<ITimerListener>
    {
        public :

            static inline void Update() { Timer::now = std::chrono::system_clock::now(); }
            inline std::chrono::system_clock::time_point GetTime() { return Timer::now; }
            inline void Start(std::chrono::milliseconds total_duration = {}) { this->start = Timer::now; this->total_duration = total_duration; }
            inline std::chrono::milliseconds GetDuration() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->start); }
            inline float GetProgression() { return static_cast<float>(this->GetDuration().count()) / static_cast<float>(this->total_duration.count()); }

        private :

            static std::chrono::system_clock::time_point now;
            std::chrono::system_clock::time_point start;
            std::chrono::milliseconds total_duration;
    };
}