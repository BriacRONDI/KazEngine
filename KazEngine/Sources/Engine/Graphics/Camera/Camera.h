#pragma once

#include <chrono>

#include "../../Common/Maths/Maths.h"
#include "../../Common/Maths/Frustum.h"
#include "../../Platform/Inputs/Mouse/Mouse.h"
#include "../../Graphics/Vulkan/Vulkan.h"

#if defined(DISPLAY_LOGS)
#include <iostream>
#endif

namespace Engine
{
    class Camera : public IMouseListener
    {
        public:

            struct CAMERA_UBO {
                Matrix4x4 projection;
                Matrix4x4 view;
                Vector3 position;
            };

            // Singleton
            void DestroyInstance();
            static inline Camera& GetInstance() { if(Camera::instance == nullptr) Camera::instance = new Camera; return *Camera::instance; }

            inline CAMERA_UBO& GetUniformBuffer() { return this->camera; }  // Récupère la transformation finale
            void SetPosition(Vector3 const& position);                      // Modifie la position
            void Rotate(Vector3 const& rotation);                           // Rotation de la camera
            inline Frustum const& GetFrustum() { return this->frustum; }    // Récupère le frustum
            void Update();                                                  // Mise à jour de la caméra
            inline Vector3 GetFrontVector() const { return {-this->camera.view[2], -this->camera.view[6], -this->camera.view[10]}; }
            inline Vector3 GetRightVector() const { return {this->camera.view[0], this->camera.view[4], this->camera.view[8]}; }    
            inline Vector3 GetUpVector() const { return {-this->camera.view[1], -this->camera.view[5], -this->camera.view[9]}; }
            inline float GetNearClipDistance() const { return this->near_clip_distance; }
            inline float GetFarClipDistance() const { return this->far_clip_distance; }

            Matrix4x4 rotation;                     // Matrice de rotation

            ///////////////////////////
            ///    IMouseListener    //
            ///////////////////////////

            inline virtual void MouseMove(unsigned int x, unsigned int y) { this->rts_mode ? this->RtsMove(x, y) : this->FpsMove(x, y); }
            virtual void MouseButtonDown(MOUSE_BUTTON button);
            virtual void MouseButtonUp(MOUSE_BUTTON button);
            virtual void MouseScrollUp();
            virtual void MouseScrollDown();

        private:

            static Camera* instance;
            float near_clip_distance;
            float far_clip_distance;

            bool rts_mode;
            bool rts_is_scrolling[2] = {false, false};
            float rts_scroll_speed = 2.5f / 1000.0f;
            float rts_scroll_initial_position[2];
            std::chrono::system_clock::time_point scroll_start[2];

            CAMERA_UBO camera;                      // Uniform buffer de la caméra
            Point<uint32_t> mouse_origin;           // Position de la souris au moment du click

            float moving_vertical_angle;            // Rotation verticale en mouvement
            float moving_horizontal_angle;          // Rotation horizontale en mouvement
            float vertical_angle;                   // Rotation verticale
            float horizontal_angle;                 // Rotation horizontale

            //Matrix4x4 rotation;                     // Matrice de rotation
            Matrix4x4 translation;                  // Matrice de translation
            Vector3 position;                       // Position de la caméra

            Frustum frustum;                        // Frustum de la caméra

            Camera();
            virtual ~Camera();
            void RtsMove(unsigned int x, unsigned int y);
            void FpsMove(unsigned int x, unsigned int y);
            void RtsScroll();
    };
}