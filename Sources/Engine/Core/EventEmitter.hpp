#pragma once

#include <vector>

namespace Engine
{
    template <class T>
    class EventEmitter
    {
        public:
            void AddListener(T* Listener);
            void RemoveListener(T* Listener);
            std::vector<T*> GetListeners();

        protected:
            std::vector<T*> Listeners;
    };

    /**
    * Ajout d'un listeneur
    */
    template <class T>
    void Engine::EventEmitter<T>::AddListener(T* Listener)
    {
        this->Listeners.push_back(Listener);
    }

    /**
    * Retrait d'un listeneur
    */
    template <class T>
    void Engine::EventEmitter<T>::RemoveListener(T* Listener)
    {
        //On parcours la liste pour trouver le listener correspondant
        for(auto ListenerIterator = this->Listeners.begin(); ListenerIterator != this->Listeners.end(); ++ListenerIterator)
        {
            //Si il est trouvé, on le supprime
            if(*ListenerIterator == Listener)
            {
                this->Listeners.erase(ListenerIterator);
                return;
            }
        }
    }

    /**
    * Getter Listeners
    */
    template <class T>
    std::vector<T*> Engine::EventEmitter<T>::GetListeners()
    {
        return this->Listeners;
    }
}


