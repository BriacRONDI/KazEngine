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

        Engine::Matrix4x4* cross_matrix;
        Engine::Matrix4x4* fleche_matrix;

        std::string display;
        float rot_h = 0.0f, rot_v = 0.0f;
        float rot_speed = 0.03f;
        float mov_speed = 0.005f;

        void Update()
        {

            /*if(Engine::Keyboard::GetInstance()->IsPressed(VK_LEFT))  this->rot_h -= this->rot_speed;
            if(Engine::Keyboard::GetInstance()->IsPressed(VK_RIGHT)) this->rot_h += this->rot_speed;
            if(Engine::Keyboard::GetInstance()->IsPressed(VK_ADD)) this->rot_v += this->rot_speed;
            if(Engine::Keyboard::GetInstance()->IsPressed(VK_SUBTRACT)) this->rot_v -= this->rot_speed;

            Engine::Matrix4x4 rotation = Engine::Matrix4x4::RotationMatrix(this->rot_h, {0.0f, 1.0f, 0.0f}) * Engine::Matrix4x4::RotationMatrix(this->rot_v, {1.0f, 0.0f, 0.0f});
            Engine::Matrix4x4 direction = rotation * Engine::Matrix4x4::TranslationMatrix({0.0f, 0.0f, 1.0f});
            Engine::Vector3 movement = {direction[12], direction[13], direction[14]};*/

            auto& camera = Engine::Camera::GetInstance();
            Engine::Area<float> const& near_plane_size = camera.GetFrustum().GetNearPlaneSize();
            Engine::Vector3 camera_position = -camera.GetUniformBuffer().position;
            Engine::Point<uint32_t> const& mouse_position = Engine::Mouse::GetInstance().GetPosition();
            Engine::Surface& draw_surface = Engine::Vulkan::GetDrawSurface();
            Engine::Area<float> float_draw_surface = {static_cast<float>(draw_surface.width), static_cast<float>(draw_surface.height)};

            float x = static_cast<float>(mouse_position.X) - float_draw_surface.Width / 2.0f;
            float y = static_cast<float>(mouse_position.Y) - float_draw_surface.Height / 2.0f;

            x /= float_draw_surface.Width / 2.0f;
            y /= float_draw_surface.Height / 2.0f;

            Engine::Vector3 mouse_world_position = camera_position + camera.GetFrontVector() * (camera.GetNearClipDistance() + 0.0f) + camera.GetRightVector() * near_plane_size.Width * x - camera.GetUpVector() * near_plane_size.Height * y;
            Engine::Vector3 mouse_ray = mouse_world_position - camera_position;
            mouse_ray = mouse_ray.Normalize();

            *this->fleche_matrix = Engine::Matrix4x4::TranslationMatrix(mouse_ray) * Engine::Matrix4x4::TranslationMatrix({0,-1,0}) * Engine::Matrix4x4::ScalingMatrix({0.25f, 0.25f, 0.25f});
            *this->cross_matrix = Engine::Matrix4x4::TranslationMatrix(mouse_world_position) * camera.rotation.Transpose() * Engine::Matrix4x4::ScalingMatrix({0.001f, 0.001f, 0.001f});
        }

        virtual void KeyDown(unsigned char Key)
        {
            if(Key == VK_SPACE) {
                Engine::Camera::GetInstance().SetPosition({});
                Engine::Camera::GetInstance().Rotate({});
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

    std::shared_ptr<Engine::Mesh> mesh_cross(new Engine::Mesh);
    mesh_cross->vertex_buffer = {{-1.0f, -1.0f, 0.0f},{1.0f, 1.0f, 0.0f},{1.0f, -1.0f, 0.0f},{-1.0f, 1.0f, 0.0f}};
    mesh_cross->index_buffer = {0,1,2,3};
    mesh_cross->UpdateRenderMask();
    std::shared_ptr<Engine::DefaultColorEntity> cross(new Engine::DefaultColorEntity);
    cross->AttachMesh(mesh_cross);
    cross->default_color = {1.0f, 0.0f, 0.0f};
    cross->matrix = Engine::Matrix4x4::ScalingMatrix({0.01f, 0.01f, 0.01f});
    // debug_me.cross_matrix = &cross->matrix;
    // engine.AddEntity(cross, false);

    Engine::ModelManager::GetInstance().LoadFile("./Assets/SimpleGuy.kea");
    std::shared_ptr<Engine::SkeletonEntity> simple_guy(new Engine::SkeletonEntity);
    simple_guy->AttachMesh(Engine::ModelManager::GetInstance().models["Body"]);
    Engine::ModelManager::GetInstance().models["Body"]->hit_box.near_left_bottom_point = {-0.25f, -1.3f, 0.25f};
    Engine::ModelManager::GetInstance().models["Body"]->hit_box.far_right_top_point = {0.25f, 0.0f, -0.25f};
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
    Engine::ModelManager::GetInstance().models["MT_Cube"]->hit_box.near_left_bottom_point = {-0.1f, -1.1f, 0.1f};
    Engine::ModelManager::GetInstance().models["MT_Cube"]->hit_box.far_right_top_point = {0.1f, -0.9f, -0.1f};
    cube->matrix = Engine::Matrix4x4::TranslationMatrix({0.0f, -1.0f, 0.0f}) * Engine::Matrix4x4::ScalingMatrix({0.1f, 0.1f, 0.1f});
    // engine.AddEntity(cube, false);
    #endif

    engine.RebuildCommandBuffers();

    Engine::Camera::GetInstance().SetPosition({0.0f, 5.0f, -5.0f});
    Engine::Camera::GetInstance().Rotate({0.0f, 45.0f, 0.0f});

    // auto animation_start = std::chrono::system_clock::now();
    auto framerate_start = std::chrono::system_clock::now();
    // auto animation_total_duration = std::chrono::milliseconds(1000);
    uint64_t frame_count = 0;

    

    // Boucle principale
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
                main_window->SetTitle("KazEngine [" + std::to_string(fps) + " FPS] (Appuyez sur ESC pour libérer la souris)");
            }else{
                main_window->SetTitle("KazEngine [" + std::to_string(fps) + " FPS] (Cliquez sur le fenêtre pour capturer la souris)");
            }
        }else{
            frame_count++;
        }

        if(Engine::Mouse::GetInstance().IsClipped()) Engine::Window::SetMouseCursor(Engine::Window::CURSOR_TYPE::CURSOR_HAND);

        Engine::Camera::GetInstance().Update();
        engine.DrawScene();
        // debug_me.Update();
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
