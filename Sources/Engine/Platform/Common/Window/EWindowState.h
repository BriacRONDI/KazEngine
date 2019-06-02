#pragma once

namespace Engine
{
    /**
     * Liste des états d'une fenêtre
     */
    enum E_WINDOW_STATE
    {
        WINDOW_STATE_WINDOWED   = 0,    //Fenêtre avec bords, maximisée ou non
        WINDOW_STATE_MINIMIZED  = 1,    //Fenêtre minimisée
        WINDOW_STATE_FULLSCREEN = 2     //Mode plein écran
    };
}
