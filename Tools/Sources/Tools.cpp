#include "Tools.h"

#if !defined(STB_IMAGE_IMPLEMENTATION)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

namespace Tools
{
    std::vector<char> GetBinaryFileContents(std::string const &filename)
    {
        // Open file
        std::ifstream file(filename, std::ios::binary);

        // In case of failure, return an empty buffer
        if(file.fail()) {
            #if defined(DISPLAY_LOGS)
            std::cout << "GetBinaryFileContents(\"" << filename << "\") failed" << std::endl;
            #endif
            return {};
        }

        // Get file size
        std::streampos begin, end;
        begin = file.tellg();
        file.seekg(0, std::ios::end);
        end = file.tellg();

        // Resize buffer to file size
        std::vector<char> result(static_cast<size_t>(end - begin));

        // Read data
        file.seekg(0, std::ios::beg);
        file.read(&result[0], end - begin);
        file.close();

        // Return filled buffer
        return result;
    }

    std::vector<char> GetBinaryFileContents(std::wstring const& filename)
    {
        // Open file
        std::ifstream file(filename, std::ios::binary);

        // In case of failure, return an empty buffer
        if(file.fail()) {
            #if defined(DISPLAY_LOGS)
            std::wcout << "GetBinaryFileContents(\"" << filename << "\") failed" << std::endl;
            #endif
            return {};
        }

        // Get file size
        std::streampos begin, end;
        begin = file.tellg();
        file.seekg(0, std::ios::end);
        end = file.tellg();

        // Resize buffer to file size
        std::vector<char> result(static_cast<size_t>(end - begin));

        // Read data
        file.seekg(0, std::ios::beg);
        file.read(&result[0], end - begin);
        file.close();

        // Return filled buffer
        return result;
    }

    bool WriteToFile(std::vector<char> const& data, std::string const &filename)
    {
        // Open file
        std::ofstream file(filename, std::ofstream::binary);

        // In case of failure, return false
        if(file.fail()) {
            #if defined(DISPLAY_LOGS)
            std::cout << "SaveToFile(\"" << filename << "\") failed" << std::endl;
            #endif
            return false;
        }

        // Writing data
        file.write(data.data(), data.size());

        // Close file
        file.close();

        // Success
        return true;
    }

    IMAGE_MAP LoadImageFile(std::string filename)
    {
        IMAGE_MAP image = {};
        stbi_uc* stbi_data = stbi_load(filename.c_str(), reinterpret_cast<int*>(&image.width), reinterpret_cast<int*>(&image.height), reinterpret_cast<int*>(&image.format), STBI_rgb_alpha);
        if(stbi_data != nullptr) {
            image.data.resize(image.width * image.height * STBI_rgb_alpha);
            std::memcpy(image.data.data(), stbi_data, image.width * image.height * STBI_rgb_alpha);
            stbi_image_free(stbi_data);
        }
        return image;
    }

    IMAGE_MAP LoadImageData(std::vector<char> buffer)
    {
        IMAGE_MAP image = {};
        stbi_uc* stbi_data = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(buffer.data()), static_cast<int>(buffer.size()), reinterpret_cast<int*>(&image.width), reinterpret_cast<int*>(&image.height), reinterpret_cast<int*>(&image.format), STBI_rgb_alpha);
        if(stbi_data != nullptr) {
            image.data.resize(image.width * image.height * STBI_rgb_alpha);
            std::memcpy(image.data.data(), stbi_data, image.width * image.height * STBI_rgb_alpha);
            stbi_image_free(stbi_data);
        }
        return image;
    }

    IMAGE_MAP LoadImageData(const char* buffer, size_t size)
    {
        IMAGE_MAP image = {};
        stbi_uc* stbi_data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(buffer), static_cast<int>(size), reinterpret_cast<int*>(&image.width), reinterpret_cast<int*>(&image.height), reinterpret_cast<int*>(&image.format), STBI_rgb_alpha);
        if(stbi_data != nullptr) {
            image.data.resize(image.width * image.height * STBI_rgb_alpha);
            std::memcpy(image.data.data(), stbi_data, image.width * image.height * STBI_rgb_alpha);
            stbi_image_free(stbi_data);
        }
        return image;
    }

    std::string GetFileDirectory(std::string const& path)
    {
        if(path.empty()) return "/";
        if(path[path.size() - 1] == '\\' || path[path.size() - 1] == '/') return path;

        std::size_t pos = path.find_last_of('/');
        if(pos == std::string::npos) pos = path.find_last_of('\\');
        if(pos != std::string::npos) return path.substr(0, pos + 1);

        return "/";
    }

    std::string FinishBySlash(std::string const& path)
    {
        if(path.empty()) return "/";
        if(path[path.size() - 1] == '\\' || path[path.size() - 1] == '/') return path;
        return path + '/';
    }

    std::string GetFileName(std::string const& path)
    {
        if(path.empty()) return {};
        if(path[path.size() - 1] == '\\' || path[path.size() - 1] == '/') return {};

        std::size_t pos = path.find_last_of('/');
        if(pos == std::string::npos) pos = path.find_last_of('\\');
        if(pos != std::string::npos) return path.substr(pos + 1);
        else return path;
    }

    std::wstring GetFileName(std::wstring const& path)
    {
        if(path.empty()) return {};
        if(path[path.size() - 1] == '\\' || path[path.size() - 1] == '/') return {};

        std::size_t pos = path.find_last_of('/');
        if(pos == std::wstring::npos) pos = path.find_last_of('\\');
        if(pos != std::wstring::npos) return path.substr(pos + 1);
        else return path;
    }

    std::string utf8_encode(const std::wstring& wstr)
    {
        std::string strTo;
        if(wstr.empty()) return std::string();

        #if defined _WIN32
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        strTo.resize(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
        #endif

        return strTo;
    }

    std::wstring utf8_decode(const std::string& str)
    {
        std::wstring wstrTo;
        if(str.empty()) return std::wstring();

        #if defined _WIN32
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
        wstrTo.resize(size_needed);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        #endif

        return wstrTo;
    }
}