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


    template <class T>
    class StaticEventEmitter
    {
        public:
            static inline void AddListener(T* listener);
            static inline void RemoveListener(T* listener);
            std::vector<T*> GetListeners();

        protected:
            static std::vector<T*> Listeners;
    };

    template <class T>
    std::vector<T*> StaticEventEmitter<T>::Listeners;

    template <class T>
    inline void StaticEventEmitter<T>::AddListener(T* listener)
    {
        StaticEventEmitter<T>::Listeners.push_back(listener);
    }

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

    template <class T>
    inline std::vector<T*> StaticEventEmitter<T>::GetListeners()
    {
        return StaticEventEmitter<T>::Listeners;
    }
}


