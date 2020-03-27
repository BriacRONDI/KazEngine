#pragma once

#include "Mouse.h"

namespace Engine
{
    Mouse* Mouse::Instance = new Mouse();

    Mouse::Mouse()
    {
        this->clipped = false;
    }

    Mouse::~Mouse()
    {
    }

    Mouse& Mouse::GetInstance()
    {
        return *Mouse::Instance;
    }

    /*
     * La souris se d�place
     */
    void Mouse::MouseMove(unsigned int x, unsigned int y)
    {
        // R�cup�re la postion de la souris
        this->position = {x , y};

        // Signale le d�placement de souris � tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseMove(x, y);
    }

    /*
     * Le bouton "button" est enfonc�
     */
    void Mouse::MouseButtonDown(IMouseListener::MOUSE_BUTTON button)
    {
        // R�cup�re l'�tat du bouton
        this->button_state[button] = true;

        // Signale l'appui du bouton de souris � tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseButtonDown(button);
    }

    /*
     * Le bouton "button" est rel�ch�
     */
    void Mouse::MouseButtonUp(IMouseListener::MOUSE_BUTTON button)
    {
        // R�cup�re l'�tat du bouton
        this->button_state[button] = false;

        // Signale le rel�chement du bouton de souris � tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseButtonUp(button);
    }

    /*
     * La molette est tourn�e vers le haut
     */
    void Mouse::MouseScrollUp()
    {
        // Signale le scroll de souris � tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseScrollUp();
    }

    /*
     * La molette est tourn�e vers le bas
     */
    void Mouse::MouseScrollDown()
    {
        // Signale le scroll de souris � tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseScrollDown();
    }

    /**
     * Renvoie la position de la souris
     */
    Point<uint32_t> const& Mouse::GetPosition()
    {
        return this->position;
    }

    /**
     * Indique si le bouton est enfonc�
     */
    bool Mouse::IsButtonPressed(IMouseListener::MOUSE_BUTTON button)
    {
        return this->button_state[button];
    }

    /**
     * Contraint la souris � rester dans une zone de l'�cran
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
     * Lib�re la souris lorsqu'elle est contrainte � rester dans une zone de l'�cran
     */
    void Mouse::UnClip()
    {
        #if defined(_WIN32)
        ClipCursor(nullptr);
        #endif

        this->clipped = false;
    }
}