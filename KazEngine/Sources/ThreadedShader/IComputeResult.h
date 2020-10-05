#pragma once

#include "../Vulkan/Vulkan.h"
#include "TheadedShader.h"

namespace Engine
{
    class IComputeResult
    {
        public :
            virtual void ComputeFinished(ThreadedShader* thread) = 0;
    };
}