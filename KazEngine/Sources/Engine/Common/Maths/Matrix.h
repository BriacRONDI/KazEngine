#pragma once

#include "Maths.h"

namespace Engine
{
    class Vector3;

    class Matrix4x4
    {
        private :
            std::array<float,16> value;

        public :

            enum EULER_ORDER {ZYX, YZX, XZY, ZXY, YXZ, XYZ};

            Matrix4x4();
            Matrix4x4(Matrix4x4 const& other);
            Matrix4x4(std::array<float,16> const& other);
            Matrix4x4(Matrix4x4&& other);
            Matrix4x4(std::array<float,16>&& other);
            Matrix4x4(float x1, float y1, float z1, float w1,
                      float x2, float y2, float z2, float w2,
                      float x3, float y3, float z3, float w3,
                      float x4, float y4, float z4, float w4);
            inline float& operator[](uint32_t index) { return this->value[index]; }
            inline float operator[](uint32_t index) const { return this->value[index]; }
            inline Matrix4x4& operator=(Matrix4x4 const& other) { if(this != &other) this->value = other.value; return *this; }
            inline Matrix4x4& operator=(Matrix4x4&& other) { if(this != &other) this->value = std::move(other.value); return *this; }
            Matrix4x4 operator*(Matrix4x4 const& other) const;
            Vector4 operator*(Vector4 const& vertex) const;
            Matrix4x4 ExtractRotation() const;
            Matrix4x4 ExtractTranslation() const;
            void SetTranslation(Vector3 const& translation);
            Vector3 GetTranslation() const;
            Matrix4x4 ToTranslationMatrix() const;
            Matrix4x4 Inverse() const;
            Matrix4x4 Transpose() const;
            static Matrix4x4 PerspectiveProjectionMatrix(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip);
            static Matrix4x4 OrthographicProjectionMatrix(float const left_plane, float const right_plane, float const top_plane,
                                                          float const bottom_plane, float const near_plane, float const far_plane);
            static Matrix4x4 RotationMatrix(float angle, Vector3 const& axis, bool normalize_axis = false, bool to_radians = true);
            static Matrix4x4 TranslationMatrix(Vector3 const& vector);
            static Matrix4x4 EulerRotation(Matrix4x4 const& matrix, Vector3 const& vector, EULER_ORDER order);
            static Matrix4x4 ScalingMatrix(Vector3 const& vector);
    };
}