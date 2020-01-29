#pragma once

#include "Mouse.h"

namespace Engine
{
    Mouse* Mouse::Instance = new Mouse();

    Mouse::Mouse()
    {
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
    Mouse::MOUSE_POSITION const& Mouse::GetPosition()
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
}