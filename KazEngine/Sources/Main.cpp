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

        Engine::Vector3 position;
        Engine::Vector3 normal_left;
        Engine::Vector3 normal_right;
        Engine::Vector3 normal_top;
        Engine::Vector3 normal_bottom;
        Engine::Vector3 normal_near;
        Engine::Vector3 normal_far;

        Engine::Vector3 point_flt;
        Engine::Vector3 point_nrb;

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

            Engine::Vector4 normal = rotation * Engine::Matrix4x4::RotationMatrix(-45.0f, {0.0f, 1.0f, 0.0f}) * Engine::Vector4({1.0f, 0.0f, 0.0f, 1.0f});
            this->normal_left = {normal[0], normal[1], normal[2]};

            normal = rotation * Engine::Matrix4x4::RotationMatrix(45.0f, {0.0f, 1.0f, 0.0f}) * Engine::Vector4({-1.0f, 0.0f, 0.0f, 1.0f});
            this->normal_right = {normal[0], normal[1], normal[2]};

            normal = rotation * Engine::Vector4({0.0f, 1.0f, 0.0f, 1.0f});
            this->normal_top = {normal[0], normal[1], normal[2]};
            this->normal_bottom = this->normal_top * -1;

            normal = rotation * Engine::Vector4({0.0f, 0.0f, 1.0f, 1.0f});
            this->normal_far = {normal[0], normal[1], normal[2]};
            this->normal_near = this->normal_far * -1;

            if(Engine::Keyboard::GetInstance()->IsPressed(VK_UP)) this->position = this->position - movement * this->mov_speed;
            if(Engine::Keyboard::GetInstance()->IsPressed(VK_DOWN)) this->position = this->position + movement * this->mov_speed;

            Engine::Matrix4x4 point_flt = Engine::Matrix4x4::TranslationMatrix(this->position) * rotation * Engine::Matrix4x4::TranslationMatrix({-9.0f, -3.0f, -3.0f});
            this->point_flt = {point_flt[12], point_flt[13], point_flt[14]};

            Engine::Matrix4x4 point_nrb = Engine::Matrix4x4::TranslationMatrix(this->position) * rotation * Engine::Matrix4x4::TranslationMatrix({3.0f, 3.0f, 3.0f});
            this->point_nrb = {point_nrb[12], point_nrb[13], point_nrb[14]};

            *this->frustum_matrix = Engine::Matrix4x4::TranslationMatrix(this->position) * rotation * Engine::Matrix4x4::ScalingMatrix({3.0f, 3.0f, 3.0f});

            Engine::Matrix4x4 rotation_fleche = Engine::Matrix4x4::RotationMatrix(this->rot_h, {0.0f, 1.0f, 0.0f}) * Engine::Matrix4x4::RotationMatrix(this->rot_v, {3.0f, 0.0f, 0.0f});
            *this->fleche_matrix = Engine::Matrix4x4::TranslationMatrix(this->point_flt) * Engine::Matrix4x4::ScalingMatrix({0.25f, 0.25f, 0.25f});

            this->display = "[NL " + std::to_string(this->normal_left.x) + ", " + std::to_string(this->normal_left.y) + ", " + std::to_string(this->normal_left.z) + ", " + "]";
        }

        virtual void KeyDown(unsigned char Key)
        {
            if(Key == VK_SPACE) {
                this->rot_h = 0.0f;
                this->rot_v = 0.0f;
                this->position = {0.0f, 0.0f, 0.0f};
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

    /*Engine::ModelManager::GetInstance().LoadFile("./Assets/SimpleGuy.kea");
    std::shared_ptr<Engine::SkeletonEntity> simple_guy(new Engine::SkeletonEntity);
    simple_guy->AttachMesh(Engine::ModelManager::GetInstance().models["Body"]);
    engine.AddEntity(simple_guy, false);
    simple_guy->SetAnimation("Armature|Walk");*/

    Engine::ModelManager::GetInstance().LoadFile("./Assets/fleche.kea");
    std::shared_ptr<Engine::Entity> fleche(new Engine::Entity);
    fleche->AttachMesh(Engine::ModelManager::GetInstance().models["Fleche"]);
    debug_me.fleche_matrix = &fleche->matrix;
    engine.AddEntity(fleche, false);

    Engine::ModelManager::GetInstance().LoadFile("./Assets/mono_textured_cube.kea");
    std::shared_ptr<Engine::Entity> cube(new Engine::Entity);
    cube->AttachMesh(Engine::ModelManager::GetInstance().models["MT_Cube"]);
    cube->matrix = Engine::Matrix4x4::ScalingMatrix({0.1f, 0.1f, 0.1f});
    engine.AddEntity(cube, false);

    std::shared_ptr<Engine::Mesh> frustum_mesh(new Engine::Mesh);
    frustum_mesh->vertex_buffer.resize(8);
    frustum_mesh->vertex_buffer[0] = {-1.0f, -1.0f, 1.0f};
    frustum_mesh->vertex_buffer[1] = {-3.0f, -1.0f, -1.0f};
    frustum_mesh->vertex_buffer[2] = {3.0f, -1.0f, -1.0f};
    frustum_mesh->vertex_buffer[3] = {1.0f, -1.0f, 1.0f};

    frustum_mesh->vertex_buffer[4] = {-1.0f, 1.0f, 1.0f};
    frustum_mesh->vertex_buffer[5] = {-3.0f, 1.0f, -1.0f};
    frustum_mesh->vertex_buffer[6] = {3.0f, 1.0f, -1.0f};
    frustum_mesh->vertex_buffer[7] = {1.0f, 1.0f, 1.0f};

    frustum_mesh->index_buffer = {0,4,5,0,5,1,0,3,4,4,3,7,2,1,5,2,5,6,6,7,3,6,3,2,0,1,3,1,2,3,5,4,7,5,7,6};
    // frustum_mesh->index_buffer = {0,4,5,0,5,1};
    frustum_mesh->UpdateRenderMask();

    std::shared_ptr<Engine::DefaultColorEntity> frustum_model(new Engine::DefaultColorEntity);
    frustum_model->AttachMesh(frustum_mesh);
    frustum_model->default_color = {1.0f, 0.0f, 0.0f};
    engine.AddEntity(frustum_model, false);
    debug_me.position = {0.0f, 0.0f, 0.0f};
    debug_me.frustum_matrix = &frustum_model->matrix;
    engine.RebuildCommandBuffers();

    // Engine::Camera::GetInstance().SetPosition({0.0f, 3.0f, -5.0f});
    // Engine::Camera::GetInstance().Rotate({0.0f, 30.0f, 0.0f});

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
        // simple_guy->frame_index = static_cast<uint32_t>(animation_ratio * 30);

        static uint64_t fps = 0;
        auto framerate_current_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - framerate_start);
        if(framerate_current_duration > std::chrono::milliseconds(1000)) {
            framerate_start = now;
            fps = frame_count;
            frame_count = 0;
        }else{
            frame_count++;
        }

        Engine::Vector3 point = {0.0f, 0.0f, 0.0f};
        /* auto camera = Engine::Camera::GetInstance().GetUniformBuffer();
        Engine::Matrix4x4 combomatrix = camera.projection * camera.view.Transpose();
        point = camera.projection * camera.view * point;*/

        // Engine::Frustum const& frustum = Engine::Camera::GetInstance().GetFrustum();
        float dist_left = debug_me.normal_left.Dot(point - debug_me.point_flt);
        float dist_right = debug_me.normal_right.Dot(point - debug_me.point_nrb);
        float dist_top = debug_me.normal_top.Dot(point - debug_me.point_flt);
        float dist_bottom = debug_me.normal_bottom.Dot(point - debug_me.point_nrb);
        float dist_near = debug_me.normal_near.Dot(point - debug_me.point_nrb);
        float dist_far = debug_me.normal_far.Dot(point - debug_me.point_flt);

        bool inside = dist_left >= 0
                   && dist_right >= 0
                   && dist_top >= 0
                   && dist_bottom >= 0
                   && dist_near >= 0
                   && dist_far >= 0;

        main_window->SetTitle("KazEngine [" + std::to_string(fps) + " FPS] "
                              + " [L : " + std::to_string(dist_left) + "] "
                              + " [R : " + std::to_string(dist_right) + "] "
                              + " [T : " + std::to_string(dist_top) + "] "
                              + " [B : " + std::to_string(dist_bottom) + "] "
                              + " [N : " + std::to_string(dist_near) + "] "
                              + " [F : " + std::to_string(dist_far) + "] "
                              + " [inside : " + std::to_string(inside) + "] "
                              + debug_me.display);

        engine.DrawScene();

        debug_me.Update();
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
