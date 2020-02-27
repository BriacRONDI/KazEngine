#pragma once

#include "../../Common/Maths/Maths.h"
#include "../../Platform/Inputs/Mouse/Mouse.h"

#if defined(DISPLAY_LOGS)
#include <iostream>
#include <chrono>
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

            void DestroyInstance();
            static inline Camera& GetInstance() { if(Camera::instance == nullptr) Camera::instance = new Camera; return *Camera::instance; }
            inline CAMERA_UBO& GetUniformBuffer() { return this->camera; }  // R�cup�re la transformation finale
            void SetPosition(Vector3 const& position);                      // Modifie la position
            void Rotate(Vector3 const& rotation);                           // Rotation de la camera

            ///////////////////////////
            ///    IMouseListener    //
            ///////////////////////////

            virtual void MouseMove(unsigned int x, unsigned int y);
            virtual void MouseButtonDown(MOUSE_BUTTON button);
            virtual void MouseButtonUp(MOUSE_BUTTON button);
            virtual void MouseScrollUp();
            virtual void MouseScrollDown();

        private:

            static Camera* instance;

            CAMERA_UBO camera;                      // Uniform buffer de la cam�ra
            Mouse::MOUSE_POSITION mouse_origin;     // Position de la souris au moment du click

            float vertical_angle;                   // Rotation verticale
            float horizontal_angle;                 // Rotation horizontale

            Matrix4x4 rotation;                     // Matrice de rotation
            Matrix4x4 translation;                  // Matrice de translation
            Vector3 position;                       // Position de la cam�ra

            Camera();
            virtual ~Camera();
    };
}