#pragma once

#include <fstream>
#include <vector>

#if defined _WIN32
#include <Windows.h>
#endif

#if defined(DISPLAY_LOGS)
#include <iostream>
#endif

namespace Tools
{
    struct IMAGE_MAP {

        /// Image width
        uint32_t width;

        /// Image height
        uint32_t height;

        /**
         * Pixel format
         *  - STBI_grey = 1,
         *  - STBI_grey_alpha = 2,
         *  - STBI_rgb        = 3,
         *  - STBI_rgb_alpha  = 4
         */
        uint32_t format;

        /// Data buffer
        std::vector<unsigned char> data;
    };

    template <typename T>
    struct Area {
        T width;
        T height;
    };

    template <typename T>
    struct Point {
        T x;
        T y;
    };

    /**
     * Open and read a file
     * @param filename Full path to file
     * @return Buffer containing file data
     */
    std::vector<char> GetBinaryFileContents(std::string const& filename);

    /**
     * Open and read a file
     * @param filename Full path to file
     * @return Buffer containing file data
     */
    std::vector<char> GetBinaryFileContents(std::wstring const& filename);

    /**
     * Write some data to a file
     * @param data Data to write
     * @param filename Full path to file
     * @retval true Success
     * @retval false Failure
     */
    bool WriteToFile(std::vector<char> const& data, std::string const &filename);

    /**
     * Get the directory containing a file in a full path
     * @param path Full path to file
     * @return Directory including last /
     */
    std::string GetFileDirectory(std::string const& path);

    /**
     * Get the name of a file in a full path
     * @param path Full path to file
     * @return File name with extension
     */
    std::string GetFileName(std::string const& path);

    /**
     * Get the name of a file in a full path
     * @param path Full path to file
     * @return File name with extension
     */
    std::wstring GetFileName(std::wstring const& path);

    /**
     * Load image file using stb_image library
     * https://github.com/nothings/stb
     * @param filename File path
     * @return IMAGE_MAP object, with empty data in case of failure
     */
    IMAGE_MAP LoadImageFile(std::string filename);

    /**
     * Load image from memory using stb_image library
     * https://github.com/nothings/stb
     * @param buffer Buffer containing the whole file data
     * @return IMAGE_MAP object, with empty data in case of failure
     */
    IMAGE_MAP LoadImageData(std::vector<char> buffer);

    /**
     * Convert a Unicode String to an UTF8 string
     * @param wstr Unicode String
     * @return UTF8 String
     */
    std::string utf8_encode(const std::wstring& wstr);

    /**
     * Convert an UTF8 string to a wide Unicode String
     * @param str UTF8 string
     * @return Unicode String
     */
    std::wstring utf8_decode(const std::string& str);
}