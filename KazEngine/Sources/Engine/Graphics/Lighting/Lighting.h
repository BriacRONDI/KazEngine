#pragma once

#include "../../Common/Maths/Maths.h"

namespace Engine
{
    class Lighting
    {
        public :

            struct LIGHTING_UBO {
                Vector3 color;
                int padding;
	            Vector3 position;
	            float ambient_strength;
	            float specular_strength;
            };

            struct LIGHTING_VIEW {
                Vector3& color;
	            Vector3& position;
	            float& ambient_strength;
	            float& specular_strength;
            };

            LIGHTING_VIEW light = {
                this->uniform_buffer.color, 
                this->uniform_buffer.position, 
                this->uniform_buffer.ambient_strength, 
                this->uniform_buffer.specular_strength
            };

            inline LIGHTING_UBO& GetUniformBuffer() { return this->uniform_buffer; }

        private :

            LIGHTING_UBO uniform_buffer;
            
    };
}