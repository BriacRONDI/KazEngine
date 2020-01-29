#pragma once

#include <string>
#include <vector>
#include <sstream>

#include "../Maths/Maths.h"
#include "../Tools/Tools.h"

#define ROOT_PACKER_NODE(size) {Engine::Packer::DATA_TYPE::ROOT_NODE,size,0,{},{}}

namespace Engine
{
    class Packer
    {
        public :

            enum DATA_TYPE : uint8_t
            {
                UNDEFINED       = 0,
                ROOT_NODE       = 1,
                PARENT_NODE     = 2,
                STRING          = 3,
                BINARY_DATA     = 4
            };
            
            struct DATA
            {
                DATA_TYPE type;                 // Type de donnée
                uint32_t size;                  // Taille des données sans l'entête
                uint32_t position;              // Position de l'entête
                std::string name;               // Nom de la donnée (contenu dans l'entête)
                std::vector<DATA> children;     // Sous-données si le noeud est de type PARENT_NODE

                DATA() : type(DATA_TYPE::UNDEFINED), size(0), position(0), name(), children() {}
                DATA(DATA const& other) : type(other.type), size(other.size), position(other.position), name(other.name), children(other.children) {}
                DATA(DATA_TYPE type, uint32_t size, uint32_t position, std::string const& name, std::vector<DATA> const& children) : type(type), size(size), position(position), name(name), children(children) {}
                DATA(DATA&& other) {*this = std::move(other);}
                DATA& operator=(DATA&& other);
                std::vector<char> Serialize() const;
                inline uint32_t HeaderSize() const;
            };

            static std::vector<DATA> UnpackMemory(std::vector<char> const& memory, uint32_t position = 0, uint32_t size = 0);
            static bool PackToMemory(std::vector<char>& memory, std::string const& path, DATA_TYPE const type,
                                     std::string const& name, std::unique_ptr<char> const& data, uint32_t data_size);
            static std::string ParseString(std::vector<char> const& memory, uint32_t position, uint32_t size);

            static std::vector<Vector3> ParseVector3Array(const char* data, size_t size);
            static std::vector<Vector2> ParseVector2Array(const char* data, size_t size);
            static std::vector<uint32_t> ParseUint32Array(const char* data, size_t size);
            
            static std::vector<char> SerializeVector3Array(std::vector<Vector3> const& array);
            static std::vector<char> SerializeVector2Array(std::vector<Vector2> const& array);
            static std::vector<char> SerializeUint32Array(std::vector<uint32_t> const& array);
            
            static DATA FindPackedItem(std::vector<DATA> const& package, std::string path);
            static void UpdatePackSize(std::vector<char>& memory, std::vector<DATA> const& package, std::string path, int32_t size_change);
            
            static std::vector<char> SerializeUint32(uint32_t value);
            static uint32_t ParseUint32(std::vector<char> const& memory, const size_t position);
            static int32_t ParseInt32(std::vector<char> const& memory, const size_t position);
            static uint64_t ParseUint64(std::vector<char> const& memory, const size_t position);
            static int64_t ParseInt64(std::vector<char> const& memory, const size_t position);
            static int16_t ParseInt16(std::vector<char> const& memory, const size_t position);
            static double ParseFloat64(std::vector<char> const& memory, const size_t position);
    };
}