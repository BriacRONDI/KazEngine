#include "Vector2.h"

namespace Engine
{
    /**
     * Désérialization
     */
    size_t Vector2::Deserialize(const char* data)
    {
        if(Tools::IsBigEndian()) {
            for(uint8_t i=0; i<this->xy.size(); i++) {
                float value = *reinterpret_cast<const float*>(data + i * sizeof(float));
                this->xy[i] = Tools::BytesSwap(value);
            }
        }else{
            *this = *reinterpret_cast<const Vector2*>(data);
        }

        return sizeof(Vector2);
    }

    /**
     * Sérialisation
     */
    std::unique_ptr<char> Vector2::Serialize() const
    {
        std::unique_ptr<char> output(new char[sizeof(Vector2)]);
        if(Tools::IsBigEndian()) {
            Vector2 swapped;
            for(uint8_t i=0; i<this->xy.size(); i++)
                swapped.xy[i] = Tools::BytesSwap(this->xy[i]);
            std::memcpy(output.get(), &swapped, sizeof(Vector2));
        }else{
            *reinterpret_cast<Vector2*>(output.get()) = *this;
        }

        return output;
    }
}