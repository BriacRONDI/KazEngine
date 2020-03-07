#pragma once

#include <memory>
#include <vector>
#include "../../Common/Tools/Tools.h"
#include "Maths.h"

namespace Engine
{
    class Matrix4x4;

    class Vector3
    {
        public :

            union
            {
                struct 
                {
                    float x;
                    float y;
                    float z;
                };
                std::array<float, 3> xyz;
            };

            Vector3();
            Vector3(Vector3 const& other);
            Vector3(Vector3&& other);
            Vector3(float x, float y, float z);
            Vector3(std::array<double, 3> const& other);
            inline float& operator[](uint32_t index){ return this->xyz[index]; }
            inline float operator[](uint32_t index) const { return this->xyz[index]; }
            Vector3& operator=(Vector3 const& other);
            Vector3& operator=(Vector3&& other);
            inline bool operator==(Vector3 const& other) const { return this->xyz == other.xyz; }
            Vector3 operator*(Matrix4x4 const& matrix) const;
            float Dot(Vector3 const& other) const;
            Vector3 Cross(Vector3 const& other) const;
            inline Vector3 operator*(float const scalar) const { return { this->x * scalar, this->y * scalar, this->z * scalar }; }
            Vector3 operator+(Vector3 const& other) const;
            Vector3 operator-(Vector3 const& other) const;
            Vector3 Normalize() const;
            Vector3 ToRadians() const;
            float Length() const;
            std::unique_ptr<char> Serialize() const;
            size_t Deserialize(const char* data);
            static Vector3 Interpolate(Vector3 const& source, Vector3 const& dest, float ratio);
    };
}