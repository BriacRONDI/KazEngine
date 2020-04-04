#pragma once

#include "IWindowListener.h"
#include "../../../Common/EventEmitter.hpp"
#include "../../Inputs/Mouse/Mouse.h"
#include "../../Inputs/Keyboard/Keyboard.h"

#if defined(_WIN32)
#include <map>
#pragma comment(lib,"Version.lib")
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
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

    #if defined(_WIN32)
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

            enum CURSOR_TYPE {
                CURSOR_DEFAULT  = 0,
                CURSOR_HAND     = 1,
                CURSOR_NO       = 2
            };

            Window(Area<uint32_t> window_size, std::string title, FULL_SCREEN_MODE full_screen);
            ~Window();

            IWindowListener::E_WINDOW_STATE GetWindowState();
            Surface GetSurface();
            static bool Loop();
            inline Area<uint32_t> const& GetSize() { return this->Size; }
            void SetTitle(std::string const& title);
            static void SetMouseCursor(CURSOR_TYPE cursor);

            void Center();
            void Hide();
            void Show();

        private:

            // Le ratio de la fenêtre est figé
            float LockRatio = 4.0f / 3.0f;

            // Taille de la fenêtre
            Area<uint32_t> Size;

            #if defined(VK_USE_PLATFORM_WIN32_KHR)
            HWND hWnd;                                                          // Windows API HWND
            std::string Title;                                                  // Titre de la fenêtre
            IWindowListener::E_WINDOW_STATE WindowState;                        // Etat de la fenêtre
            static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);     // Gestion des messages MS Windows
            static std::map<HWND, Window*> WindowList;                          // Contient la liste de toutes les fenêtres crées
            #endif
    };
}
