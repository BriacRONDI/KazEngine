#pragma once

namespace Engine
{
    /**
     * Liste des �tats d'une fen�tre
     */
    enum E_WINDOW_STATE
    {
        WINDOW_STATE_WINDOWED   = 0,    //Fen�tre avec bords, maximis�e ou non
        WINDOW_STATE_MINIMIZED  = 1,    //Fen�tre minimis�e
        WINDOW_STATE_FULLSCREEN = 2     //Mode plein �cran
    };
}
