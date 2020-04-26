#if defined(_WIN32)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")       // Window application
#pragma comment(linker, "/ENTRY:mainCRTStartup")    // Entry point initializes CRT
#endif

#if defined(DISPLAY_LOGS)
#include <LogManager.h>
#endif

#include "Engine.h"

int main(int argc, char** argv)
{
    Engine::Window* main_window = new Engine::Window({1024, 768}, "KazEngine", Engine::Window::FULLSCREEN_MODE_DISABLED);

    #if defined(DISPLAY_LOGS)
    Log::Terminal::Open();
    #endif

    #if defined(USE_RENDERDOC)
    system("PAUSE");
    #endif

    // Create de main window
    Engine::Surface draw_surface =  main_window->GetSurface();

    // Initialize vulkan
    Engine::Vulkan& vulkan = Engine::Vulkan::CreateInstance();
    vulkan.Initialize(main_window, VK_MAKE_VERSION(0, 0, 1), "KazEngine", false);

    Engine::Core engine;
    engine.Initialize();

    auto framerate_start = std::chrono::system_clock::now();
    uint64_t frame_count = 0;

    // Main loop
    while(Engine::Window::Loop())
    {
        static uint64_t fps = 0;
        auto now = std::chrono::system_clock::now();
        auto framerate_current_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - framerate_start);
        if(framerate_current_duration > std::chrono::milliseconds(1000)) {
            framerate_start = now;
            fps = frame_count;
            frame_count = 0;
            if(Engine::Mouse::GetInstance().IsClipped()) {
                main_window->SetTitle("KazEngine [" + std::to_string(fps) + " FPS] (Appuyez sur ESC pour liberer la souris)");
            }else{
                main_window->SetTitle("KazEngine [" + std::to_string(fps) + " FPS] (Cliquez sur le fenetre pour capturer la souris)");
            }
        }else{
            frame_count++;
        }

        engine.Loop();
    }

    engine.Clear();
    vulkan.DestroyInstance();
    delete main_window;

    #if defined(DISPLAY_LOGS)
    system("PAUSE");
    Log::Terminal::Close();
    #endif

    return EXIT_SUCCESS;
}