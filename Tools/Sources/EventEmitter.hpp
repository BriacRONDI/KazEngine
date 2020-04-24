#pragma once

#include <vector>

namespace Tools
{
    template <class T>
    class EventEmitter
    {
        public:
            void AddListener(T* listener);
            void RemoveListener(T* listener);
            std::vector<T*> GetListeners();

        protected:
            std::vector<T*> Listeners;
    };

    template <class T>
    void EventEmitter<T>::AddListener(T* listener)
    {
        this->Listeners.push_back(listener);
    }

    template <class T>
    void EventEmitter<T>::RemoveListener(T* listener)
    {
        //On parcours la liste pour trouver le listener correspondant
        for(auto iterator = this->Listeners.begin(); iterator != this->Listeners.end(); iterator++)
        {
            //Si il est trouvé, on le supprime
            if(*iterator == listener)
            {
                this->Listeners.erase(iterator);
                return;
            }
        }
    }

    template <class T>
    std::vector<T*> EventEmitter<T>::GetListeners()
    {
        return this->Listeners;
    }
}


