#pragma once

#include <vector>
#include <string>

namespace DataPackerGUI
{
    /// Interface : Allow classes to parse data
    class Parser
    {
        public :
        
            /**
             * Parse specific data and convert to DataPacker format
             * @param data Raw input data
             * @param texture_directory Path to referenced textures
             * @param package_directory Path to dependancies inside package
             * @return Buffer of DataPacker structured data
             */
            virtual std::vector<char> ParseData(std::vector<char> const& data, std::string const& texture_directory = {}, std::string const& package_directory = "/") = 0;
    };
}