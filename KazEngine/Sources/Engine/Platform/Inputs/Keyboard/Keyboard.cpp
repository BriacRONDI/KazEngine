#include "Keyboard.h"

namespace Engine
{
    Keyboard* Keyboard::Instance = new Keyboard();

    /**
     * Constructeur privé, instanciation impossible
     */
    Keyboard::Keyboard()
    {
        //Aucune touche n'est enfoncée
        std::memset(this->KeyPressed, false, sizeof(this->KeyPressed));
    }

    /**
     * Attention, ne jamais oublier de détruire un singleton en fin d'utilisation!
     */
    Keyboard::~Keyboard()
    {
        this->Listeners.clear();
    }

    /**
     * Permet de savoir si une touche est enfoncée
     */
    bool Keyboard::IsPressed(const unsigned char Key)
    {
        return this->KeyPressed[Key];
    }

    /**
     * Signale au gestionnaire de clavier qu'une touche a été enfoncée.
     * Tous les listeners sont ainsi informés de l'action.
     */
    void Keyboard::PressKey(const unsigned char Key)
    {
        if(!this->KeyPressed[Key])
        {
            this->KeyPressed[Key] = true;

            //On notifie tous les listeners qu'un touche est enfoncée
            for(auto ListenerIterator = this->Listeners.begin(); ListenerIterator != this->Listeners.end(); ++ListenerIterator)
            {
                IKeyboardListener* Listener = *ListenerIterator;
                Listener->KeyDown(Key);
            }
        }
    }

    /**
     * Signale au gestionnaire de clavier qu'une touche a été relachée.
     * Tous les listeners sont ainsi informés de l'action.
     */
    void Keyboard::ReleaseKey(const unsigned char Key)
    {
        if(this->KeyPressed[Key])
        {
            this->KeyPressed[Key] = false;

            //On notifie tous les listeners qu'une touche est relâchée
            for(auto ListenerIterator = this->Listeners.begin(); ListenerIterator != this->Listeners.end(); ++ListenerIterator)
            {
                IKeyboardListener* Listener = *ListenerIterator;
                Listener->KeyUp(Key);
            }
        }
    }
}
