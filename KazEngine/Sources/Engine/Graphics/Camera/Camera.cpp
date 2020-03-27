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
        this->rotation              = Matrix4x4::RotationMatrix(this->vertical_angle, {1.0f, 0.0f, 0.0f}) * Matrix4x4::RotationMatrix(this->horizontal_angle, {0.0f, 1.0f, 0.0f});
        this->translation           = Matrix4x4::TranslationMatrix(this->position);
        this->camera.view           = this->rotation * this->translation;
        this->camera.projection     = Matrix4x4::PerspectiveProjectionMatrix(4.0f/3.0f, 60.0f, this->near_clip_distance, this->far_clip_distance);
        // this->camera.projection     = Matrix4x4::OrthographicProjectionMatrix(-5.0f, 5.0f, -5.0f, 5.0f, 0.0f, 30.0f);
        this->camera.position       = {0.0f, 0.0f, 0.0f};
        this->rts_move              = false;

        this->frustum.Setup(4.0f/3.0f, 60.0f, 0.1f, 2000.0f);
        Mouse::GetInstance().AddListener(this);
    }

    /**
     * Destructeur
     */
    Camera::~Camera()
    {
        Mouse::GetInstance().RemoveListener(this);
    }

    void Camera::Update()
    {
        if(this->rts_move && Mouse::GetInstance().IsClipped()) {
            auto& mouse_position = Mouse::GetInstance().GetPosition();
            auto& surface = Vulkan::GetDrawSurface();

            if(mouse_position.X > 0 && mouse_position.Y > 0 && mouse_position.X < surface.width - 1 && mouse_position.Y < surface.height - 1) return;

            float scroll_speed = 0.001f * this->position.y;
            if(mouse_position.X == 0) this->position.x += scroll_speed;
            if(mouse_position.Y == 0) this->position.z += scroll_speed;
            if(mouse_position.X == surface.width - 1) this->position.x -= scroll_speed;
            if(mouse_position.Y == surface.height - 1) this->position.z -= scroll_speed;
        
            this->translation = Matrix4x4::TranslationMatrix(position);
            this->camera.view = this->rotation * this->translation;
            this->camera.position = this->position;
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

            float sensitivity = 0.5f;
            float delta_fx = static_cast<float>(delta_ix) * sensitivity;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;

            this->moving_horizontal_angle = std::fmod(this->horizontal_angle + delta_fx, 360.0f);
            if(this->moving_horizontal_angle < 0) this->moving_horizontal_angle += 360.0f;

            this->moving_vertical_angle = this->vertical_angle + delta_fy;
            if(this->moving_vertical_angle > 90.0f) this->moving_vertical_angle = 90.0f;
            if(this->moving_vertical_angle < -90.0f) this->moving_vertical_angle = -90.0f;

            this->rotation = Matrix4x4::RotationMatrix(this->moving_vertical_angle, {1.0f, 0.0f, 0.0f})
                           * Matrix4x4::RotationMatrix(this->moving_horizontal_angle, {0.0f, 1.0f, 0.0f});
            this->camera.view = this->rotation * this->translation;

        }else if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_MIDDLE)) {

            int32_t delta_ix = this->mouse_origin.X - x;
            int32_t delta_iy = y - this->mouse_origin.Y;

            float sensitivity = 0.01f;
            float delta_fx = static_cast<float>(delta_ix) * sensitivity;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;

            Vector3 strafe_x = this->GetRightVector() * delta_fx;
            Vector3 strafe_y = this->GetUpVector() * delta_fy;
            Vector3 position = this->position + strafe_x + strafe_y;
            this->translation = Matrix4x4::TranslationMatrix(position);
            this->camera.view = this->rotation * this->translation;
            this->camera.position = position;
            
        }else if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_RIGHT)) {

            int32_t delta_iy = this->mouse_origin.Y - y;
            float sensitivity = 0.01f;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;
            
            Vector3 direction = this->GetFrontVector() * delta_fy;
            Vector3 position = this->position - direction;
            this->translation = Matrix4x4::TranslationMatrix(position);
            this->camera.view = this->rotation * this->translation;
            this->camera.position = position;
        }
    }

    void Camera::MouseButtonDown(MOUSE_BUTTON button)
    {
        if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT || MOUSE_BUTTON::MOUSE_BUTTON_MIDDLE)
            this->mouse_origin = Mouse::GetInstance().GetPosition();
    }

    void Camera::MouseButtonUp(MOUSE_BUTTON button)
    {
        if(!this->rts_move) {
            if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT) {

                Point<uint32_t> const& position = Mouse::GetInstance().GetPosition();

                int32_t delta_ix = this->mouse_origin.X - position.X;
                int32_t delta_iy = position.Y - this->mouse_origin.Y;

                float sensitivity = 0.5f;
                float delta_fx = static_cast<float>(delta_ix) * sensitivity;
                float delta_fy = static_cast<float>(delta_iy) * sensitivity;

                this->horizontal_angle += delta_fx;
                this->horizontal_angle = std::fmod(this->horizontal_angle, 360.0f);
                if(this->horizontal_angle < 0) this->horizontal_angle += 360.0f;

                this->vertical_angle += delta_fy;
                if(this->vertical_angle > 90.0f) this->vertical_angle = 90.0f;
                if(this->vertical_angle < -90.0f) this->vertical_angle = -90.0f;
        
            }else if(button == MOUSE_BUTTON::MOUSE_BUTTON_MIDDLE) {

                Point<uint32_t> const& position = Mouse::GetInstance().GetPosition();

                int32_t delta_ix = this->mouse_origin.X - position.X;
                int32_t delta_iy = position.Y - this->mouse_origin.Y;

                float sensitivity = 0.01f;
                float delta_fx = static_cast<float>(delta_ix) * sensitivity;
                float delta_fy = static_cast<float>(delta_iy) * sensitivity;

                Vector3 strafe_x = this->GetRightVector() * delta_fx;
                Vector3 strafe_y = this->GetUpVector() * delta_fy;
                this->position = this->position + strafe_x + strafe_y;
                this->translation = Matrix4x4::TranslationMatrix(this->position);
                this->camera.position = this->position;

            }else if(button == MOUSE_BUTTON::MOUSE_BUTTON_RIGHT) {

                Point<uint32_t> const& mouse = Mouse::GetInstance().GetPosition();

                int32_t delta_iy = this->mouse_origin.Y - mouse.Y;
                float sensitivity = 0.01f;
                float delta_fy = static_cast<float>(delta_iy) * sensitivity;

                Vector3 direction = this->GetFrontVector() * delta_fy;
                this->position = this->position - direction;
                this->translation = Matrix4x4::TranslationMatrix(this->position);
                this->camera.view = this->rotation * this->translation;
                this->camera.position = this->position;
            }
        }
    }

    void Camera::MouseScrollUp()
    {
        if(this->rts_move) {
            float min_height = 1.0f;
            float zoom_speed = 1.0f;
            this->position.y -= zoom_speed;
            if(this->position.y < min_height) this->position.y = min_height;

        }else{
            this->position = this->position - this->GetFrontVector();
        }
        
        this->camera.position = this->position;
        this->translation = Matrix4x4::TranslationMatrix(this->position);
        this->camera.view = this->rotation * this->translation;
    }

    void Camera::MouseScrollDown()
    {
        if(this->rts_move) {
            float max_height = 30.0f;
            float zoom_speed = 1.0f;
            this->position.y += zoom_speed;
            if(this->position.y > max_height) this->position.y = max_height;

        }else{
            this->position = this->position + this->GetFrontVector();
        }

        this->camera.position = this->position;
        this->translation = Matrix4x4::TranslationMatrix(this->position);
        this->camera.view = this->rotation * this->translation;
    }

    void Camera::SetPosition(Vector3 const& position)
    {
        this->position = position;
        this->translation = Matrix4x4::TranslationMatrix(this->position);
        this->camera.view = this->rotation * this->translation;
        this->camera.position = position;
    }

    /**
     * Rotation de la camera
     */
    void Camera::Rotate(Vector3 const& rotation)
    {
        this->horizontal_angle = rotation.x;
        this->horizontal_angle = std::fmod(this->horizontal_angle, 360.0f);
        if(this->horizontal_angle < 0) this->horizontal_angle += 360.0f;

        this->vertical_angle = rotation.y;
        if(this->vertical_angle > 90.0f) this->vertical_angle = 90.0f;
        if(this->vertical_angle < -90.0f) this->vertical_angle = -90.0f;

        this->rotation = Matrix4x4::RotationMatrix(this->vertical_angle, {1.0f, 0.0f, 0.0f}) * Matrix4x4::RotationMatrix(this->horizontal_angle, {0.0f, 1.0f, 0.0f});
        this->camera.view = this->rotation * this->translation;
    }
}