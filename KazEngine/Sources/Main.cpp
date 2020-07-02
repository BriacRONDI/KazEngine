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

    ///////////
    // DEBUG //
    ///////////

    /*std::shared_ptr<Model::Mesh> debug_mesh1(new Model::Mesh);
    debug_mesh1->vertex_buffer = {{0.0, 0.0f, 0.0f},{0.0f, 0.0f, -100.0f}};
    debug_mesh1->UpdateRenderMask({});

    std::shared_ptr<Model::Mesh> debug_mesh2(new Model::Mesh);
    debug_mesh2->vertex_buffer = {{0.0, 0.0f, 0.0f},{0.0f, 0.0f, -100.0f}};
    debug_mesh2->UpdateRenderMask({});

    std::shared_ptr<Model::Mesh> debug_mesh3(new Model::Mesh);
    debug_mesh3->vertex_buffer = {{0.0, 0.0f, 0.0f},{0.0f, 0.0f, -100.0f}};
    debug_mesh3->UpdateRenderMask({});

    Engine::Entity debug_line1, debug_line2, debug_line3;

    debug_line1.AddMesh(debug_mesh1);
    debug_line2.AddMesh(debug_mesh2);
    debug_line3.AddMesh(debug_mesh3);

    engine.GetEntityRender().AddEntity(debug_line1);
    engine.GetEntityRender().AddEntity(debug_line2);
    engine.GetEntityRender().AddEntity(debug_line3);

    debug_line1.properties.matrix = Maths::Matrix4x4::TranslationMatrix({0.0f, -3.0f, 0.0f});
    debug_line2.properties.matrix = Maths::Matrix4x4::TranslationMatrix({0.0f, -3.0f, 0.0f}) * Maths::Matrix4x4::RotationMatrix({1.0f, 0.0f, 0.0f});
    debug_line3.properties.matrix = Maths::Matrix4x4::TranslationMatrix({0.0f, -3.0f, 0.0f}) * Maths::Matrix4x4::RotationMatrix({0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
    */

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

    Engine::StaticEntity cube1;
    cube1.SetMatrix(Maths::Matrix4x4::TranslationMatrix({-2.0f, -1.0f, 0.0f}));
    cube1.AddMesh(cube_mesh);
    // engine.GetEntityRender().AddEntity(cube1);

    Engine::StaticEntity cube2;
    cube2.SetMatrix(Maths::Matrix4x4::TranslationMatrix({2.0f, -1.0f, 0.0f}));
    cube2.AddMesh(cube_mesh);
    // engine.GetEntityRender().AddEntity(cube2);

    ////////////////
    // SIMPLE GUY //
    ////////////////

    auto simple_guy_png = Engine::DataBank::GetImageFromPackage(data_buffer, "/SimpleGuy/SimpleGuy.2.0.png");
    Engine::DataBank::AddTexture(simple_guy_png, "SimpleGuy.2.0.png");
    auto simple_guy_mesh = Engine::DataBank::GetMeshFromPackage(data_buffer, "/SimpleGuy/Body");
    simple_guy_mesh->UpdateRenderMask({});
    simple_guy_mesh->render_mask |= Model::Mesh::RENDER_TEXTURE;
    simple_guy_mesh->SetHitBox({{-0.25f, 0.0f, 0.25f},{0.25f, -1.3f, -0.25f}});
    auto simple_guy_skeleton = Engine::DataBank::GetSkeletonFromPackage(data_buffer, "/SimpleGuy/Armature");
    Engine::DataBank::AddSkeleton(simple_guy_skeleton, "Armature");
    auto simple_guy_material = Engine::DataBank::GetMaterialFromPackage(data_buffer, "/SimpleGuy/AO");
    Engine::DataBank::AddMaterial(simple_guy_material, "AO");

    Engine::DynamicEntity guy;
    guy.AddMesh(simple_guy_mesh);
    // engine.GetEntityRender().AddEntity(guy);
    guy.PlayAnimation("Armature|Walk", 1.0f, true);

    Engine::Camera::GetInstance().SetPosition({0.0f, 5.0f, -4.0f});
    Engine::Camera::GetInstance().Rotate({0.0f, Maths::F_PI_4, 0.0f});
    Engine::Camera::GetInstance().SetRtsMode();

    Engine::Timer framerate_timer, refresh_timer;
    framerate_timer.Start(std::chrono::milliseconds(1000));
    refresh_timer.Start(std::chrono::milliseconds(50));
    uint32_t frame_count = 0;

    uint32_t entity_count = 0;
    Engine::Timer dynamic_entity_add_start;
    dynamic_entity_add_start.Start(std::chrono::milliseconds(10));
    std::vector<std::shared_ptr<Engine::Entity>> guys;

    /*for(int i=0; i<100; i++) {
        auto new_entity = std::shared_ptr<Engine::DynamicEntity>(new Engine::DynamicEntity);
        new_entity->AddMesh(simple_guy_mesh);

        guys.resize(guys.size() + 1);
        guys[guys.size()-1] = new_entity;

        new_entity->PlayAnimation("Armature|Walk", 1.0f, true);
    }

    uint32_t count_z = static_cast<uint32_t>(std::sqrt(guys.size()));
    uint32_t count_x = count_z;

    uint32_t last = 0, rest = 0;
    if(guys.size() > 2) {
        last = (count_z - 1) + (count_z - 1) * count_z;
        rest = static_cast<uint32_t>(guys.size() - last - 1);
    }

    for(uint32_t z=0; z<count_z; z++) {
        for(uint32_t x=0; x<count_x; x++) {
            guys[x + z * count_z]->SetMatrix(Maths::Matrix4x4::TranslationMatrix({x - count_x * 0.5f + 0.5f, 0.0f, -1.0f * z - 5.0f}));
        }
    }

    for(uint32_t x=0; x<rest; x++) {
        guys[x + last + 1]->SetMatrix(Maths::Matrix4x4::TranslationMatrix({x - rest * 0.5f + 0.5f, 0.0f, -1.0f * count_z - 6.0f}));
    }*/

    // Main loop
    while(Engine::Window::Loop())
    {
        ///////////////
        // FRAMERATE //
        ///////////////
        frame_count++;
        if(refresh_timer.GetProgression() >= 1.0f) {
            float fps = static_cast<float>((float)frame_count / framerate_timer.GetProgression());
            if(Engine::Mouse::GetInstance().IsClipped()) {
                main_window->SetTitle("KazEngine [" + Tools::to_string_with_precision(fps, 1) + " FPS] [" + std::to_string(guys.size()) + " Units] (Appuyez sur ESC pour liberer la souris)");
            }else{
                main_window->SetTitle("KazEngine [" + Tools::to_string_with_precision(fps, 1) + " FPS] [" + std::to_string(guys.size()) + " Units] (Cliquez sur la fenetre pour capturer la souris)");
            }

            refresh_timer.Start(std::chrono::milliseconds(50));
        }

        if(framerate_timer.GetProgression() >= 1.0f) {
            framerate_timer.Start(std::chrono::milliseconds(1000));
            frame_count = 0;
        }

        //////////////
        // ENTITIES //
        //////////////

        static int count = 0;
        if(/*count < 1000 && */dynamic_entity_add_start.GetProgression() >= 1.0f && Engine::Keyboard::GetInstance().IsPressed(VK_SPACE)) {

            uint32_t step = 1;
            for(uint32_t i=0; i<step; i++) {
                auto new_entity = std::shared_ptr<Engine::DynamicEntity>(new Engine::DynamicEntity);
                new_entity->AddMesh(simple_guy_mesh);

                guys.resize(guys.size() + 1);
                guys[guys.size()-1] = new_entity;

                new_entity->PlayAnimation("Armature|Walk", 1.0f, true);

                count++;
            }

            uint32_t count_z = static_cast<uint32_t>(std::sqrt(guys.size()));
            uint32_t count_x = count_z;

            uint32_t last = 0, rest = 0;
            if(guys.size() >= 2) {
                last = (count_z - 1) + (count_z - 1) * count_z;
                rest = static_cast<uint32_t>(guys.size() - last - 1);
            }

            for(uint32_t z=0; z<count_z; z++) {
                for(uint32_t x=0; x<count_x; x++) {
                    guys[x + z * count_z]->SetMatrix(Maths::Matrix4x4::TranslationMatrix({x - count_x * 0.5f + 0.5f, 0.0f, -1.0f * z - 5.0f}));
                }
            }

            for(uint32_t x=0; x<rest; x++) {
                guys[x + last + 1]->SetMatrix(Maths::Matrix4x4::TranslationMatrix({x - rest * 0.5f + 0.5f, 0.0f, -1.0f * count_z - 6.0f}));
            }

            dynamic_entity_add_start.Start(std::chrono::milliseconds(100));
        }

        ///////////////
        // MAIN LOOP //
        ///////////////

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