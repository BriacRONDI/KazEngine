#pragma once

namespace Engine
{
    /**
     * Cette interface permet � une classe d'�couter les �v�nements en provenance de la souris
     */
    class IMouseListener
    {
        public:
            enum MOUSE_BUTTON {
                MOUSE_BUTTON_LEFT       = 0,
                MOUSE_BUTTON_RIGHT      = 1,
                MOUSE_BUTTON_MIDDLE     = 2
            };

            /**
             * D�placement de la souris
             */
            virtual void MouseMove(unsigned int x, unsigned int y) = 0;

            /**
             * Bouton enfonc�
             */
            virtual void MouseButtonDown(MOUSE_BUTTON button) = 0;

            /**
             * Bouton rel�ch�
             */
            virtual void MouseButtonUp(MOUSE_BUTTON button) = 0;

            /**
             * Molette haut
             */
            virtual void MouseScrollUp() = 0;

            /**
             * Molette bas
             */
            virtual void MouseScrollDown() = 0;

            /**
             * Destructeur virtuel
             */
            virtual ~IMouseListener(){};
    };
}