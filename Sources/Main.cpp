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

// Lire une image depuis un fichier
#define STB_IMAGE_IMPLEMENTATION
#include "Engine/Tools/stb_image.h"

#if defined(_DEBUG)
#include "Engine/Platform/Win32/Console/Console.h"
#endif


int main(int argc, char** argv)
{
    // Création de la fenêtre principale
    Engine::Window* main_window = new Engine::Window({1024, 768}, "Cube", Engine::Window::FULLSCREEN_MODE_DISABLED);
    
    #if defined(_DEBUG)
    Engine::Console::Start();
    #endif

    // Initialisation de Vulkan
    Engine::Surface draw_surface =  main_window->GetSurface();
    Engine::Vulkan* engine = Engine::Vulkan::GetInstance();
    engine->Initialize(main_window, VK_MAKE_VERSION(0, 0, 1), "Cube");

    // Chargement d'une image depuis un fichier
    int width, height, format;
    stbi_uc* stbi_data = stbi_load("./Assets/texture.png", &width, &height, &format, STBI_rgb_alpha);
    if(stbi_data != nullptr) {
        std::vector<unsigned char> image_data(width * height * STBI_rgb_alpha);
        std::memcpy(image_data.data(), stbi_data, width * height * STBI_rgb_alpha);
        stbi_image_free(stbi_data);
        engine->CreateTexture(image_data, width, height);
    }

    #if defined(_DEBUG)
    if(stbi_data == nullptr) std::cout << "Impossible de charger la texture !" << std::endl;
    #endif

    #define VERT_COORD(x,y,z) {x,y,z,1.0f}
    #define TEXT_COORD(u,v) {u,v}

    std::vector<Engine::Vulkan::VERTEX> vertex_data = {
            {VERT_COORD(-1, -1, 1), TEXT_COORD(0, 0)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(1, -1, 1), TEXT_COORD(1, 0)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(1, 1, 1), TEXT_COORD(1, 1)},
            {VERT_COORD(1, -1, 1), TEXT_COORD(1, 0)},

            {VERT_COORD(-1, -1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(1, -1, 1), TEXT_COORD(1, 1)},
            {VERT_COORD(1, -1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(-1, -1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(1, -1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(-1, -1, -1), TEXT_COORD(0, 0)},

            {VERT_COORD(1, -1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(1, -1, 1), TEXT_COORD(0, 0)},
            {VERT_COORD(1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(1, 1, -1), TEXT_COORD(1, 1)},
            {VERT_COORD(1, -1, -1), TEXT_COORD(1, 0)},
              
            {VERT_COORD(1, -1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(-1, 1, -1), TEXT_COORD(0, 1)},
            {VERT_COORD(-1, -1, -1), TEXT_COORD(0, 0)},
            {VERT_COORD(1, -1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(1, 1, -1), TEXT_COORD(1, 1)},
            {VERT_COORD(-1, 1, -1), TEXT_COORD(0, 1)},
              
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(-1, -1, 1), TEXT_COORD(0, 0)},
            {VERT_COORD(-1, -1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(-1, -1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(-1, 1, -1), TEXT_COORD(1, 1)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)},
              
            {VERT_COORD(1, 1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(1, 1, 1), TEXT_COORD(1, 1)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(-1, 1, -1), TEXT_COORD(0, 0)},
            {VERT_COORD(1, 1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)}
    };

    //engine->UpdateVertexBuffer(vertex_data);
    uint32_t cube_model = engine->CreateVertexBuffer(vertex_data);
    uint32_t cube1 = engine->CreateMesh(cube_model);
    uint32_t cube2 = engine->CreateMesh(cube_model);
    engine->Start();

    // Boucle principale
    while(Engine::Window::Loop())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Libération des resources
    engine->DestroyInstance();
    delete main_window;

    #if defined(_DEBUG)
    system("pause");
    Engine::Console::Release();
    #endif

    return 0;
}
