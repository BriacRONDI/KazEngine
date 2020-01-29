#include "Vector4.h"

namespace Engine
{
    /**
     * Désérialization
     */
    size_t Vector4::Deserialize(const char* data)
    {
        if(Tools::IsBigEndian()) {
            for(uint8_t i=0; i<this->xyzw.size(); i++) {
                float value = *reinterpret_cast<const float*>(data + i * sizeof(float));
                this->xyzw[i] = Tools::BytesSwap(value);
            }
        }else{
            *this = *reinterpret_cast<const Vector4*>(data);
        }

        return sizeof(Vector4);
    }

    /**
     * Sérialisation
     */
    std::unique_ptr<char> Vector4::Serialize() const
    {
        std::unique_ptr<char> output(new char[sizeof(Vector4)]);
        if(Tools::IsBigEndian()) {
            Vector4 swapped;
            for(uint8_t i=0; i<this->xyzw.size(); i++)
                swapped.xyzw[i] = Tools::BytesSwap(this->xyzw[i]);
            std::memcpy(output.get(), &swapped, sizeof(Vector4));
        }else{
            *reinterpret_cast<Vector4*>(output.get()) = *this;
        }

        return output;
    }
}