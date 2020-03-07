#if defined (_WIN32)
#include <Windows.h>
#endif

#include <iostream>

#include <Engine/Common/Tools/Tools.h>
#include "./FBX/FbxParser.h"

int main(int argc, char* argv[])
{
    #if defined(_WIN32)
    SetConsoleOutputCP(28591); // Console mode ISO 8859-1 Latin 1; Western European
    #endif

    if(argc != 3) {
        std::cout << "Usage : <source> <destination>" << std::endl;
    }else{
        std::string input_filename = std::string(argv[1]);
        std::string output_filename = std::string(argv[2]);

        // input_filename = "Assets/Models/base_cube.fbx";
        // output_filename = "../KazEngine/Assets/base_cube.kea";
        // input_filename = "Assets/Models/base_sphere.fbx";
        // output_filename = "../KazEngine/Assets/base_sphere.kea";
        // input_filename = "Assets/Models/Bulb/Bulb.FBX";
        // output_filename = "../KazEngine/Assets/ampoule.kea";
        // input_filename = "Assets/Models/textured_cube/mono_textured_cube.fbx";
        // output_filename = "../KazEngine/Assets/mono_textured_cube.kea";
        // input_filename = "Assets/Models/textured_cube/multi_textured_cube.fbx";
        // output_filename = "../KazEngine/Assets/multi_textured_cube.kea";
        // input_filename = "Assets/Models/chevalier/chevalier.fbx";
        // output_filename = "../KazEngine/Assets/chevalier.kea";
        // input_filename = "Assets/Models/textured_cube/multi_material_cube.fbx";
        // output_filename = "../KazEngine/Assets/multi_material_cube.kea";
        // input_filename = "Assets/Models/grom-hellscream/grom_rig.fbx";
        // output_filename = "../KazEngine/Assets/hellscream.kea";
        // input_filename = "Assets/Models/SimpleGuy.2.0/SimpleGuy.2.0.fbx";
        // output_filename = "../KazEngine/Assets/SimpleGuy.kea";
        input_filename = "Assets/Models/fleche/fleche.fbx";
        output_filename = "../KazEngine/Assets/fleche.kea";

        std::cout << "Opening file : " << input_filename << std::endl;
        std::vector<char> data = Engine::Tools::GetBinaryFileContents(input_filename);
        FbxParser parser(data);
        std::vector<char> package = parser.ParseData(Engine::Tools::GetFileDirectory(input_filename));
        Engine::Tools::WriteToFile(package, output_filename);
    }

    std::cout << std::endl;
    system("pause");
    return 0;
}