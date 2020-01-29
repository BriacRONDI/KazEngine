// Astuce permettant de démarrer une application sans console directement sur la fonction main()
#if defined(_WIN32)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")       // Application fenêtrée
#pragma comment(linker, "/ENTRY:mainCRTStartup")    // Point d'entrée positionné sur l'initialisation de la Runtime Windows
#endif

// Librairie Standard
#include <thread>
#include <chrono>

// Moteur
#include "Engine/Engine.h"

#if defined(_DEBUG)
#include "Engine/Platform/Win32/Console/Console.h"
#endif


int main(int argc, char** argv)
{
    // Création de la fenêtre principale
    Engine::Window* main_window = new Engine::Window({1024, 768}, "KazEngine", Engine::Window::FULLSCREEN_MODE_DISABLED);
    
    #if defined(_DEBUG)
    Engine::Console::Open();
    #endif

    // Initialisation de Vulkan
    Engine::Surface draw_surface =  main_window->GetSurface();
    Engine::Vulkan* engine = Engine::Vulkan::GetInstance();
    Engine::Vulkan::RETURN_CODE init = engine->Initialize(main_window, VK_MAKE_VERSION(0, 0, 1), "KazEngine");

    // Erreur à l'initialisation de vulkan
    if(init != Engine::Vulkan::RETURN_CODE::SUCCESS) {

        // On ferme la fenêtre
        delete main_window;

        #if defined(_DEBUG)
        system("PAUSE");
        Engine::Console::Close();
        #endif

        // On renvoie le code d'erreur
        return static_cast<int>(init);
    }

    Engine::Vulkan::MESH grid;
    Engine::Tools::IMAGE_MAP grid_image = Engine::Tools::LoadImageFile("./Assets/grid.png");
    grid.texture_id = Engine::Vulkan::GetInstance()->CreateTexture(grid_image);
    
    std::vector<Engine::Vulkan::VERTEX> grid_vertex_buffer(8);
    grid_vertex_buffer[0].texture_coordinates = {0, 0};
    grid_vertex_buffer[0].vertex = {-250.0f, 0.0f, 250.0f};
    grid_vertex_buffer[0].normal = {0.0f, 1.0f, 0.0f};

    grid_vertex_buffer[1].texture_coordinates = {1, 0};
    grid_vertex_buffer[1].vertex = {250.0f, 0.0f, 250.0f};
    grid_vertex_buffer[1].normal = {0.0f, 1.0f, 0.0f};

    grid_vertex_buffer[2].texture_coordinates = {1, 1};
    grid_vertex_buffer[2].vertex = {250.0f, 0.0f, -250.0f};
    grid_vertex_buffer[2].normal = {0.0f, 1.0f, 0.0f};

    grid_vertex_buffer[3].texture_coordinates = {0, 1};
    grid_vertex_buffer[3].vertex = {-250.0f, 0.0f, -250.0f};
    grid_vertex_buffer[3].normal = {0.0f, 1.0f, 0.0f};

    grid_vertex_buffer[4].texture_coordinates = {0, 0};
    grid_vertex_buffer[4].vertex = {-250.0f, 0.0f, 250.0f};
    grid_vertex_buffer[4].normal = {0.0f, -1.0f, 0.0f};

    grid_vertex_buffer[5].texture_coordinates = {1, 0};
    grid_vertex_buffer[5].vertex = {250.0f, 0.0f, 250.0f};
    grid_vertex_buffer[5].normal = {0.0f, -1.0f, 0.0f};

    grid_vertex_buffer[6].texture_coordinates = {1, 1};
    grid_vertex_buffer[6].vertex = {250.0f, 0.0f, -250.0f};
    grid_vertex_buffer[6].normal = {0.0f, -1.0f, 0.0f};

    grid_vertex_buffer[7].texture_coordinates = {0, 1};
    grid_vertex_buffer[7].vertex = {-250.0f, 0.0f, -250.0f};
    grid_vertex_buffer[7].normal = {0.0f, -1.0f, 0.0f};

    grid.vertex_buffer_id = Engine::Vulkan::GetInstance()->CreateVertexBuffer(grid_vertex_buffer);

    std::vector<uint32_t> grid_index_buffer = {0, 1, 3, 1, 2, 3, 7, 6, 5, 7, 5, 4};
    grid.index_buffer_id = Engine::Vulkan::GetInstance()->CreateIndexBuffer(grid_index_buffer);

    uint32_t grid_id = Engine::Vulkan::GetInstance()->CreateMesh(grid);
    
    // std::string fbx_file = "./Assets/Models/grom-hellscream/source/grom_rig.fbx";
    // std::string fbx_file = "./Assets/Models/wolf/Wolf_One_fbx7.4_binary.fbx";
    std::string fbx_file = "./Assets/Models/chevalier/chevalier.fbx";
    // std::string fbx_file = "./Assets/Models/cerberus/cerberus.fbx";
    // std::string fbx_file = "./Assets/Models/cube.fbx";

    Engine::ModelLoader loader;

    std::vector<Engine::Vulkan::MESH> meshes = loader.LoadFile(fbx_file);
    if(!meshes.size()) {
        engine->DestroyInstance();
        delete main_window;

        #if defined(_DEBUG)
        std::cout << "Failed to load : " << fbx_file << std::endl;
        system("PAUSE");
        Engine::Console::Close();
        #endif

        return 1;
    }

    // std::string play_animation = "Wolf_Skeleton|Wolf_creep_cycle";
    std::string play_animation = "metarig|stand";
    auto animation_start = std::chrono::system_clock::now();
    auto animation_total_duration = std::chrono::milliseconds(2000);

    // Boucle principale
    while(Engine::Window::Loop())
    {
        // Affichage
        engine->Draw();

        // Attente de 10 ms
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Avancement de l'animation
        /*auto now = std::chrono::system_clock::now();
        auto animation_current_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - animation_start);
        if(animation_current_duration > animation_total_duration) animation_start = now;
        for(auto& mesh : meshes) engine->UpdateSkeleton(mesh.vertex_buffer_id, animation_current_duration, play_animation);*/
    }

    // Libération des resources
    // TODO : Free VertexBuffer, Mesh, Texture
    engine->DestroyInstance();
    delete main_window;

    #if defined(_DEBUG)
    system("PAUSE");
    Engine::Console::Close();
    #endif

    return 0;
}
