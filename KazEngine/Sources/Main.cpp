// Astuce permettant de démarrer une application sans console directement sur la fonction main()
#if defined(_WIN32)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")       // Application fenêtrée
#pragma comment(linker, "/ENTRY:mainCRTStartup")    // Point d'entrée positionné sur l'initialisation de la Runtime Windows
#endif

#include <chrono>
#include <thread>

// Moteur
#include "Engine/Engine.h"

// Mode debug
#if defined(DISPLAY_LOGS)
#include "Engine/Platform/Win32/Console/Console.h"
#endif

// Debug via RenderDoc
#define USE_RENDERDOC


int main(int argc, char** argv)
{
    // Création de la fenêtre principale
    Engine::Window* main_window = new Engine::Window({1024, 768}, "KazEngine", Engine::Window::FULLSCREEN_MODE_DISABLED);

    #if defined(DISPLAY_LOGS)
    Engine::Console::Open();
    #endif

    #if defined(USE_RENDERDOC)
    system("PAUSE");
    #endif

    // Création d'une fenêtre
    Engine::Surface draw_surface =  main_window->GetSurface();

    // Initialisation de vulkan
    Engine::Vulkan& vulkan = Engine::Vulkan::GetInstance();
    vulkan.Initialize(main_window, VK_MAKE_VERSION(0, 0, 1), "KazEngine", false);

    // Initilisation du moteur
    Engine::Core engine;
    engine.Initialize();

    /*engine.model_manager.LoadFile("./Assets/base_cube.kea");
    std::shared_ptr<Engine::DefaultColorEntity> light(new Engine::DefaultColorEntity);
    light->AttachMesh(engine.model_manager.models["SimplestCube"]);
    light->default_color = {1.0f, 1.0f, 1.0f};
    engine.AddEntity(light);*/

    /*engine.model_manager.LoadFile("./Assets/base_sphere.kea");
    std::shared_ptr<Engine::DefaultColorEntity> sphere(new Engine::DefaultColorEntity);
    sphere->AttachMesh(engine.model_manager.models["Sphere"]);
    sphere->default_color = {0.0f, 1.0f, 0.0f};
    engine.AddEntity(sphere);*/

    engine.model_manager.LoadFile("./Assets/chevalier.kea");
    std::shared_ptr<Engine::DefaultColorEntity> chevalier(new Engine::DefaultColorEntity);
    chevalier->AttachMesh(engine.model_manager.models["18489_Knight_V1_"]);
    chevalier->default_color = {76.0f/255, 14.0f/255, 138.0f/255};
    //engine.AddEntity(chevalier);

    engine.model_manager.LoadFile("./Assets/multi_textured_cube.kea");
    std::shared_ptr<Engine::Entity> multi_textured_cube(new Engine::Entity);
    multi_textured_cube->AttachMesh(engine.model_manager.models["Cube"]);
    engine.AddEntity(multi_textured_cube);

    engine.model_manager.LoadFile("./Assets/mono_textured_cube.kea");
    std::shared_ptr<Engine::Entity> mono_textured_cube(new Engine::Entity);
    mono_textured_cube->AttachMesh(engine.model_manager.models["MT_Cube"]);
    engine.AddEntity(mono_textured_cube);

    engine.model_manager.LoadFile("./Assets/multi_material_cube.kea");
    std::shared_ptr<Engine::Entity> multi_material_cube(new Engine::Entity);
    multi_material_cube->AttachMesh(engine.model_manager.models["MM_Cube"]);
    engine.AddEntity(multi_material_cube);

    engine.model_manager.LoadFile("./Assets/hellscream.kea");
    std::shared_ptr<Engine::SkeletonEntity> hellscream(new Engine::SkeletonEntity);
    hellscream->AttachMesh(engine.model_manager.models["shackle"]);
    hellscream->AttachMesh(engine.model_manager.models["eyes"]);
    hellscream->AttachMesh(engine.model_manager.models["earring"]);
    hellscream->AttachMesh(engine.model_manager.models["cloth"]);
    hellscream->AttachMesh(engine.model_manager.models["chain_base.003"]);
    hellscream->AttachMesh(engine.model_manager.models["chain_base.002"]);
    hellscream->AttachMesh(engine.model_manager.models["chain_base"]);
    hellscream->AttachMesh(engine.model_manager.models["chain.002"]);
    hellscream->AttachMesh(engine.model_manager.models["chain.001"]);
    hellscream->AttachMesh(engine.model_manager.models["chain"]);
    hellscream->AttachMesh(engine.model_manager.models["banner"]);
    hellscream->AttachMesh(engine.model_manager.models["amulet"]);
    hellscream->AttachMesh(engine.model_manager.models["axe"]);
    hellscream->AttachMesh(engine.model_manager.models["rope"]);
    hellscream->AttachMesh(engine.model_manager.models["body"]);

    engine.AddEntity(hellscream);

    engine.camera.SetPosition({0.0f, 3.5f, -7.5f});
    engine.camera.Rotate({0.0f, 30.0f, 0.0f});

    auto animation_start = std::chrono::system_clock::now();
    auto framerate_start = animation_start;
    auto animation_total_duration = std::chrono::milliseconds(10000);
    uint64_t frame_count = 0;

    // Boucle principale
    while(Engine::Window::Loop())
    {
        auto now = std::chrono::system_clock::now();

        auto animation_current_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - animation_start);
        float ratio = static_cast<float>(animation_current_duration.count()) / static_cast<float>(animation_total_duration.count());
        if(animation_current_duration > animation_total_duration) animation_start = now;

        auto framerate_current_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - framerate_start);
        if(framerate_current_duration > std::chrono::milliseconds(100)) {
            framerate_start = now;
            main_window->SetTitle("KazEngine [" + std::to_string(frame_count * 10) + " FPS]");
            frame_count = 0;
        }else{
            frame_count++;
        }

        float angle = 360.0f * ratio;

        Engine::Matrix4x4 translation = Engine::Matrix4x4::TranslationMatrix({-2.0f, 0.0f, 0.0f});
        Engine::Matrix4x4 rotation = Engine::Matrix4x4::RotationMatrix(angle, {0.0f, 1.0f, 0.0f});
        multi_textured_cube->matrix = translation * rotation;

        translation = Engine::Matrix4x4::TranslationMatrix({-2.0f, 0.0f, -3.5f});
        rotation = Engine::Matrix4x4::RotationMatrix(angle, {0.0f, -1.0f, 0.0f});
        mono_textured_cube->matrix = translation * rotation;

        
        // light->matrix = Engine::Matrix4x4::TranslationMatrix({-1.0f, -2.0f, 0.0f}) * Engine::Matrix4x4::ScalingMatrix({0.1f, 0.1f, 0.1f});
        translation = Engine::Matrix4x4::TranslationMatrix({1.5f, 1.0f, 0.0f});
        translation = Engine::Matrix4x4::TranslationMatrix({0.0f, 0.0f, 0.0f});
        rotation = Engine::Matrix4x4::RotationMatrix(-90.0f, {1.0f, 0.0f, 0.0f});
        rotation = rotation * Engine::Matrix4x4::RotationMatrix(angle + 180.0f, {0.0f, 0.0f, 1.0f});
        Engine::Matrix4x4 scale = Engine::Matrix4x4::ScalingMatrix({0.18f, 0.18f, 0.18f});
        chevalier->matrix = translation * rotation * scale;

        // light->matrix = Engine::Matrix4x4::TranslationMatrix({1.0f, 3.0f, 0.0f}) * Engine::Matrix4x4::ScalingMatrix({0.1f, 0.1f, 0.1f});
        // sphere->matrix = Engine::Matrix4x4::TranslationMatrix({1.0f, 2.0f, 1.0f});
        
        // light->matrix = Engine::Matrix4x4::TranslationMatrix({0.0f, -3.5f, 15.0f}) * Engine::Matrix4x4::ScalingMatrix({0.1f, 0.1f, 0.1f});
        // rotation = Engine::Matrix4x4::RotationMatrix(angle + 180.0f, {0.0f, 0.0f, 1.0f});
        // chevalier->matrix = rotation;

        translation = Engine::Matrix4x4::TranslationMatrix({2.0f, 0.0f, -3.5f});
        rotation = Engine::Matrix4x4::RotationMatrix(angle, {0.0f, 1.0f, 0.0f});
        multi_material_cube->matrix = translation * rotation;
        
        translation = Engine::Matrix4x4::TranslationMatrix({0.0f, 1.0f, 2.5f});
        rotation = Engine::Matrix4x4::RotationMatrix(180.0, {0.0f, 0.0f, 1.0f});
        scale = Engine::Matrix4x4::ScalingMatrix({0.01f, 0.01f, 0.01f});
        hellscream->matrix = translation * rotation * scale;

        engine.DrawScene();
    }

    // Libération des resources
    engine.Clear();
    vulkan.DestroyInstance();
    delete main_window;

    mono_textured_cube->Clear();
    multi_textured_cube->Clear();
    chevalier->Clear();
    multi_material_cube->Clear();
    //hellscream->Clear();

    #if defined(DISPLAY_LOGS)
    system("PAUSE");
    Engine::Console::Close();
    #endif

    return 0;
}
