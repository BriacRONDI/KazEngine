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
    Camera::Camera()
    {
        this->mouse_origin          = {};
        this->vertical_angle        = 0.0f;
        this->horizontal_angle      = 0.0f;
        this->position              = {0.0f, 0.0f, 0.0f};
        this->rotation              = Matrix4x4::RotationMatrix(this->vertical_angle, {1.0f, 0.0f, 0.0f}) * Matrix4x4::RotationMatrix(this->horizontal_angle, {0.0f, 1.0f, 0.0f});
        this->translation           = Matrix4x4::TranslationMatrix(this->position);
        this->camera.view           = this->rotation * this->translation;
        this->camera.projection     = Matrix4x4::PerspectiveProjectionMatrix(4.0f/3.0f, 60.0f, 0.1f, 2000.0f);
        // this->camera.projection     = Matrix4x4::OrthographicProjectionMatrix(-5.0f, 5.0f, -5.0f, 5.0f, 0.0f, 30.0f);

        this->camera.position       = {0.0f, 0.0f, 0.0f};

        Mouse::GetInstance().AddListener(this);
    }

    /**
     * Destructeur
     */
    Camera::~Camera()
    {
        Mouse::GetInstance().RemoveListener(this);
    }

    void Camera::MouseMove(unsigned int x, unsigned int y)
    {
        if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_LEFT)) {

            int32_t delta_ix = this->mouse_origin.x - x;
            int32_t delta_iy = y - this->mouse_origin.y;

            float sensitivity = 0.5f;
            float delta_fx = static_cast<float>(delta_ix) * sensitivity;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;

            float horizontal;
            horizontal = std::fmod(this->horizontal_angle + delta_fx, 360.0f);
            if(horizontal < 0) horizontal += 360.0f;

            float vertical = this->vertical_angle + delta_fy;
            if(vertical > 90.0f) vertical = 90.0f;
            if(vertical < -90.0f) vertical = -90.0f;

            this->rotation = Matrix4x4::RotationMatrix(vertical, {1.0f, 0.0f, 0.0f}) * Matrix4x4::RotationMatrix(horizontal, {0.0f, 1.0f, 0.0f});
            this->camera.view = this->rotation * this->translation;

        }else if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_MIDDLE)) {

            int32_t delta_ix = this->mouse_origin.x - x;
            int32_t delta_iy = y - this->mouse_origin.y;

            float sensitivity = 0.01f;
            float delta_fx = static_cast<float>(delta_ix) * sensitivity;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;

            Vector3 strafe_x = Vector3({0.0f, 0.0f, delta_fx}) * Matrix4x4::RotationMatrix(90.0f, {0.0f, 1.0f, 0.0f}) * this->rotation;
            Vector3 strafe_y = Vector3({0.0f, 0.0f, delta_fy}) * Matrix4x4::RotationMatrix(90.0f, {1.0f, 0.0f, 0.0f}) * this->rotation;
            Vector3 position = this->position + strafe_x + strafe_y;
            this->translation = Matrix4x4::TranslationMatrix(position);
            this->camera.view = this->rotation * this->translation;
            this->camera.position = position;
            
        }else if(Mouse::GetInstance().IsButtonPressed(MOUSE_BUTTON::MOUSE_BUTTON_RIGHT)) {

            int32_t delta_iy = this->mouse_origin.y - y;
            float sensitivity = 0.01f;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;
            
            Vector3 direction = Vector3({0.0f, 0.0f, delta_fy}) * this->rotation;

            Vector3 position = this->position + direction;
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
        if(button == MOUSE_BUTTON::MOUSE_BUTTON_LEFT) {

            Mouse::MOUSE_POSITION const& position = Mouse::GetInstance().GetPosition();

            int32_t delta_ix = this->mouse_origin.x - position.x;
            int32_t delta_iy = position.y - this->mouse_origin.y;

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

            Mouse::MOUSE_POSITION const& position = Mouse::GetInstance().GetPosition();

            int32_t delta_ix = this->mouse_origin.x - position.x;
            int32_t delta_iy = position.y - this->mouse_origin.y;

            float sensitivity = 0.01f;
            float delta_fx = static_cast<float>(delta_ix) * sensitivity;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;

            Vector3 strafe_x = Vector3({0.0f, 0.0f, delta_fx}) * Matrix4x4::RotationMatrix(90.0f, {0.0f, 1.0f, 0.0f}) * this->rotation;
            Vector3 strafe_y = Vector3({0.0f, 0.0f, delta_fy}) * Matrix4x4::RotationMatrix(90.0f, {1.0f, 0.0f, 0.0f}) * this->rotation;
            this->position = this->position + strafe_x + strafe_y;
            this->translation = Matrix4x4::TranslationMatrix(this->position);
            this->camera.position = this->position;

        }else if(button == MOUSE_BUTTON::MOUSE_BUTTON_RIGHT) {

            Mouse::MOUSE_POSITION const& mouse = Mouse::GetInstance().GetPosition();

            int32_t delta_iy = this->mouse_origin.y - mouse.y;
            float sensitivity = 0.01f;
            float delta_fy = static_cast<float>(delta_iy) * sensitivity;
            
            Vector3 direction = Vector3({0.0f, 0.0f, delta_fy}) * this->rotation;

            this->position = this->position + direction;
            this->translation = Matrix4x4::TranslationMatrix(this->position);
            this->camera.view = this->rotation * this->translation;
            this->camera.position = this->position;
        }
    }

    void Camera::MouseScrollUp()
    {
        Vector3 direction = Vector3({0.0f, 0.0f, 1.0f}) * this->rotation;
        this->position = this->position + direction;
        this->translation = Matrix4x4::TranslationMatrix(this->position);
        this->camera.view = this->rotation * this->translation;
    }

    void Camera::MouseScrollDown()
    {
        Vector3 direction = Vector3({0.0f, 0.0f, 1.0f}) * this->rotation;
        this->position = this->position - direction;
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