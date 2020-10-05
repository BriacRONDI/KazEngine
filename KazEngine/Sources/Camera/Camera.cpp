#include "Camera.h"

namespace Engine
{
    // Instance du singleton
    Camera* Camera::instance = nullptr;

    void Camera::DestroyInstance()
    {
        if(Camera::instance == nullptr) return;
        delete Camera::instance;
        Camera::instance = nullptr;
    }

    /**
     * Constructeur
     */
    Camera::Camera() : frustum(this->camera.position, this->rotation)
    {
        this->near_clip_distance    = 0.1f;
        this->far_clip_distance     = 2000.0f;
        this->mouse_origin          = {};
        this->vertical_angle        = 0.0f;
        this->horizontal_angle      = 0.0f;
        this->position              = {0.0f, 0.0f, 0.0f};
        this->rotation              = Maths::Matrix4x4::RotationMatrix(this->vertical_angle, {1.0f, 0.0f, 0.0f}) * Maths::Matrix4x4::RotationMatrix(this->horizontal_angle, {0.0f, 1.0f, 0.0f});
        this->translation           = Maths::Matrix4x4::TranslationMatrix(this->position);
        this->camera.view           = this->rotation * this->translation;
        this->camera.projection     = Maths::Matrix4x4::PerspectiveProjectionMatrix(4.0f/3.0f, 60.0f, this->near_clip_distance, this->far_clip_distance);
        // this->camera.projection     = Matrix4x4::OrthographicProjectionMatrix(-5.0f, 5.0f, -5.0f, 5.0f, 0.0f, 30.0f);
        this->camera.position       = {0.0f, 0.0f, 0.0f};
        this->rts_mode              = true;
        
        if(!camera_descriptor.Create({
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            sizeof(Maths::Matrix4x4) * 2 + sizeof(std::array<Maths::Vector4,6>) + sizeof(Maths::Vector4)}
        })) return;

        this->frustum.Setup(4.0f/3.0f, 60.0f, 0.1f, 2000.0f);
        this->last_ubo.resize(Vulkan::GetConcurrentFrameCount());

        Mouse::GetInstance().AddListener(this);
    }

    /**
     * Destructeur
     */
    Camera::~Camera()
    {
        Mouse::GetInstance().RemoveListener(this);
    }

    void Camera::Update(uint8_t frame_index)
    {
        if(Mouse::GetInstance().IsClipped() && this->rts_mode && !this->frozen) this->RtsScroll();

        if(this->camera != this->last_ubo[frame_index]) {
            size_t offset = 0;
            this->camera_descriptor.WriteData(&this->camera, sizeof(Maths::Matrix4x4) * 2, offset);
            offset += sizeof(Maths::Matrix4x4) * 2;
            this->camera_descriptor.WriteData(&this->frustum.GetPlanes(), sizeof(std::array<Maths::Vector4,6>), offset);
            offset += sizeof(std::array<Maths::Vector4,6>);
            Maths::Vector4 camera_position = {-this->camera.position[0], this->camera.position[1], -this->camera.position[2], 1.0f};
            this->camera_descriptor.WriteData(&camera_position, sizeof(Maths::Vector4), offset);
            this->last_ubo[frame_index] = this->camera;
        }
    }

    void Camera::RtsScroll()
    {
        auto& mouse_position = Mouse::GetInstance().GetPosition();
        auto& surface = Vulkan::GetDrawSurface();
        bool update_camera = false;

        if(mouse_position.X > 0 && mouse_position.X < surface.width - 1) {
            if(this->rts_is_scrolling[0]) this->rts_is_scrolling[0] = false;
        }else{
            if(!this->rts_is_scrolling[0]) {
                this->scroll_start[0] = std::chrono::system_clock::now();
                this->rts_scroll_initial_position[0] = this->position.x;
                this->rts_is_scrolling[0] = true;
            }

            update_camera = true;
            auto scroll_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->scroll_start[0]);
            float scroll_length = this->rts_scroll_speed * this->position.y * static_cast<float>(scroll_duration.count());

            if(mouse_position.X == 0) this->position.x = this->rts_scroll_initial_position[0] + scroll_length;
            if(mouse_position.X == surface.width - 1) this->position.x = this->rts_scroll_initial_position[0] - scroll_length;
        }

        if(mouse_position.Y > 0 && mouse_position.Y < surface.height - 1) {
            if(this->rts_is_scrolling[1]) this->rts_is_scrolling[1] = false;
        }else{
            if(!this->rts_is_scrolling[1]) {
                this->scroll_start[1] = std::chrono::system_clock::now();
                this->rts_scroll_initial_position[1] = this->position.z;
                this->rts_is_scrolling[1] = true;
            }

            update_camera = true;
            auto scroll_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->scroll_start[1]);
            float scroll_length = this->rts_scroll_speed * this->position.y * static_cast<float>(scroll_duration.count());

            if(mouse_position.Y == 0) this->position.z = this->rts_scroll_initial_position[1] + scroll_length;
            if(mouse_position.Y == surface.height - 1) this->position.z = this->rts_scroll_initial_position[1] - scroll_length;
        }

        if(update_camera) {
            this->translation = Maths::Matrix4x4::TranslationMatrix(position);
            this->camera.view = this->rotation * this->translation;
            this->camera.position = this->position;
            this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);
        }
    }

    void Camera::RtsMove(unsigned int x, unsigned int y)
    {
        
    }

    void Camera::FpsMove(unsigned int x, unsigned int y)
    {
        if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_LEFT)) {

            int32_t delta_ix = this->mouse_origin.X - x;
            int32_t delta_iy = y - this->mouse_origin.Y;

            float sensitivity = 0.005f;
            float delta_fx = static_cast<float>(delta_ix) * sensitivity;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;

            this->moving_horizontal_angle = std::fmod(this->horizontal_angle + delta_fx, Maths::F_2_PI);
            if(this->moving_horizontal_angle < 0) this->moving_horizontal_angle += Maths::F_2_PI;

            this->moving_vertical_angle = this->vertical_angle + delta_fy;
            if(this->moving_vertical_angle > Maths::F_PI_2) this->moving_vertical_angle = Maths::F_PI_2;
            if(this->moving_vertical_angle < -Maths::F_PI_2) this->moving_vertical_angle = -Maths::F_PI_2;

            this->rotation = Maths::Matrix4x4::RotationMatrix(this->moving_vertical_angle, {1.0f, 0.0f, 0.0f})
                           * Maths::Matrix4x4::RotationMatrix(this->moving_horizontal_angle, {0.0f, 1.0f, 0.0f});
            this->camera.view = this->rotation * this->translation;
            this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);

        }else if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_MIDDLE)) {

            int32_t delta_ix = this->mouse_origin.X - x;
            int32_t delta_iy = y - this->mouse_origin.Y;

            float sensitivity = 0.01f;
            float delta_fx = static_cast<float>(delta_ix) * sensitivity;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;

            Maths::Vector3 strafe_x = this->GetRightVector() * delta_fx;
            Maths::Vector3 strafe_y = this->GetUpVector() * delta_fy;
            Maths::Vector3 position = this->position + strafe_x + strafe_y;
            this->translation = Maths::Matrix4x4::TranslationMatrix(position);
            this->camera.view = this->rotation * this->translation;
            this->camera.position = position;
            this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);
            
        }else if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_RIGHT)) {

            int32_t delta_iy = this->mouse_origin.Y - y;
            float sensitivity = 0.01f;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;
            
            Maths::Vector3 direction = this->GetFrontVector() * delta_fy;
            Maths::Vector3 position = this->position - direction;
            this->translation = Maths::Matrix4x4::TranslationMatrix(position);
            this->camera.view = this->rotation * this->translation;
            this->camera.position = position;
            this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);
        }
    }

    void Camera::MouseButtonDown(MOUSE_BUTTON button)
    {
        if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT || MOUSE_BUTTON::MOUSE_BUTTON_MIDDLE)
            this->mouse_origin = Mouse::GetInstance().GetPosition();
    }

    void Camera::MouseButtonUp(MOUSE_BUTTON button)
    {
        if(!this->rts_mode) {
            if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT) {

                Point<uint32_t> const& position = Mouse::GetInstance().GetPosition();

                int32_t delta_ix = this->mouse_origin.X - position.X;
                int32_t delta_iy = position.Y - this->mouse_origin.Y;

                float sensitivity = 0.005f;
                float delta_fx = static_cast<float>(delta_ix) * sensitivity;
                float delta_fy = static_cast<float>(delta_iy) * sensitivity;

                this->horizontal_angle += delta_fx;

                this->horizontal_angle = std::fmod(this->horizontal_angle, Maths::F_2_PI);
                if(this->horizontal_angle < 0) this->horizontal_angle += Maths::F_2_PI;

                this->vertical_angle += delta_fy;
                if(this->vertical_angle > Maths::F_PI_2) this->vertical_angle = Maths::F_PI_2;
                if(this->vertical_angle < -Maths::F_PI_2) this->vertical_angle = -Maths::F_PI_2;
        
            }else if(button == MOUSE_BUTTON::MOUSE_BUTTON_MIDDLE) {

                Point<uint32_t> const& position = Mouse::GetInstance().GetPosition();

                int32_t delta_ix = this->mouse_origin.X - position.X;
                int32_t delta_iy = position.Y - this->mouse_origin.Y;

                float sensitivity = 0.01f;
                float delta_fx = static_cast<float>(delta_ix) * sensitivity;
                float delta_fy = static_cast<float>(delta_iy) * sensitivity;

                Maths::Vector3 strafe_x = this->GetRightVector() * delta_fx;
                Maths::Vector3 strafe_y = this->GetUpVector() * delta_fy;
                this->position = this->position + strafe_x + strafe_y;
                this->translation = Maths::Matrix4x4::TranslationMatrix(this->position);
                this->camera.position = this->position;
                this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);

            }else if(button == MOUSE_BUTTON::MOUSE_BUTTON_RIGHT) {

                Point<uint32_t> const& mouse = Mouse::GetInstance().GetPosition();

                int32_t delta_iy = this->mouse_origin.Y - mouse.Y;
                float sensitivity = 0.01f;
                float delta_fy = static_cast<float>(delta_iy) * sensitivity;

                Maths::Vector3 direction = this->GetFrontVector() * delta_fy;
                this->position = this->position - direction;
                this->translation = Maths::Matrix4x4::TranslationMatrix(this->position);
                this->camera.view = this->rotation * this->translation;
                this->camera.position = this->position;
                this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);
            }
        }
    }

    void Camera::MouseWheelUp()
    {
        this->position = this->position - this->GetFrontVector();
        
        this->camera.position = this->position;
        this->translation = Maths::Matrix4x4::TranslationMatrix(this->position);
        this->camera.view = this->rotation * this->translation;
        this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);
    }

    void Camera::MouseWheelDown()
    {
        this->position = this->position + this->GetFrontVector();

        this->camera.position = this->position;
        this->translation = Maths::Matrix4x4::TranslationMatrix(this->position);
        this->camera.view = this->rotation * this->translation;
        this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);
    }

    void Camera::SetPosition(Maths::Vector3 const& position)
    {
        this->position = position;
        this->translation = Maths::Matrix4x4::TranslationMatrix(this->position);
        this->camera.view = this->rotation * this->translation;
        this->camera.position = position;
        this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);
    }

    /**
     * Rotation de la camera
     */
    void Camera::Rotate(Maths::Vector3 const& rotation)
    {
        this->horizontal_angle = rotation.x;
        this->horizontal_angle = std::fmod(this->horizontal_angle, Maths::F_2_PI);
        if(this->horizontal_angle < 0) this->horizontal_angle += Maths::F_2_PI;

        this->vertical_angle = rotation.y;
        if(this->vertical_angle > 90.0f) this->vertical_angle = Maths::F_PI_4;
        if(this->vertical_angle < -90.0f) this->vertical_angle = -Maths::F_PI_4;

        this->rotation = Maths::Matrix4x4::RotationMatrix(this->vertical_angle, {1.0f, 0.0f, 0.0f}) * Maths::Matrix4x4::RotationMatrix(this->horizontal_angle, {0.0f, 1.0f, 0.0f});
        this->camera.view = this->rotation * this->translation;
        this->frustum.UpdatePlanes(this->camera.projection * this->camera.view);
    }
}