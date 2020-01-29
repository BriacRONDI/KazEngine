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
     * La souris se déplace
     */
    void Mouse::MouseMove(unsigned int x, unsigned int y)
    {
        // Récupère la postion de la souris
        this->position = {x , y};

        // Signale le déplacement de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseMove(x, y);
    }

    /*
     * Le bouton "button" est enfoncé
     */
    void Mouse::MouseButtonDown(IMouseListener::MOUSE_BUTTON button)
    {
        // Récupère l'état du bouton
        this->button_state[button] = true;

        // Signale l'appui du bouton de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseButtonDown(button);
    }

    /*
     * Le bouton "button" est relâché
     */
    void Mouse::MouseButtonUp(IMouseListener::MOUSE_BUTTON button)
    {
        // Récupère l'état du bouton
        this->button_state[button] = false;

        // Signale le relâchement du bouton de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseButtonUp(button);
    }

    /*
     * La molette est tournée vers le haut
     */
    void Mouse::MouseScrollUp()
    {
        // Signale le scroll de souris à tous les listeners
        for(auto& listener : this->Listeners)
            listener->MouseScrollUp();
    }

    /*
     * La molette est tournée vers le bas
     */
    void Mouse::MouseScrollDown()
    {
        // Signale le scroll de souris à tous les listeners
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
     * Indique si le bouton est enfoncé
     */
    bool Mouse::IsButtonPressed(IMouseListener::MOUSE_BUTTON button)
    {
        return this->button_state[button];
    }
}