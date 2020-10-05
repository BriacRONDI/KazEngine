#pragma once

#include "../../DescriptorSet/DescriptorSet.h"
#include "../Entity.h"

namespace Engine
{
    class StaticEntity : public Entity
    {
        public :
            
            StaticEntity();
            virtual inline void SetMatrix(Maths::Matrix4x4 matrix);
            virtual inline Maths::Matrix4x4 const& GetMatrix() const { return this->matrix; }

            static void Clear();
            static inline DescriptorSet const& GetMatrixDescriptor() { return StaticEntity::matrix_descriptor; }
            static bool Initialize();
            static inline bool UpdateMatrixDescriptor(uint8_t instance_id) { return StaticEntity::matrix_descriptor.Update(instance_id); }

        private :

            Maths::Matrix4x4 matrix;
            static DescriptorSet matrix_descriptor;
    };
}