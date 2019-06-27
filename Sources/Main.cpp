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
    Engine::Console::Open();
    #endif

    // Initialisation de Vulkan
    Engine::Surface draw_surface =  main_window->GetSurface();
    Engine::Vulkan* engine = Engine::Vulkan::GetInstance();
    Engine::Vulkan::RETURN_CODE init = engine->Initialize(main_window, VK_MAKE_VERSION(0, 0, 1), "Cube");

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

    // Chargement de la première texture
    int width, height, format;
    uint32_t texture1_id;
    stbi_uc* stbi_data = stbi_load("./Assets/texture.png", &width, &height, &format, STBI_rgb_alpha);
    if(stbi_data != nullptr) {
        std::vector<unsigned char> image_data(width * height * STBI_rgb_alpha);
        std::memcpy(image_data.data(), stbi_data, width * height * STBI_rgb_alpha);
        stbi_image_free(stbi_data);
        texture1_id = engine->CreateTexture(image_data, width, height);
    }

    #if defined(_DEBUG)
    if(stbi_data == nullptr) std::cout << "Impossible de charger la texture : texture.png !" << std::endl;
    #endif

    // Chargement de la seconde texture
    uint32_t texture2_id;
    stbi_data = stbi_load("./Assets/leaf.jpg", &width, &height, &format, STBI_rgb_alpha);
    if(stbi_data != nullptr) {
        std::vector<unsigned char> image_data(width * height * STBI_rgb_alpha);
        std::memcpy(image_data.data(), stbi_data, width * height * STBI_rgb_alpha);
        stbi_image_free(stbi_data);
        texture2_id = engine->CreateTexture(image_data, width, height);
    }

    #if defined(_DEBUG)
    if(stbi_data == nullptr) std::cout << "Impossible de charger la texture : leaf.jpg !" << std::endl;
    #endif

    #define VERT_COORD(x,y,z) {x,y,z,1.0f}
    #define TEXT_COORD(u,v) {u,v}

    std::vector<Engine::Vulkan::VERTEX> cube_data = {
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

    std::vector<Engine::Vulkan::VERTEX> prisme_data = {
            {VERT_COORD(0, -1, 0), TEXT_COORD(0.5f, 0)},
            {VERT_COORD(-1, 1, -1), TEXT_COORD(0, 1)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(1, 1)},

            {VERT_COORD(0, -1, 0), TEXT_COORD(0.5f, 0)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(1, 1, 1), TEXT_COORD(1, 1)},

            {VERT_COORD(0, -1, 0), TEXT_COORD(0.5f, 0)},
            {VERT_COORD(1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(1, 1, -1), TEXT_COORD(1, 1)},

            {VERT_COORD(0, -1, 0), TEXT_COORD(0.5f, 0)},
            {VERT_COORD(1, 1, -1), TEXT_COORD(0, 1)},
            {VERT_COORD(-1, 1, -1), TEXT_COORD(1, 1)},

            {VERT_COORD(1, 1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(1, 1, 1), TEXT_COORD(1, 1)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)},
            {VERT_COORD(-1, 1, -1), TEXT_COORD(0, 0)},
            {VERT_COORD(1, 1, -1), TEXT_COORD(1, 0)},
            {VERT_COORD(-1, 1, 1), TEXT_COORD(0, 1)}
    };

    uint32_t cube_model = engine->CreateVertexBuffer(cube_data);
    uint32_t cube = engine->CreateModel(cube_model, texture1_id);

    uint32_t prisme_model = engine->CreateVertexBuffer(prisme_data);
    uint32_t prisme = engine->CreateModel(prisme_model, texture2_id);

    // Boucle principale
    while(Engine::Window::Loop())
    {
        // Affichage
        engine->Draw();

        // Attente de 10 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
