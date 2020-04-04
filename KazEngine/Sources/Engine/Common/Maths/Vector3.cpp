#include "Vector3.h"

namespace Engine
{
    Vector3::Vector3() : xyz({}) {}
    Vector3::Vector3(Vector3 const& other) : xyz(other.xyz) {}
    Vector3::Vector3(Vector3&& other) : xyz(std::move(other.xyz)) {}
    Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3::Vector3(std::array<double, 3> const& other) : x(static_cast<float>(other[0])), y(static_cast<float>(other[1])), z(static_cast<float>(other[2])) {}

    /**
    * Affectation par copie
    */
    Vector3& Vector3::operator=(Vector3 const& other)
    {
        if(this != &other) this->xyz = other.xyz;
        return *this;
    }

    /**
    * Affectation par déplacement
    */
    Vector3& Vector3::operator=(Vector3&& other)
    {
        if(this != &other) this->xyz = std::move(other.xyz);
        return *this;
    }

    /**
    * Multiplication par une matrice
    */
    /*Vector3 Vector3::operator*(Matrix4x4 const& matrix) const
    {
        return {
            this->x * matrix[0] + this->y * matrix[1] + this->z * matrix[2],
            this->x * matrix[4] + this->y * matrix[5] + this->z * matrix[6],
            this->x * matrix[8] + this->y * matrix[9] + this->z * matrix[10]
        };
    }*/

    /**
    * Addition de vecteurs
    */
    Vector3 Vector3::operator+(Vector3 const& other) const
    {
        return {
            this->x + other.x,
            this->y + other.y,
            this->z + other.z
        };
    }

    /**
    * Soustraction de vecteurs
    */
    Vector3 Vector3::operator-(Vector3 const& other) const
    {
        return {
            this->x - other.x,
            this->y - other.y,
            this->z - other.z
        };
    }

    /**
    * Fixe le module d'un vecteur à 1
    */
    Vector3 Vector3::Normalize() const
    {
        float length = std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
        return {
            this->x / length,
            this->y / length,
            this->z / length
        };
    }

    /**
     * Change les coordonnées en radians
     */
    Vector3 Vector3::ToRadians() const
    {
        return {
            this->x * DEGREES_TO_RADIANS,
            this->y * DEGREES_TO_RADIANS,
            this->z * DEGREES_TO_RADIANS
        };
    }

    /**
     * Calcule le module du vecteur
     */
    float Vector3::Length() const
    {
        return std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
    }

    /**
     * Calcule la valeurs du vecteur allant de la source à la destination,
     * en utilisant le ratio comme point de progression.
     */
    Vector3 Vector3::Interpolate(Vector3 const& source, Vector3 const& dest, float ratio)
    {
        Vector3 result;

        for(uint8_t i=0; i<3; i++) {
            if(source[i] <= dest[i]) result[i] = source[i] + std::abs(dest[i] - source[i]) * ratio;
            else result[i] = source[i] - std::abs(dest[i] - source[i]) * ratio;
        }

        return result;
    }

    /**
     * Désérialization
     */
    size_t Vector3::Deserialize(const char* data)
    {
        if(Tools::IsBigEndian()) {
            for(uint8_t i=0; i<3; i++) {
                float value = *reinterpret_cast<const float*>(data + i * sizeof(float));
                this->xyz[i] = Tools::BytesSwap(value);
            }
        }else{
            *this = *reinterpret_cast<const Vector3*>(data);
        }

        return sizeof(Vector3);
    }

    /**
     * Sérialisation
     */
    std::unique_ptr<char> Vector3::Serialize() const
    {
        std::unique_ptr<char> output(new char[sizeof(Vector3)]);
        if(Tools::IsBigEndian()) {
            Vector3 swapped;
            for(uint8_t i=0; i<this->xyz.size(); i++)
                swapped.xyz[i] = Tools::BytesSwap(this->xyz[i]);
            std::memcpy(output.get(), &swapped, sizeof(Vector3));
        }else{
            *reinterpret_cast<Vector3*>(output.get()) = *this;
        }

        return output;
    }
}