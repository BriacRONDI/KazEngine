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
            };

            Camera();
            virtual ~Camera();
            inline CAMERA_UBO& GetUniformBuffer() { return this->camera; }  // Récupère la transformation finale
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

            CAMERA_UBO camera;                      // Uniform buffer de la caméra
            Mouse::MOUSE_POSITION mouse_origin;     // Position de la souris au moment du click

            float vertical_angle;                   // Rotation verticale
            float horizontal_angle;                 // Rotation horizontale

            Matrix4x4 rotation;                     // Matrice de rotation
            Matrix4x4 translation;                  // Matrice de translation
            Vector3 position;                       // Position de la caméra
    };
}