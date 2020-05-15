#pragma once

#include "Mouse.h"

namespace Engine
{
    Mouse Mouse::instance;

    Mouse::Mouse()
    {
        this->clipped = false;
    }

    Mouse::~Mouse()
    {
        this->Listeners.clear();
    }

    /**
     * The mouse is moving over screen
     */
    void Mouse::MouseMove(unsigned int x, unsigned int y)
    {
        // Récupère la postion de la souris
        this->position = {x , y};

        // Signale le déplacement de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseMove(x, y);
    }

    /**
     * A button is pushed
     */
    void Mouse::MouseButtonDown(IMouseListener::MOUSE_BUTTON button)
    {
        // Récupère l'état du bouton
        this->button_state[button] = true;

        // Signale l'appui du bouton de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseButtonDown(button);
    }

    /**
     * A button is released
     */
    void Mouse::MouseButtonUp(IMouseListener::MOUSE_BUTTON button)
    {
        // Récupère l'état du bouton
        this->button_state[button] = false;

        // Signale le relâchement du bouton de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseButtonUp(button);
    }

    /**
     * The mouse wheel is scrolled up
     */
    void Mouse::MouseWheelUp()
    {
        // Signale le scroll de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseWheelUp();
    }

    /**
     * The mouse wheel is scrolled down
     */
    void Mouse::MouseWheelDown()
    {
        // Signale le scroll de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseWheelDown();
    }

    /**
     * CHeck if a mouse button is pressed
     */
    bool Mouse::IsButtonPressed(IMouseListener::MOUSE_BUTTON button)
    {
        return this->button_state[button];
    }

    /**
     * Confines the cursor to a rectangular area on the screen
     */
    void Mouse::Clip(Point<uint32_t> const& clip_origin, Area<uint32_t> const& clip_size)
    {
        #if defined(_WIN32)
        RECT clip_area;
        clip_area.left = clip_origin.X;
        clip_area.top = clip_origin.Y;
        clip_area.right = clip_origin.X + clip_size.Width;
        clip_area.bottom = clip_origin.Y + clip_size.Height;
        ClipCursor(&clip_area);
        #endif

        this->clipped = true;
    }

    /**
     * Free cursor movement
     */
    void Mouse::UnClip()
    {
        #if defined(_WIN32)
        ClipCursor(nullptr);
        #endif

        this->clipped = false;
    }
}