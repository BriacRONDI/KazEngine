#include "Timer.h"

namespace Engine
{
    std::chrono::system_clock::time_point Timer::now = std::chrono::system_clock::now();
}