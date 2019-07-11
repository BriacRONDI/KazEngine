#pragma once

#include "IWindowListener.h"
#include "EWindowState.h"
#include "../../../Core/EventEmitter.hpp"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#pragma comment(lib,"Version.lib")
#define NOMINMAX
#include <Windows.h>
#include <string>

#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#include <dlfcn.h>
#include <cstdlib>

#elif defined(VK_USE_PLATFORM_XLIB_KHR)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <cstdlib>

#endif

namespace Engine
{

    #if defined(VK_USE_PLATFORM_WIN32_KHR)
    typedef HMODULE LibraryHandle;

    #elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
    typedef void* LibraryHandle;

    #endif

    struct Surface
    {
        unsigned int width;
        unsigned int height;

        #if defined(VK_USE_PLATFORM_WIN32_KHR)
        HINSTANCE           instance;
        HWND                handle;

        #elif defined(VK_USE_PLATFORM_XCB_KHR)
        xcb_connection_t   *connection;
        xcb_window_t        handle;

        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        Display            *display;
        Window              handle;

        #endif

        Surface() : width(0), height(0) {}
    };

    class Window : public EventEmitter<IWindowListener>
    {
        public:

            enum FULL_SCREEN_MODE {
                FULLSCREEN_MODE_ENABLED = true,
                FULLSCREEN_MODE_DISABLED = false
            };

            Window(Area<uint32_t> window_size, std::string title, FULL_SCREEN_MODE full_screen);
            ~Window();

            E_WINDOW_STATE GetWindowState();
            Surface GetSurface();
            static bool Loop();

            void Center();
            void Hide();
            void Show();

        private:

            #if defined(VK_USE_PLATFORM_WIN32_KHR)
            HWND hWnd;                                                          // Windows API HWND
            std::string Title;                                                  // Titre de la fenêtre
            E_WINDOW_STATE WindowState;                                         // Etat de la fenêtre
            static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);     // Gestion des messages MS Windows
            #endif
    };
}
