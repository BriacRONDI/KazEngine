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

    // Create de main window
    Engine::Surface draw_surface =  main_window->GetSurface();

    // Initialize vulkan
    Engine::Vulkan& vulkan = Engine::Vulkan::CreateInstance();
    vulkan.Initialize(main_window, VK_MAKE_VERSION(0, 0, 1), "KazEngine", false);

    Engine::Core engine;
    engine.Initialize();

    ////////////////////////
    // MONO TEXTURED_CUBE //
    ////////////////////////
    
    auto data_buffer = Tools::GetBinaryFileContents("data.kea");
    auto concrete_jpg = Engine::DataBank::GetImageFromPackage(data_buffer, "/mono_textured_cube/concrete.jpg");
    Engine::DataBank::AddTexture(concrete_jpg, "concrete.jpg");
    auto cube_mesh = Engine::DataBank::GetMeshFromPackage(data_buffer, "/mono_textured_cube/MT_Cube");
    cube_mesh->UpdateRenderMask({});
    cube_mesh->render_mask |= Model::Mesh::RENDER_TEXTURE;
    auto cube_material = Engine::DataBank::GetMaterialFromPackage(data_buffer, "/mono_textured_cube/Ciment");
    Engine::DataBank::AddMaterial(cube_material, "Ciment");

    Engine::Entity cube1;
    cube1.properties.matrix = Maths::Matrix4x4::TranslationMatrix({-2.0f, -1.0f, 0.0f});
    cube1.AddMesh(cube_mesh);
    engine.GetEntityRender().AddEntity(cube1);

    Engine::Entity cube2;
    cube2.properties.matrix = Maths::Matrix4x4::TranslationMatrix({2.0f, -1.0f, 0.0f});
    cube2.AddMesh(cube_mesh);
    engine.GetEntityRender().AddEntity(cube2);

    ////////////////
    // SIMPLE GUY //
    ////////////////

    auto simple_guy_png = Engine::DataBank::GetImageFromPackage(data_buffer, "/SimpleGuy/SimpleGuy.2.0.png");
    Engine::DataBank::AddTexture(simple_guy_png, "SimpleGuy.2.0.png");
    auto simple_guy_mesh = Engine::DataBank::GetMeshFromPackage(data_buffer, "/SimpleGuy/Body");
    simple_guy_mesh->UpdateRenderMask({});
    simple_guy_mesh->render_mask |= Model::Mesh::RENDER_TEXTURE;
    auto simple_guy_skeleton = Engine::DataBank::GetSkeletonFromPackage(data_buffer, "/SimpleGuy/Armature");
    Engine::DataBank::AddSkeleton(simple_guy_skeleton, "Armature");
    auto simple_guy_material = Engine::DataBank::GetMaterialFromPackage(data_buffer, "/SimpleGuy/AO");
    Engine::DataBank::AddMaterial(simple_guy_material, "AO");

    Engine::Entity simple_guy;
    simple_guy.properties.matrix = Maths::Matrix4x4::TranslationMatrix({});
    simple_guy.AddMesh(simple_guy_mesh);
    engine.GetEntityRender().AddEntity(simple_guy);
    // simple_guy.PlayAnimation("Armature|Walk", 1.5f, false);

    Engine::Camera::GetInstance().SetPosition({0.0f, 5.0f, -5.0f});
    Engine::Camera::GetInstance().Rotate({0.0f, Maths::F_PI_4, 0.0f});

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
                main_window->SetTitle("KazEngine [" + std::to_string(fps) + " FPS] (Cliquez sur la fenetre pour capturer la souris)");
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