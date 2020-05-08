#include "DataPacker.h"

namespace DataPacker
{
    Packer::DATA& Packer::DATA::operator=(DATA&& other)
    {
        if(&other != this) {
            this->type = other.type;
            this->size = other.size;
            this->position = other.position;
            this->name = std::move(other.name);
            this->children = std::move(other.children);
            other.type = DATA_TYPE::UNDEFINED;
            other.size = 0;
            other.position = 0;
        }

        return *this;
    }

    Packer::DATA& Packer::DATA::operator=(DATA const& other)
    {
        if(this != &other) {
            this->type = other.type;
            this->size = other.size;
            this->position = other.position;
            this->name = other.name;
            this->children = other.children;
        }
        return *this;
    }

    std::vector<char> Packer::DATA::Serialize() const
    {
        std::vector<char> output;
        
        // Type
        output.insert(output.end(), &this->type, &this->type + sizeof(DATA_TYPE));

        // Name
        uint8_t name_length = static_cast<uint8_t>(this->name.size());
        output.insert(output.end(), &name_length, &name_length + sizeof(uint8_t));
        output.insert(output.end(), this->name.begin(), this->name.end());

        // Data size
        std::vector<char> serialized_size = Packer::SerializeUint32(this->size);
        output.insert(output.end(), serialized_size.begin(), serialized_size.end());

        // Serialize nested nodes
        for(auto& child : this->children) {
            std::vector<char> serialized_child = child.Serialize();
            output.insert(output.end(), serialized_child.begin(), serialized_child.end());
        }
        
        return output;
    }

    std::vector<Packer::DATA> Packer::UnpackMemory(std::vector<char> const& memory, uint32_t position, uint32_t size)
    {
        std::vector<DATA> package;

        uint32_t final_position;
        if(!position && !size && memory.size() > 0) final_position = static_cast<uint32_t>(memory.size());
        else final_position = position + size;
        
        while(position < final_position) {

            // Store node position
            uint32_t node_position = position;
            
            // First byte = Type
            DATA_TYPE type = static_cast<DATA_TYPE>(memory[position]);
            position++;

            // Name length
            uint8_t name_length = memory[position];
            position++;

            // Name
            std::string name;
            if(name_length > 0) name = std::string(memory.begin() + position, memory.begin() + position + name_length);
            position += name_length;

            // Data size
            uint32_t data_size = *reinterpret_cast<const uint32_t*>(&memory[position]);
            position += sizeof(uint32_t);

            // Unpack child nodes
            std::vector<DATA> children;
            if(Packer::IsContainer(type)) children = Packer::UnpackMemory(memory, position, data_size);

            // Add unpacked data to result vector
            package.push_back({type, data_size, node_position, name, children});

            // Unpack next node
            position += data_size;
        }

        return package;
    }

    bool Packer::PackToMemory(std::vector<char>& memory, std::string const& path, DATA_TYPE const type,
                              std::string const& name, std::unique_ptr<char> const& data, uint32_t data_size)
    {
        // Read data table
        std::vector<DATA> package;
        if(memory.size() > 0) package = Packer::UnpackMemory(memory);

        // Find data in table
        DATA pack = Packer::FindPackedItem(package, path);

        // Only PARENT_NODE or ROOT_NODE can contain data
        if(pack.type != DATA_TYPE::PARENT_NODE && pack.type != DATA_TYPE::ROOT_NODE) return false;

        // Create the new node to insert
        DATA node;
        node.name = name;
        node.type = type;
        node.size = data_size;
        if(pack.type == DATA_TYPE::ROOT_NODE) node.position = pack.size;
        else node.position = pack.position + pack.HeaderSize() + pack.size;

        // Insert the node
        std::vector<char> serialized_node = node.Serialize();
        memory.insert(memory.begin() + node.position, serialized_node.begin(), serialized_node.end());

        // Insert data if needed
        if(node.size) memory.insert(memory.begin() + node.position + serialized_node.size(), data.get(), data.get() + data_size);

        // Update parents size
        Packer::UpdatePackSize(memory, package, path, static_cast<uint32_t>(serialized_node.size() + data_size));

        return true;
    }

    void Packer::SetNodeName(std::vector<char>& memory, std::string const& path, std::string const& name)
    {
        // Read data table
        std::vector<DATA> package;
        if(memory.size() > 0) package = Packer::UnpackMemory(memory);
        else return;

        // Find data in table
        DATA& pack = Packer::FindPackedItem(package, path);

        // Compute name position inside buffer
        uint32_t name_position = pack.position + sizeof(DATA_TYPE) + sizeof(uint8_t);

        // Erase old name
        memory.erase(memory.begin() + name_position, memory.begin() + name_position + pack.name.size());

        // Insert new name
        memory.insert(memory.begin() + name_position, name.begin(), name.end());

        // Change name size
        memory[pack.position + sizeof(DATA_TYPE)] = static_cast<uint8_t>(name.size());

        // Path is root
        if(!path.size() || path == "/") return;

        // Update parents size if necessary
        int32_t size_change = static_cast<int32_t>(name.size() - pack.name.size());
        pack.name = name;

        std::string parent_path = path;
        if(parent_path[parent_path.size() - 1] == '/') parent_path = parent_path.substr(0, parent_path.size() - 1);
        size_t pos = parent_path.find_last_of('/');
        
        // Path is direct root child => no parent
        if(pos == 0) return;

        parent_path = parent_path.substr(0, pos);
        Packer::UpdatePackSize(memory, package, parent_path, size_change);
    }

    void Packer::SetNodeType(std::vector<char>& memory, std::string const& path, DATA_TYPE type)
    {
        // Read data table
        std::vector<DATA> package;
        if(memory.size() > 0) package = Packer::UnpackMemory(memory);
        else return;

        // Find data in table
        DATA& pack = Packer::FindPackedItem(package, path);

        // Change type
        memory[pack.position] = type;
    }

    void Packer::RemoveNode(std::vector<char>& memory, std::string const& path)
    {
        // If path is root, just clear all data
        if(!path.size() || path == "/") {
            memory.clear();
            return;
        }

        // Read data table
        std::vector<DATA> package;
        if(memory.size() > 0) package = Packer::UnpackMemory(memory);
        else return;

        // Find data in table
        DATA& pack = Packer::FindPackedItem(package, path);

        // Compute node size
        int32_t node_size = pack.HeaderSize() + pack.size;

        // Remove complete node
        memory.erase(memory.begin() + pack.position, memory.begin() + pack.position + node_size);

        // Update parents size if necessary
        std::string parent_path = path;
        if(parent_path[parent_path.size() - 1] == '/') parent_path = parent_path.substr(0, parent_path.size() - 1);
        size_t pos = parent_path.find_last_of('/');
        
        // Path is direct root child => no parent
        if(pos == 0) return;

        parent_path = parent_path.substr(0, pos);
        Packer::UpdatePackSize(memory, package, parent_path, -node_size);
    }

    bool Packer::MoveNode(std::vector<char>& memory, std::string const& source_path, std::string const& dest_path)
    {
        /////////////////////////////////
        // Check if operation is valid //
        /////////////////////////////////

        // Root cannot be moved
        if(!source_path.size() || source_path == "/") return false;

        // Node stay at the same place
        if(source_path == dest_path) return false;

        // A node cannot be moved to its subtree
        if(dest_path.find(source_path) == 0) return false;

        // Read data table
        std::vector<DATA> package;
        if(memory.size() > 0) package = Packer::UnpackMemory(memory);
        else return false;

        // Find destination in table
        DATA dest_node = Packer::FindPackedItem(package, dest_path);

        // Destination must be a container node
        if(!Packer::IsContainer(dest_node.type)) return false;

        // Find source node container path
        std::string parent_path = source_path;
        if(parent_path[parent_path.size() - 1] == '/') parent_path = parent_path.substr(0, parent_path.size() - 1);
        size_t pos = parent_path.find_last_of('/');

        bool root_child = pos == 0;
        if(!root_child) parent_path = parent_path.substr(0, pos);

        // The node is moved to its own container
        if(parent_path == dest_path || root_child && dest_node.type == DATA_TYPE::ROOT_NODE) return false;

        // Find source node in table
        DATA source_node = Packer::FindPackedItem(package, source_path);

        // Source node not found
        if(source_node.type == DATA_TYPE::UNDEFINED) return false;

        // Name already exist in target directory
        std::string dest_duplicate_path = dest_path;
        if(dest_path[dest_path.size() - 1] == '/') dest_duplicate_path = dest_duplicate_path.substr(0, dest_duplicate_path.size() - 1);
        DATA dest_duplicate_node = Packer::FindPackedItem(package, dest_duplicate_path + '/' + source_node.name);
        if(dest_duplicate_node.type != DATA_TYPE::UNDEFINED) return false;

        ///////////////
        // Move node //
        ///////////////

        // Compute destination position and copy size
        uint32_t dest_position = dest_node.position + dest_node.HeaderSize() + dest_node.size;
        int32_t copy_size = source_node.HeaderSize() + source_node.size;

        
        if(dest_position < source_node.position) {
            // If data must be moved before its old position, then we have to copy data inside a temporary buffer before copy it,
            // because data can't be moved directly from a buffer to itself, this may cause undefined behavior
            std::vector<char> temp_buffer(memory.begin() + source_node.position, memory.begin() + source_node.position + copy_size);
            memory.insert(memory.begin() + dest_position, temp_buffer.begin(), temp_buffer.end());

        }else{
            // Direct move
            memory.insert(memory.begin() + dest_position, memory.begin() + source_node.position, memory.begin() + source_node.position + copy_size);
        }

        // Update destination size, as if it were a simple insertion
        Packer::UpdatePackSize(memory, package, dest_path, copy_size);

        // If source node data is placed after destination, then its position has now changed and must be refreshed
        if(dest_position < source_node.position) {
            package = Packer::UnpackMemory(memory);
            source_node = Packer::FindPackedItem(package, source_path);
        }

        // Remove source node
        memory.erase(memory.begin() + source_node.position, memory.begin() + source_node.position + copy_size);

        // Update source parent size
        if(root_child) return true;
        else Packer::UpdatePackSize(memory, package, parent_path, -copy_size);

        return true;
    }

    Packer::DATA_TYPE Packer::GetNodeType(std::vector<char>& memory, std::string const& path)
    {
        // Path is root
        if(!path.size() || path == "/") return DATA_TYPE::ROOT_NODE;

        // Read data table
        std::vector<DATA> package;
        if(memory.size() > 0) package = Packer::UnpackMemory(memory);
        else return DATA_TYPE::UNDEFINED;

        // Find data in table
        DATA& pack = Packer::FindPackedItem(package, path);

        return pack.type;
    }

    void Packer::UpdatePackSize(std::vector<char>& memory, std::vector<DATA> const& package, std::string path, int32_t size_change)
    {
        // Path is root
        if(!path.size() || path == "/") return;

        // Take first element of path
        size_t pos = path.find_first_of('/');
        if(pos == 0) {
            path = path.substr(1);
            pos = path.find_first_of('/');
        }

        std::string name;
        if(pos == std::string::npos) name = path;
        else name = path.substr(0, pos);

        // Seek first element of path
        for(DATA const& pack : package) {

            // Element found
            if(pack.name == name) {

                // Update node size
                uint32_t size_position = static_cast<uint32_t>(pack.position + sizeof(DATA_TYPE) + sizeof(uint8_t) + pack.name.size());
                uint32_t new_size = pack.size + size_change;
                *reinterpret_cast<uint32_t*>(memory.data() + size_position) = new_size;

                // If end of path is reached, return
                if(pos == std::string::npos) return;

                // If end of path is not reached, update children sizes
                if(pack.children.size() > 0) Packer::UpdatePackSize(memory, pack.children, path.substr(pos + 1), size_change);
            }
        }
    }

    Packer::DATA& Packer::FindPackedItem(std::vector<DATA>& package, std::string path)
    {
        // Return this in case of failure
        static Packer::DATA default_node = {};
        default_node.size = 0;
        default_node.type = DATA_TYPE::UNDEFINED;

        // Path is root
        if(!path.size() || path == "/") {
            uint32_t root_size = 0;
            for(auto& pack : package) root_size += pack.HeaderSize() + pack.size;
            default_node.size = root_size;
            default_node.type = DATA_TYPE::ROOT_NODE;
            return default_node;
        }

        // Take first element of path
        size_t pos = path.find_first_of('/');
        if(pos == 0) {
            path = path.substr(1);
            pos = path.find_first_of('/');
        }

        std::string name;
        if(pos == std::string::npos) name = path;
        else name = path.substr(0, pos);

        // Seek first element of path
        for(DATA& pack : package) {

            // Element found
            if(pack.name == name) {

                // If end of path is reached, return element
                if(pos == std::string::npos) return pack;

                // If end of path is not reached, explore children
                if(pack.children.size() > 0) {
                    DATA& child = Packer::FindPackedItem(pack.children, path.substr(pos + 1));
                    if(child.type != DATA_TYPE::UNDEFINED) return child;
                }
            }
        }

        // No element found
        return default_node;
    }

    std::string Packer::ParseString(std::vector<char> const& memory, uint32_t position, uint32_t size)
    {
        // Return empty string if size is zero
        if(!size) return std::string();

        // Build string
        return std::string(memory.begin() + position, memory.begin() + position + size);
    }

    std::vector<Maths::Vector3> Packer::ParseVector3Array(const char* data, size_t size)
    {
        std::vector<Maths::Vector3> output;

        // Buffer allocation
        uint32_t vector_count = static_cast<uint32_t>(size / sizeof(Maths::Vector3));
        if(vector_count * sizeof(Maths::Vector3) != size) return output;
        output.resize(vector_count);

        // Read data
        std::memcpy(output.data(), data, size);

        return output;
    }

    std::vector<Maths::Vector2> Packer::ParseVector2Array(const char* data, size_t size)
    {
        std::vector<Maths::Vector2> output;

        // Buffer allocation
        uint32_t vector_count = static_cast<uint32_t>(size / sizeof(Maths::Vector2));
        if(vector_count * sizeof(Maths::Vector2) != size) return output;
        output.resize(vector_count);

        // Read data
        std::memcpy(output.data(), data, size);

        return output;
    }

    std::vector<uint32_t> Packer::ParseUint32Array(const char* data, size_t size)
    {
        std::vector<uint32_t> output;

        // Buffer allocation
        uint32_t count = static_cast<uint32_t>(size / sizeof(uint32_t));
        if(count * sizeof(uint32_t) != size) return output;
        output.resize(count);

        // Read data
        std::memcpy(output.data(), data, size);

        return output;
    }

    std::vector<char> Packer::SerializeVector3Array(std::vector<Maths::Vector3> const& array)
    {
        size_t buffer_size = array.size() * sizeof(Maths::Vector3);
        std::vector<char> output(buffer_size);
        std::memcpy(output.data(), array.data(), buffer_size);

        return output;
    }

    std::vector<char> Packer::SerializeVector2Array(std::vector<Maths::Vector2> const& array)
    {
        size_t buffer_size = array.size() * sizeof(Maths::Vector2);
        std::vector<char> output(buffer_size);
        std::memcpy(output.data(), array.data(), buffer_size);

        return output;
    }

    std::vector<char> Packer::SerializeUint32Array(std::vector<uint32_t> const& array)
    {
        size_t buffer_size = array.size() * sizeof(uint32_t);
        std::vector<char> output(buffer_size);
        std::memcpy(output.data(), array.data(), buffer_size);
        
        return output;
    }
}