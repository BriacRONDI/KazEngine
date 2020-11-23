#pragma once

template <class T>
class Singleton
{
    public:

        /// Create a unique instance of class T
        static T*& CreateInstance() { if(Singleton::instance == nullptr) Singleton::instance = new T; return Singleton::instance; }

        /// Get the unique instance of class T
        static T* GetInstance() { return Singleton::instance; }

        /// Destroy the unique instance of class T
        virtual void DestroyInstance() { delete this; };

    protected:

        Singleton() {};
        virtual ~Singleton() { Singleton::instance = nullptr; }

        static T* instance;
};

template <class T>
T* Singleton<T>::instance;