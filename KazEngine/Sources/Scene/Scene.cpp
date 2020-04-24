#include "Scene.h"

namespace Engine
{
    Scene* Scene::instance = nullptr;

    bool Scene::Initialize()
    {
        if(Scene::instance == nullptr) Scene::instance = new Scene;
        return true;
    }
}