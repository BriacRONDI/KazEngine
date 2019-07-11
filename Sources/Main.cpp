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

    Engine::ModelLoader loader;
    if(!loader.LoadFromFile("./Assets/Models/grom-hellscream/source/grom_rig.fbx")) {
        engine->DestroyInstance();
        delete main_window;

        #if defined(_DEBUG)
        std::cout << "Failed to load : " << "./Assets/Models/grom-hellscream/source/grom_rig.fbx" << std::endl;
        system("PAUSE");
        Engine::Console::Close();
        #endif

        return 1;
    }

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
