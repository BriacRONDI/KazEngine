#pragma once

#include <vector>
#include <fstream>
#include <vector>

#if defined(DISPLAY_LOGS)
#include <iostream>
#endif

namespace Engine
{
    namespace Tools
    {
        struct IMAGE_MAP {
            uint32_t width;
            uint32_t height;
            uint32_t format;
            std::vector<unsigned char> data;
        };

        std::vector<char> GetBinaryFileContents(std::string const& filename);
        bool WriteToFile(std::vector<char> const& data, std::string const &filename);
        std::string GetFileDirectory(std::string const& path);
        std::string GetFileName(std::string const& path);
        IMAGE_MAP LoadImageFile(std::string filename);
        IMAGE_MAP LoadImageData(std::vector<char> buffer);

        /**
         * Trick utilisé pour déterminer si le système hôte est Big Endian
         * Méthode plus propre si std c++20 activé
         * #include <bit>
         * this->is_big_endian = std::endian::native == std::endian::big;
         */
        inline bool IsBigEndian()
        {
            union {
                uint32_t i;
                char c[4];
            } val = {0x01020304};

            return val.c[0] == 1;
        }

        /////////////////////////////////////////////////
        // Échange des bytes de poids forts et faibles //
        // pour passer du little endian au big endian  //
        //            et réciproquement                //
        /////////////////////////////////////////////////

        static inline uint32_t BytesSwap(uint32_t value) { return _byteswap_ulong(value); }
        static inline uint64_t BytesSwap(uint64_t value) { return _byteswap_uint64(value); }
        static inline float BytesSwap(float value) {uint32_t swapped = _byteswap_ulong(*reinterpret_cast<uint32_t*>(&value)); return *reinterpret_cast<float*>(&swapped);}
        static inline int32_t BytesSwap(int32_t value) {uint32_t swapped = _byteswap_ulong(*reinterpret_cast<uint32_t*>(&value)); return *reinterpret_cast<int32_t*>(&swapped);}
        static inline double BytesSwap(double value) {uint64_t swapped = _byteswap_uint64(*reinterpret_cast<uint64_t*>(&value)); return *reinterpret_cast<double*>(&swapped);}
        static inline int64_t BytesSwap(int64_t value) {uint64_t swapped = _byteswap_uint64(*reinterpret_cast<uint64_t*>(&value)); return *reinterpret_cast<int64_t*>(&swapped);}
    }
}