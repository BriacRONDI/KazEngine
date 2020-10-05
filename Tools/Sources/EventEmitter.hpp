#pragma once

#include <vector>

namespace Tools
{
    template <class T>
    class EventEmitter
    {
        public:
            inline void AddListener(T* listener) { this->Listeners.push_back(listener); }
            inline void RemoveListener(T* listener);
            inline void ClearListeners() { this->Listeners.clear(); }
            inline std::vector<T*> GetListeners() { return this->Listeners; }

        protected:
            std::vector<T*> Listeners;
    };

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
    class StaticEventEmitter
    {
        public:
            static inline void AddListener(T* listener) { StaticEventEmitter<T>::Listeners.push_back(listener); }
            static inline void RemoveListener(T* listener);
            static inline std::vector<T*> GetListeners() { return StaticEventEmitter<T>::Listeners; }

        protected:
            static std::vector<T*> Listeners;
    };

    template <class T>
    std::vector<T*> StaticEventEmitter<T>::Listeners;

    template <class T>
    void StaticEventEmitter<T>::RemoveListener(T* listener)
    {
        //On parcours la liste pour trouver le listener correspondant
        for(auto iterator = StaticEventEmitter<T>::Listeners.begin(); iterator != StaticEventEmitter<T>::Listeners.end(); iterator++)
        {
            //Si il est trouvé, on le supprime
            if(*iterator == listener)
            {
                StaticEventEmitter<T>::Listeners.erase(iterator);
                return;
            }
        }
    }
}


