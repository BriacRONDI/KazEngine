#pragma once

namespace Engine
{
    /**
     * Mouse events listener
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
             * Mouse move
             * @param x Mouse cursor position on X axis
             * @param x Mouse cursor position on Y axis
             */
            virtual void MouseMove(unsigned int x, unsigned int y) = 0;

            /**
             * Bouton pushed
             * @param button Mouse button pushed
             */
            virtual void MouseButtonDown(MOUSE_BUTTON button) = 0;

            /**
             * Bouton released
             * @param button Mouse button pushed
             */
            virtual void MouseButtonUp(MOUSE_BUTTON button) = 0;

            /**
             * Mouse wheel scrolled up
             */
            virtual void MouseWheelUp() = 0;

            /**
             * Mouse wheel scrolled down
             */
            virtual void MouseWheelDown() = 0;

            /**
             * Virtual desctructor
             */
            virtual ~IMouseListener(){};
    };
}