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
#include "Engine/Platform/Inputs/Keyboard/IKeyboardListener.h"
class DebugMe : public Engine::IKeyboardListener
{
    public:
        DebugMe() { Engine::Keyboard::GetInstance()->AddListener(this); };
        ~DebugMe()  { Engine::Keyboard::GetInstance()->RemoveListener(this); };

        Engine::Matrix4x4* frustum_matrix;
        Engine::Matrix4x4* fleche_matrix;

        std::string display;
        float rot_h = 0.0f, rot_v = 0.0f;
        float rot_speed = 0.03f;
        float mov_speed = 0.005f;

        void Update()
        {

            if(Engine::Keyboard::GetInstance()->IsPressed(VK_LEFT))  this->rot_h -= this->rot_speed;
            if(Engine::Keyboard::GetInstance()->IsPressed(VK_RIGHT)) this->rot_h += this->rot_speed;
            if(Engine::Keyboard::GetInstance()->IsPressed(VK_ADD)) this->rot_v += this->rot_speed;
            if(Engine::Keyboard::GetInstance()->IsPressed(VK_SUBTRACT)) this->rot_v -= this->rot_speed;

            Engine::Matrix4x4 rotation = Engine::Matrix4x4::RotationMatrix(this->rot_h, {0.0f, 1.0f, 0.0f}) * Engine::Matrix4x4::RotationMatrix(this->rot_v, {1.0f, 0.0f, 0.0f});
            Engine::Matrix4x4 direction = rotation * Engine::Matrix4x4::TranslationMatrix({0.0f, 0.0f, 1.0f});
            Engine::Vector3 movement = {direction[12], direction[13], direction[14]};

            // *this->fleche_matrix = Engine::Matrix4x4::TranslationMatrix(cam_normal_left) * Engine::Matrix4x4::ScalingMatrix({0.25f, 0.25f, 0.25f});
        }

        virtual void KeyDown(unsigned char Key)
        {
            if(Key == VK_SPACE) {
                this->rot_h = 0.0f;
                this->rot_v = 0.0f;
            }
        }

        virtual void KeyUp(unsigned char Key)
        {
        }
};
DebugMe debug_me;
#endif

// Debug via RenderDoc
// #define USE_RENDERDOC


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
    Engine::Core& engine = Engine::Core::GetInstance();
    engine.Initialize();

    Engine::ModelManager::GetInstance().LoadFile("./Assets/SimpleGuy.kea");
    std::shared_ptr<Engine::SkeletonEntity> simple_guy(new Engine::SkeletonEntity);
    simple_guy->AttachMesh(Engine::ModelManager::GetInstance().models["Body"]);
    engine.AddEntity(simple_guy, false);
    simple_guy->SetAnimation("Armature|Walk");

    #if defined(DISPLAY_LOGS)
    Engine::ModelManager::GetInstance().LoadFile("./Assets/fleche.kea");
    std::shared_ptr<Engine::Entity> fleche(new Engine::Entity);
    fleche->AttachMesh(Engine::ModelManager::GetInstance().models["Fleche"]);
    debug_me.fleche_matrix = &fleche->matrix;
    // engine.AddEntity(fleche, false);

    Engine::ModelManager::GetInstance().LoadFile("./Assets/mono_textured_cube.kea");
    std::shared_ptr<Engine::Entity> cube(new Engine::Entity);
    cube->AttachMesh(Engine::ModelManager::GetInstance().models["MT_Cube"]);
    cube->matrix = Engine::Matrix4x4::ScalingMatrix({0.1f, 0.1f, 0.1f});
    engine.AddEntity(cube, false);
    #endif

    engine.RebuildCommandBuffers();

    Engine::Camera::GetInstance().SetPosition({0.0f, 3.0f, -5.0f});
    Engine::Camera::GetInstance().Rotate({0.0f, 30.0f, 0.0f});

    auto animation_start = std::chrono::system_clock::now();
    auto framerate_start = std::chrono::system_clock::now();
    auto animation_total_duration = std::chrono::milliseconds(1000);
    uint64_t frame_count = 0;

    // Boucle principale
    while(Engine::Window::Loop())
    {
        auto now = std::chrono::system_clock::now();

        auto animation_current_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - animation_start);
        float animation_ratio = static_cast<float>(animation_current_duration.count()) / static_cast<float>(animation_total_duration.count());
        if(animation_current_duration > animation_total_duration) animation_start = now;
        simple_guy->frame_index = static_cast<uint32_t>(animation_ratio * 30);

        static uint64_t fps = 0;
        auto framerate_current_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - framerate_start);
        if(framerate_current_duration > std::chrono::milliseconds(1000)) {
            framerate_start = now;
            fps = frame_count;
            frame_count = 0;
        }else{
            frame_count++;
        }

        bool inside_cam = Engine::Camera::GetInstance().GetFrustum().IsInside({0.0f, 0.0f, 0.0f});
        main_window->SetTitle("KazEngine [" + std::to_string(fps) + " FPS] "
                              + " [inside cam : " + std::to_string(inside_cam) + "] "
        #if defined(DISPLAY_LOGS)
                              + debug_me.display);
        debug_me.Update();
        #else
        );
        #endif

        engine.DrawScene();
    }

    // Libération des resources
    engine.DestroyInstance();
    vulkan.DestroyInstance();
    delete main_window;

    // simple_guy->Clear();

    #if defined(DISPLAY_LOGS)
    system("PAUSE");
    Engine::Console::Close();
    #endif

    return 0;
}
