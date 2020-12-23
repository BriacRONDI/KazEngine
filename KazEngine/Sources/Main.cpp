#if defined(_WIN32)
#pragma comment(linker, "/ENTRY:mainCRTStartup")    // Entry point initializes CRT
#endif

#include "Engine.h"

int main(int argc, char** argv)
{
    #if defined(DISPLAY_LOGS)
    Log::Terminal::Open();
    #endif

    Engine::Window* main_window = new Engine::Window({1024, 768}, "KazEngine", Engine::Window::FULLSCREEN_MODE_DISABLED);

    auto& vulkan = Engine::Vulkan::CreateInstance();
    if(!vulkan->Initialize(main_window, VK_MAKE_VERSION(0, 0, 1), "KazEngine")) {
        #if defined(DISPLAY_LOGS)
        std::cout << "Vulkan initialization : Failure" << std::endl;
        #endif
        return 0;
    }

    #if defined(DISPLAY_LOGS)
    std::cout << "Vulkan initialization : Success" << std::endl;
    #endif

    auto& engine = Engine::Core::CreateInstance();
    if(!engine->Initialize()) {
        #if defined(DISPLAY_LOGS)
        std::cout << "Engine initialization : Failure" << std::endl;
        #endif
        return 0;
    }

    #if defined(DISPLAY_LOGS)
    std::cout << "Engine initialization : Success" << std::endl;
    #endif

    auto data_buffer = Tools::GetBinaryFileContents("data.kea");
    auto guy_texture = Model::Loader::GetImageFromPackage(data_buffer, "/SimpleGuy/SimpleGuy.2.0.png");
    auto paladin_texture = Model::Loader::GetImageFromPackage(data_buffer, "/Paladin/Paladin.png");
    auto cube_texture = Model::Loader::GetImageFromPackage(data_buffer, "/TurningCube/concrete.jpg");
    engine->LoadTexture(paladin_texture, "Paladin.png");
    engine->LoadTexture(cube_texture, "concrete.jpg");
    engine->LoadTexture(guy_texture, "SimpleGuy.2.0.png");
    auto grass_texture = Model::Loader::GetImageFromPackage(data_buffer, "/grass_tile2");
    // engine->LoadTexture(guy_texture, "SimpleGuy.2.0.png");
    engine->LoadTexture(grass_texture, "grass_tile2");

    /*auto guy_skeleton = Model::Loader::GetSkeletonFromPackage(data_buffer, "/SimpleGuy/Armature");
    engine->LoadSkeleton(guy_skeleton, "Armature|Idle", "Armature|Walk");
    auto mesh_lod0 = Model::Loader::GetMeshFromPackage(data_buffer, "/SimpleGuy/Body_LOD0");*/

    /*auto mesh_lod1 = Model::Loader::GetMeshFromPackage(data_buffer, "/SimpleGuy/Body_LOD1");
    auto mesh_lod2 = Model::Loader::GetMeshFromPackage(data_buffer, "/SimpleGuy/Body_LOD2");
    auto mesh_lod3 = Model::Loader::GetMeshFromPackage(data_buffer, "/SimpleGuy/Body_LOD3");*/

    // "Paladin_Attack", "Paladin_Damage", "Paladin_Death", "Paladin_Idle", "Paladin_Idle_long", "Paladin_Tpose", "Paladin_Walk"
    auto paladin_skeleton = Model::Loader::GetSkeletonFromPackage(data_buffer, "/Paladin/bones");
    engine->LoadSkeleton(paladin_skeleton, "Paladin_Idle", "Paladin_Walk", "Paladin_Attack");
    auto mesh_lod0 = Model::Loader::GetMeshFromPackage(data_buffer, "/Paladin/Paladin");

    /*auto cube_skeleton = Model::Loader::GetSkeletonFromPackage(data_buffer, "/TurningCube/Armature");
    engine->LoadSkeleton(cube_skeleton, "Armature|Idle");
    auto mesh_lod0 = Model::Loader::GetMeshFromPackage(data_buffer, "/TurningCube/Cube");*/

    Engine::LODGroup simple_guy_lod;
    simple_guy_lod.AddLOD(mesh_lod0, 0);
    /*simple_guy_lod.AddLOD(mesh_lod1, 1);
    simple_guy_lod.AddLOD(mesh_lod2, 2);
    simple_guy_lod.AddLOD(mesh_lod3, 3);*/
    simple_guy_lod.SetHitBox({{-0.25f, 0.0f, 0.25f},{0.25f, -1.3f, -0.25f}});
    engine->LoadModel(simple_guy_lod);

    std::vector<std::shared_ptr<Engine::DynamicEntity>> entities;

    /*Engine::DynamicEntity entityA;
    entityA.AddModel(&simple_guy_lod);
    entityA.Matrix() = Maths::Matrix4x4::TranslationMatrix({-1.0f, 0.0f, 0.0f});
    engine->AddToScene(entityA);
    entityA.PlayAnimation("Armature|Walk", 1.0f, true);*/

    /*Engine::DynamicEntity entityB;
    entityB.AddModel(&simple_guy_lod);
    entityB.Matrix() = Maths::Matrix4x4::TranslationMatrix({1.0f, 0.0f, 0.0f});
    engine->AddToScene(entityB);*/

    Engine::Camera::GetInstance()->SetPosition({0.0f, 5.0f, -4.0f});
    Engine::Camera::GetInstance()->Rotate({0.0f, Maths::F_PI_4, 0.0f});
    // Engine::Camera::GetInstance()->SetRtsMode();
    Engine::Camera::GetInstance()->SetFpsMode();

    Engine::Timer framerate_timer, refresh_timer;
    framerate_timer.Start(std::chrono::milliseconds(1000));
    refresh_timer.Start(std::chrono::milliseconds(50));
    uint32_t frame_count = 0;

    Engine::Timer dynamic_entity_add_start;
    dynamic_entity_add_start.Start(std::chrono::milliseconds(10));

    while(Engine::Window::Loop())
    {

        ///////////////
        //   DEBUG   //
        ///////////////

        if(dynamic_entity_add_start.GetProgression() >= 1.0f && Engine::Keyboard::GetInstance().IsPressed(VK_ADD)) {
            uint32_t* debug = reinterpret_cast<uint32_t*>(Engine::GlobalData::GetInstance()->debug_descriptor.AccessData(0));
            (*debug)++;
            dynamic_entity_add_start.Start(std::chrono::milliseconds(100));
        }

        if(dynamic_entity_add_start.GetProgression() >= 1.0f && Engine::Keyboard::GetInstance().IsPressed(VK_SUBTRACT)) {
            uint32_t* debug = reinterpret_cast<uint32_t*>(Engine::GlobalData::GetInstance()->debug_descriptor.AccessData(0));
            (*debug)--;
            dynamic_entity_add_start.Start(std::chrono::milliseconds(100));
        }
        
        static int32_t move_me = 21;
        if(dynamic_entity_add_start.GetProgression() >= 1.0f && Engine::Keyboard::GetInstance().IsPressed(VK_NUMPAD0)) {
            int32_t* debug_move_item = reinterpret_cast<int32_t*>(Engine::GlobalData::GetInstance()->debug_descriptor.AccessData(sizeof(uint32_t)));
            if(*debug_move_item >= 0) *debug_move_item = -1;
            else *debug_move_item = move_me;
            dynamic_entity_add_start.Start(std::chrono::milliseconds(100));
        }

        if(dynamic_entity_add_start.GetProgression() >= 1.0f && Engine::Keyboard::GetInstance().IsPressed(VK_PRIOR)) {
            move_me++;
            dynamic_entity_add_start.Start(std::chrono::milliseconds(100));
        }

        if(dynamic_entity_add_start.GetProgression() >= 1.0f && Engine::Keyboard::GetInstance().IsPressed(VK_NEXT)) {
            move_me--;
            dynamic_entity_add_start.Start(std::chrono::milliseconds(100));
        }

        ///////////////
        // FRAMERATE //
        ///////////////
        
        static int count = 0;
        frame_count++;
        if(refresh_timer.GetProgression() >= 1.0f) {
            float fps = static_cast<float>((float)frame_count / framerate_timer.GetProgression());
            if(Engine::Mouse::GetInstance().IsClipped()) {
                main_window->SetTitle(std::string("KazEngine") +
                    " [" + Tools::to_string_with_precision(fps, 1) + " FPS]" +
                    " [Units : " + std::to_string(count) + "]" +
                    " [Debug : " + std::to_string(move_me) + "]" +
                    " (Appuyez sur ESC pour liberer la souris)");
            }else{
                main_window->SetTitle(std::string("KazEngine") +
                    " [" + Tools::to_string_with_precision(fps, 1) + " FPS]" +
                    " [Units : " + std::to_string(count) + "]" +
                    " [Debug : " + std::to_string(move_me) + "]" +
                    " (Cliquez sur la fenetre pour capturer la souris)");
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

        if(dynamic_entity_add_start.GetProgression() >= 1.0f && Engine::Keyboard::GetInstance().IsPressed(VK_SPACE)) {

            uint32_t step = 1;
            for(uint32_t i=0; i<step; i++) {

                std::shared_ptr<Engine::DynamicEntity> entity = std::shared_ptr<Engine::DynamicEntity>(new Engine::DynamicEntity);
                entity->AddModel(&simple_guy_lod);
                engine->AddToScene(*entity);

                entities.resize(entities.size() + 1);
                entities[entities.size()-1] = entity;

                // if(entities.size() % 2) entity->PlayAnimation("Armature|Walk", 1.0f, true);
                // else entity->PlayAnimation("Armature|Idle", 1.0f, true);
                // entity->PlayAnimation("Armature|Idle", 1.0f, true);

                // "Paladin_Attack", "Paladin_Damage", "Paladin_Death", "Paladin_Idle", "Paladin_Idle_long", "Paladin_Tpose", "Paladin_Walk"
                entity->PlayAnimation("Paladin_Walk", 1.0f, true);
                // entity->PlayAnimation("Armature|Walk", 1.0f, true);

                count++;
            }

            uint32_t count_z = static_cast<uint32_t>(std::sqrt(entities.size()));
            uint32_t count_x = count_z;

            uint32_t last = 0, rest = 0;
            if(entities.size() >= 2) {
                last = (count_z - 1) + (count_z - 1) * count_z;
                rest = static_cast<uint32_t>(entities.size() - last - 1);
            }

            for(uint32_t z=0; z<count_z; z++) {
                for(uint32_t x=0; x<count_x; x++) {
                    entities[x + z * count_z]->Matrix() = Maths::Matrix4x4::TranslationMatrix({x - count_x * 0.5f + 0.5f, 0.0f, -1.0f * z})
                                                        * Maths::Matrix4x4::RotationMatrix(180.0f, {0.0f, 1.0f, 0.0f})
                                                        * Maths::Matrix4x4::ScalingMatrix({0.05f, 0.05f, 0.05f});
                }
            }

            for(uint32_t x=0; x<rest; x++) {
                entities[x + last + 1]->Matrix() = Maths::Matrix4x4::TranslationMatrix({x - rest * 0.5f + 0.5f, 0.0f, -1.0f * count_z - 1.0f});
            }

            dynamic_entity_add_start.Start(std::chrono::milliseconds(100));
        }

        ///////////////
        // MAIN LOOP //
        ///////////////

        engine->Loop();
    }

    engine->DestroyInstance();
    vulkan->DestroyInstance();
    delete main_window;

    #if defined(DISPLAY_LOGS)
    Log::Terminal::Pause();
    Log::Terminal::Close();
    #endif

    return 0;
}