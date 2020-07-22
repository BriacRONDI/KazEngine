#pragma once

#include <vector>
#include <string>
#include <Tools.h>
#include "../../Maths/Sources/Maths.h"

namespace DataPacker
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
                BINARY_DATA     = 4,
                BONE_TREE       = 5,
                IMAGE_FILE      = 6,
                MESH_DATA       = 7,
                MATERIAL_DATA   = 8,
                MODEL_TREE      = 9
            };
            
            struct DATA
            {
                /// Data type
                DATA_TYPE type;

                /// Data size
                uint32_t size;

                /// Header first byte position
                uint32_t position;

                /// Data name
                std::string name;

                /// Object dependancies
                std::vector<std::string> dependancies;

                /// Children data if this node is parent
                std::vector<DATA> children;

                /// Initialize to empty data
                DATA() : type(DATA_TYPE::UNDEFINED), size(0), position(0) {}

                /// Copy constructor
                DATA(DATA const& other) : type(other.type), size(other.size), position(other.position), name(other.name), dependancies(other.dependancies), children(other.children) {}

                /// Explicit values constructor
                DATA(DATA_TYPE type, uint32_t size, uint32_t position, std::string const& name, std::vector<std::string> dependancies, std::vector<DATA> const& children) : type(type), size(size), position(position), name(name), dependancies(dependancies), children(children) {}
                
                /// Move constructor
                DATA(DATA&& other) { *this = std::move(other); }

                /// Move assignment
                DATA& operator=(DATA&& other);

                /// Copy assignment
                DATA& operator=(DATA const& other);

                /// Convert this struct to serialized string
                std::vector<char> Serialize() const;

                /// Compute serialized header dependancies size
                inline uint32_t HeaderDependanciesSize() const { size_t size = 0; for(auto& dep : this->dependancies) size += dep.size(); return static_cast<uint32_t>(sizeof(uint8_t) * this->dependancies.size() + size + 1); }

                /// Compute serialized header size
                inline uint32_t HeaderSize() const { return this->type == DATA_TYPE::ROOT_NODE ? 0 : static_cast<uint32_t>(sizeof(DATA_TYPE) + sizeof(uint8_t) + this->name.size() + sizeof(uint32_t) + this->HeaderDependanciesSize()); }

                /// Get a pointer on the target buffer
                inline const char* Data(const char* buffer) { return buffer + this->position + this->HeaderSize(); }
            };

            /**
             * Extract data table from raw buffer
             * @param memory Data buffer
             * @param position Parse data from this position
             * @param size Amount of data to parse, if size = 0 for no limit
             * @return Data table
             */
            static std::vector<DATA> UnpackMemory(std::vector<char> const& memory, uint32_t position = 0, uint32_t size = 0);

            /**
             * Insert serialized data inside a packaged data buffer
             * @param memory Data buffer
             * @param path Destination node
             * @param type Insertion type
             * @param name Insertion name
             * @param data Serialized data to insert
             * @param data_size Serialized data size
             * @param dependancies Object dependancies
             * @retval true Success
             * @retval false Data not inserted
             */
            static bool PackToMemory(std::vector<char>& memory, std::string const& path, DATA_TYPE const type,
                                     std::string const& name, std::unique_ptr<char> const& data, uint32_t data_size,
                                     std::vector<std::string> dependancies = {});

            /**
             * Change the name of the specified node
             * @param memory Data buffer
             * @param path Specified node
             * @param name New name
             */
            static void SetNodeName(std::vector<char>& memory, std::string const& path, std::string const& name);

            /**
             * Removes the specified node
             * @param memory Data buffer
             * @param path Target node path
             */
            static void RemoveNode(std::vector<char>& memory, std::string const& path);

            /**
             * Moves a node from a container to an other.
             *  - Root node cannot be moved.
             *  - A node cannot be moved to anything but a container.
             *  - A node cannot be moved to its subtree.
             *  - Duplicate names are note allowed inside a container.
             * @param memory Data buffer
             * @param source_path Node to move
             * @param dest_path Destination container
             * @retval true Data has changed
             * @retval false Data not changed
             */
            static bool MoveNode(std::vector<char>& memory, std::string const& source_path, std::string const& dest_path);

            /**
             * Get the data type of specified node
             * @param memory Data buffer
             * @param path Target node path
             * @return Data type
             */
            static DATA_TYPE GetNodeType(std::vector<char>& memory, std::string const& path);

            /**
             * Change the type of the specified node
             * @param memory Data buffer
             * @param path Specified node
             * @param type New type
             */
            static void SetNodeType(std::vector<char>& memory, std::string const& path, DATA_TYPE type);

            /**
             * Search for serialized data inside a packaged buffer at desired location
             * @param package Packaged buffer
             * @param path Search location
             * @return Information about data if found, undefined otherwise
             */
            static DATA& FindPackedItem(std::vector<DATA>& package, std::string path);

            /**
             * Determines if the given type is a container
             * @param type Type to check
             * @retval true Type is a container
             * @retval false Type is not a container
             */
            static inline bool IsContainer(DATA_TYPE type) { return type == DATA_TYPE::ROOT_NODE || type == DATA_TYPE::PARENT_NODE || type == DATA_TYPE::MODEL_TREE; }
            
            /**
             * Repair broken references to a modified path
             * @param memory Data buffer
             * @param old_path Broken dependancy path
             * @param new_path New dependancy path
             * @param package Unpacked memory package
             * @param parent_path Path to target node to update
             */
            static void FixDependancies(std::vector<char>& memory, std::string old_path, std::string new_path, std::vector<DATA> package, std::string parent_path = "/");

            /**
             * Change the value of the specified dependancy
             * @param memory Data buffer
             * @param path Specified node
             * @param index Dependancy index
             * @param value Dependancy value
             */
            static void SetNodeDependancy(std::vector<char>& memory, std::string path, uint8_t index, std::string value);

            static inline std::vector<char> SerializeUint32(uint32_t value) { return std::vector<char>(reinterpret_cast<char*>(&value), reinterpret_cast<char*>(&value) + sizeof(uint32_t)); }
            static std::vector<char> SerializeVector3Array(std::vector<Maths::Vector3> const& array);
            static std::vector<char> SerializeVector2Array(std::vector<Maths::Vector2> const& array);
            static std::vector<char> SerializeUint32Array(std::vector<uint32_t> const& array);

            static std::string ParseString(std::vector<char> const& memory, uint32_t position, uint32_t size);
            static std::vector<Maths::Vector3> ParseVector3Array(const char* data, size_t size);
            static std::vector<Maths::Vector2> ParseVector2Array(const char* data, size_t size);
            static std::vector<uint32_t> ParseUint32Array(const char* data, size_t size);

            static inline uint32_t ParseUint32(std::vector<char> const& memory, const size_t position) { return *reinterpret_cast<const uint32_t*>(&memory[position]); }
            static inline int32_t ParseInt32(std::vector<char> const& memory, const size_t position) { return *reinterpret_cast<const int32_t*>(&memory[position]); }
            static inline uint64_t ParseUint64(std::vector<char> const& memory, const size_t position) { return *reinterpret_cast<const uint64_t*>(&memory[position]); }
            static inline int64_t ParseInt64(std::vector<char> const& memory, const size_t position) { return *reinterpret_cast<const int64_t*>(&memory[position]); }
            static inline int16_t ParseInt16(std::vector<char> const& memory, const size_t position) { return *reinterpret_cast<const int16_t*>(&memory[position]); }
            static double ParseFloat64(std::vector<char> const& memory, const size_t position) { return *reinterpret_cast<const double*>(&memory[position]); }

        private :

            /**
             * Update parent node sizes in case of data insertion/removal
             * @param memory Data buffer
             * @param package Data table of packaged buffer, obtained with UnpackMemory()
             * @param path Path of modified data
             * @param size_change Size modification (positive or negative)
             */
            static void UpdatePackSize(std::vector<char>& memory, std::vector<DATA> const& package, std::string path, int32_t size_change);
    };
}