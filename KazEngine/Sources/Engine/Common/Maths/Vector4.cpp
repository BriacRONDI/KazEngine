#include "Vector4.h"
#include "Vector3.h"

namespace Engine
{
    /**
    * Multiplication par une matrice
    */
    Vector4 Vector4::operator*(Matrix4x4 const& matrix) const
    {
        return {
            this->x * matrix[0] + this->y * matrix[1] + this->z * matrix[2] + this->w * matrix[3],
            this->x * matrix[4] + this->y * matrix[5] + this->z * matrix[6] + this->w * matrix[7],
            this->x * matrix[8] + this->y * matrix[9] + this->z * matrix[10] + this->w * matrix[11],
            this->x * matrix[12] + this->y * matrix[13] + this->z * matrix[14] + this->w * matrix[15]
        };
    }

    float Vector4::DistanceToPoint(Vector3 const& point)
    {
        // return this->x * point.x + this->y * point.y + this->z * point.z + this->w;
        float value = this->x * point.x + this->y * point.y + this->z * point.z + this->w;
        return value;
    }

    bool Vector4::InsidePositiveHalfSpace(Vector3 const& point)
    {
        // return this->DistanceToPoint(point) >= 0;
        float distance = this->DistanceToPoint(point);
        return distance >= 0;
    }

    /**
     * Le vector4 est vu comme un plan et est normalisé comme tel
     */
    Vector4 Vector4::NormalizePlane()
    {
        float module = std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z);

        return {
            this->x / module,
            this->y / module,
            this->z / module,
            this->w / module
        };
    }

    /**
     * Désérialization
     */
    size_t Vector4::Deserialize(const char* data)
    {
        *this = *reinterpret_cast<const Vector4*>(data);
        return sizeof(Vector4);
    }

    /**
     * Sérialisation
     */
    std::unique_ptr<char> Vector4::Serialize() const
    {
        std::unique_ptr<char> output(new char[sizeof(Vector4)]);
        *reinterpret_cast<Vector4*>(output.get()) = *this;
        return output;
    }
}