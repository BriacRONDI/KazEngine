#pragma once

#include <cstring>
#include <array>

namespace Engine
{
    class Vector3;
    class Matrix4x4;

    class Vector4
    {
        public :
            union
            {
                struct 
                {
                    float x;
                    float y;
                    float z;
                    float w;
                };
                std::array<float, 4> xyzw;
            };

            // Distance entre le plan "this" et le point "point"
            float DistanceToPoint(Vector3 const& point);

            // Le point se trouve dans le demi-espace pointé par la normale au plan
            bool InsidePositiveHalfSpace(Vector3 const& point);

            // Produit scalaire
            inline float Dot(Vector4 const& other) { return this->x * other.x + this->y * other.y + this->z * other.z + this->w * other.w; }

            inline float& operator[](uint32_t index){ return this->xyzw[index]; }
            inline float operator[](uint32_t index) const { return this->xyzw[index]; }
            inline Vector4 operator*(float scalar) const { return {this->x * scalar, this->y * scalar, this->z * scalar, this->w * scalar}; }
            inline bool operator==(Vector4 const& other) const { return this->xyzw == other.xyzw; }
            Vector3 ToVector3();
            Vector4 operator*(Matrix4x4 const& matrix) const;
            Vector4 NormalizePlane();
            std::unique_ptr<char> Serialize() const;
            size_t Deserialize(const char* data);
    };
}