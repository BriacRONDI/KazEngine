#include "../../Common/Window/Window.h"
#include <strsafe.h>

namespace Engine
{
    // static WindowList initilization
    std::map<HWND, Window*> Window::WindowList;

    Window::Window(Area<uint32_t> const& window_size, std::string const& title, FULL_SCREEN_MODE full_screen)
    {
        // Initialization
        this->Title = title;
        this->hWnd = NULL;

        // String to Wide string
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring wtitle = converter.from_bytes(title).data();

        // Create window
        WNDCLASSEX wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wcex.lpfnWndProc = Window::WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = NULL;
        wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = wtitle.data();
        wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

        // Class registration
        LPCTSTR lpClassName = reinterpret_cast<LPCTSTR>(RegisterClassEx(&wcex));
        if(lpClassName == nullptr) return;

        // Window style
        DWORD dwExStyle;
        DWORD dwStyle;

        // Get window size
        RECT WindowRect;
        WindowRect.left = 0;
        WindowRect.right = window_size.Width;
        WindowRect.top = 0;
        WindowRect.bottom = window_size.Height;

        if(full_screen == Window::FULLSCREEN_MODE_ENABLED)
        {
            // Enable fullscreen, change screen resolution if needed
            DEVMODE dmScreenSettings;
            memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
            dmScreenSettings.dmSize = sizeof(dmScreenSettings);
            dmScreenSettings.dmPelsWidth	= window_size.Width;
            dmScreenSettings.dmPelsHeight	= window_size.Height;
            dmScreenSettings.dmBitsPerPel	= 32;
            dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

            // Borderless style
            dwExStyle = WS_EX_APPWINDOW;
            dwStyle = WS_POPUP;

            // Hide mouse
            ShowCursor(FALSE);

            // Update state
            this->WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_FULLSCREEN;

        }else{

            // Windowed style
            dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
            // dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            dwStyle = WS_OVERLAPPEDWINDOW;

            // Update state
            this->WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED;
        }

        // Compute render surface excluding window borders if any
        AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

        // Create window
        this->hWnd = CreateWindowEx(dwExStyle,
                                    lpClassName,
                                    wtitle.data(),
                                    dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                    0,
                                    0,
                                    WindowRect.right - WindowRect.left,
                                    WindowRect.bottom - WindowRect.top,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

        //Centrage et affichage de la fenêtre
        this->Center();
        this->Show();

        // On ajoute la fenêtre à la liste
        Window::WindowList[this->hWnd] = this;
    }

    bool Window::ToggleFullScreen()
    {
        if(this->WindowState == IWindowListener::E_WINDOW_STATE::WINDOW_STATE_FULLSCREEN) {

            RECT rect = {
                static_cast<LONG>(this->mem_rect.X),
                static_cast<LONG>(this->mem_rect.Y),
                static_cast<LONG>(this->mem_rect.Width),
                static_cast<LONG>(this->mem_rect.Height)
            };

            SetWindowLongPtr(this->hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
            SetWindowLongPtr(this->hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
            SetWindowPos(this->hWnd, HWND_NOTOPMOST, rect.left, rect.top, rect.right, rect.bottom, SWP_SHOWWINDOW);
            
            if(Mouse::GetInstance().IsClipped()) {
                RECT client_rect;
                POINT client_origin = {0, 0};
                if(GetClientRect(this->hWnd, &client_rect) && ClientToScreen(this->hWnd, &client_origin)) {
                    Point<uint32_t> clip_offset = {static_cast<uint32_t>(client_origin.x), static_cast<uint32_t>(client_origin.y)};
                    Area<uint32_t> clip_size = {static_cast<uint32_t>(client_rect.right), static_cast<uint32_t>(client_rect.bottom)};
                    Mouse::GetInstance().Clip(clip_offset, clip_size);
                }
            }

            this->WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED;

        }else{

            RECT rect;
            GetWindowRect(this->hWnd, &rect);
            this->mem_rect = {
                static_cast<uint32_t>(rect.left),
                static_cast<uint32_t>(rect.top),
                static_cast<uint32_t>(rect.right - rect.left),
                static_cast<uint32_t>(rect.bottom - rect.top)
            };

            SetWindowLongPtr(this->hWnd, GWL_STYLE, WS_POPUP);
            SetWindowLongPtr(this->hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW);

            rect = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
            SetWindowPos(this->hWnd, HWND_TOPMOST, 0, 0, rect.right, rect.bottom, SWP_SHOWWINDOW);

            if(Mouse::GetInstance().IsClipped())
                Mouse::GetInstance().Clip({}, {static_cast<uint32_t>(rect.right), static_cast<uint32_t>(rect.bottom)});

            this->WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_FULLSCREEN;

            // Enable fullscreen, change screen resolution if needed
            /*DEVMODE dmScreenSettings = {};
            dmScreenSettings.dmSize         = sizeof(dmScreenSettings);
            dmScreenSettings.dmPelsWidth	= rect.right;
            dmScreenSettings.dmPelsHeight	= rect.bottom;
            dmScreenSettings.dmBitsPerPel	= 32;
            dmScreenSettings.dmFields       = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            int state = ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

            if(state == DISP_CHANGE_SUCCESSFUL) {
                this->WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_FULLSCREEN;
                return state == DISP_CHANGE_SUCCESSFUL;
            }else{
                return false;
            }*/
        }

        return true;
    }

    /**
     * Destructeur
     */
    Window::~Window()
    {
        // Destruction de la fenêtre
        if(this->hWnd != NULL) DestroyWindow(this->hWnd);

        // Retrait de la liste
        Window::WindowList.erase(this->hWnd);
    }

    /**
     * Récupère une surface de présentation pour Vulkan, indépendante de l'OS
     */
    Engine::Surface Window::GetSurface()
    {
        RECT rect;
        GetClientRect(this->hWnd, &rect);

        Engine::Surface surface;
        surface.handle = this->hWnd;
        surface.instance = nullptr;
        surface.width = rect.right - rect.left;
        surface.height = rect.bottom - rect.top;

        return surface;
    }

    /**
     * Getter : WindowState
     */
    IWindowListener::E_WINDOW_STATE Window::GetWindowState()
    {
        return this->WindowState;
    }

    /**
     * Centrage de la fenêtre
     */
    void Window::Center()
    {
        RECT rect;
        ::GetWindowRect(this->hWnd, &rect);
        SetWindowPos(this->hWnd, NULL, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, 0, 0, SWP_NOSIZE);
        
    }

    /**
     * Dissimulation de la fenêtre
     */
    void Window::Hide()
    {
        ShowWindow(this->hWnd, SW_HIDE);
    }

    /**
     * Affichage de la fenêtre
     */
    void Window::Show()
    {
        ShowWindow(this->hWnd, SW_SHOW);
    }

    /**
     * Change le titre de la fenêtre
     */
    void Window::SetTitle(std::string const& title)
    {
        // String to Wide string
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
        SetWindowText(this->hWnd, converter.from_bytes(title).data());
    }

    /**
     * Boucle principale permettant le traitement des messages envoyés par le système d'exploitation aux fenêtres
     * Si la fonction répond false, c'est que l'application a reçu le signal d'arrêt
     */
    bool Window::Loop()
    {
        MSG msg;

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                // Si le message WM_QUIT est reçu, on signale la fermeture de l'application
                return false;
            }else{
                //Gestion des messages de fenêtre
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // La boucle peut se poursuivre
        return true;
    }

    void Window::SetMouseCursor(CURSOR_TYPE cursor)
    {
        HCURSOR hCursor;

        switch(cursor) {
            case CURSOR_HAND :
                hCursor = LoadCursor(NULL, IDC_HAND);
                break;

            default :
                hCursor = LoadCursor(NULL, IDC_ARROW);
        }

        SetCursor(hCursor);
    }

    /**
     * Callback des messages Windows adressés aux fenêtres de l'application
     * Réception et traitement des messages Windows
     */
    LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            //La fenêtre est fermée par l'utilisateur
            case WM_CLOSE :
                PostQuitMessage(0);
                return TRUE;

            //La fenêtre est détruite
            case WM_DESTROY :
                return TRUE;

            //La fenêtre est est cours de redimensionnement
            case WM_SIZING :
            {
                // On retrouve la fenêtre appelante
                Window* self = Window::WindowList[hwnd];

                // La fenêtre dispose d'un ratio forcé
                if(self != nullptr) {

                    if(self->LockRatio > 0) {

                        // On force la conservation du ratio
                        RECT coordinates = *reinterpret_cast<RECT*>(lParam);
                        switch(wParam)
                        {
                            case WMSZ_LEFT :
                               coordinates.bottom = static_cast<LONG>((coordinates.right - coordinates.left) / self->LockRatio + coordinates.top);
                               *reinterpret_cast<RECT*>(lParam) = coordinates;
                               break;
   
                            case WMSZ_RIGHT :
                               coordinates.bottom = static_cast<LONG>((coordinates.right - coordinates.left) / self->LockRatio + coordinates.top);
                               *reinterpret_cast<RECT*>(lParam) = coordinates;
                               break;
   
                            case WMSZ_TOP :
                               coordinates.right = static_cast<LONG>((coordinates.bottom - coordinates.top) * self->LockRatio + coordinates.left);
                               *reinterpret_cast<RECT*>(lParam) = coordinates;
                               break;
   
                            case WMSZ_BOTTOM :
                               coordinates.right = static_cast<LONG>((coordinates.bottom - coordinates.top) * self->LockRatio + coordinates.left);
                               *reinterpret_cast<RECT*>(lParam) = coordinates;
                               break;
   
                            case WMSZ_TOPLEFT :
                               coordinates.left = static_cast<LONG>((coordinates.top - coordinates.bottom) * self->LockRatio + (coordinates.right));
                               *reinterpret_cast<RECT*>(lParam) = coordinates;
                               break;
               
                            case WMSZ_TOPRIGHT :
                               coordinates.right = static_cast<LONG>((coordinates.bottom - coordinates.top) * self->LockRatio + coordinates.left);
                               *reinterpret_cast<RECT*>(lParam) = coordinates;
                               break;
   
                            case WMSZ_BOTTOMLEFT :
                               coordinates.bottom = static_cast<LONG>((coordinates.right - coordinates.left) / self->LockRatio + (coordinates.top));
                               *reinterpret_cast<RECT*>(lParam) = coordinates;
                               break;
               
                            case WMSZ_BOTTOMRIGHT :
                               coordinates.bottom = static_cast<LONG>((coordinates.right - coordinates.left) / self->LockRatio + coordinates.top);
                               *reinterpret_cast<RECT*>(lParam) = coordinates;
                        }
                    }
                }
                return TRUE;
            }

            //La fenêtre est redimensionnée
            case WM_SIZE :
            {
                //On retrouve l'objet Window qui a prévoqué l'évènement
                Window* self = Window::WindowList[hwnd];

                if(self != NULL)
                {
                    //On parcours ses listeners pour les avertir du changement
                    std::vector<IWindowListener*> Listeners = self->GetListeners();
                    for(auto ListenerIterator = Listeners.begin(); ListenerIterator != Listeners.end(); ++ListenerIterator)
                    {
                        IWindowListener* Listener = *ListenerIterator;
                        IWindowListener::E_WINDOW_STATE WindowState = self->GetWindowState();

                        // La fenêtre a été minimisée
                        if(wParam == SIZE_MINIMIZED && WindowState != IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MINIMIZED)
                        {
                            WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MINIMIZED;
                            Listener->StateChanged(IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MINIMIZED);
                        // La fenêtre a été réstaurée
                        }else if((wParam == SIZE_RESTORED) && WindowState != IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED){
                            WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED;
                            Listener->StateChanged(IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED);
                        // La fenêtre a été maximisée
                        }else if((wParam == SIZE_MAXIMIZED) && WindowState != IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MAXIMIZED){
                            WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MAXIMIZED;
                            Listener->StateChanged(IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MAXIMIZED);
                        }
                        //On notifie les listeners de la nouvelle taille de la fenêtre
                        Listener->SizeChanged({LOWORD(lParam), HIWORD(lParam)});
                    }
                }
                return TRUE;
            }

            case WM_SYSCOMMAND :
            {
                if(wParam == SC_KEYMENU) {
                    if(lParam == VK_RETURN) {
                        Window* self = Window::WindowList[hwnd];
                        if(self != nullptr) self->ToggleFullScreen();
                    }
                    return TRUE;
                }
                break;
            }

            // Appui sur une touche du clavier
            case WM_KEYDOWN :
                if(wParam == VK_ESCAPE) Mouse::GetInstance().UnClip();
                else if(Mouse::GetInstance().IsClipped()) Keyboard::GetInstance().PressKey(static_cast<unsigned char>(wParam));
                return TRUE;

            // Relâchement d'une touche du clavier
            case WM_KEYUP :
                if(Mouse::GetInstance().IsClipped()) Keyboard::GetInstance().ReleaseKey(static_cast<unsigned char>(wParam));
                return TRUE;

            // La souris de séplace sur la fenêtre
            case WM_MOUSEMOVE :
                if(Mouse::GetInstance().IsClipped()) Mouse::GetInstance().MouseMove(LOWORD(lParam), HIWORD(lParam));
                return TRUE;

            case WM_MOUSELEAVE :
                if(Mouse::GetInstance().IsClipped()) Mouse::GetInstance().UnClip();
                return TRUE;

            case WM_KILLFOCUS :
                if(Mouse::GetInstance().IsClipped()) Mouse::GetInstance().UnClip();
                return TRUE;

            // La molette est utilisée
            case WM_MOUSEWHEEL :
                if(Mouse::GetInstance().IsClipped()) {
                    if(GET_WHEEL_DELTA_WPARAM(wParam) > 0) Mouse::GetInstance().MouseWheelUp();
                    else Mouse::GetInstance().MouseWheelDown();
                }
                return TRUE;

            // Bouton gauche de la souris enfoncé
            case WM_LBUTTONDOWN :
                Mouse::GetInstance().GetPosition() = {LOWORD(lParam), HIWORD(lParam)};
                if(Mouse::GetInstance().IsClipped()) Mouse::GetInstance().MouseButtonDown(IMouseListener::MOUSE_BUTTON_LEFT);
                return TRUE;

            // Bouton droit de la souris enfoncé
            case WM_RBUTTONDOWN :
                if(Mouse::GetInstance().IsClipped()) Mouse::GetInstance().MouseButtonDown(IMouseListener::MOUSE_BUTTON_RIGHT);
                return TRUE;

            // Bouton central de la souris enfoncé
            case WM_MBUTTONDOWN :
                if(Mouse::GetInstance().IsClipped()) Mouse::GetInstance().MouseButtonDown(IMouseListener::MOUSE_BUTTON_MIDDLE);
                return TRUE;

            // Bouton gauche de la souris relâché
            case WM_LBUTTONUP :
                if(!Mouse::GetInstance().IsClipped()) {
                    RECT client_rect;
                    POINT client_origin = {0, 0};
                    if(GetClientRect(hwnd, &client_rect) && ClientToScreen(hwnd, &client_origin)) {
                        Point<uint32_t> clip_offset = {static_cast<uint32_t>(client_origin.x), static_cast<uint32_t>(client_origin.y)};
                        Area<uint32_t> clip_size = {static_cast<uint32_t>(client_rect.right), static_cast<uint32_t>(client_rect.bottom)};
                        Mouse::GetInstance().Clip(clip_offset, clip_size);
                    }
                }else{
                    Mouse::GetInstance().MouseButtonUp(IMouseListener::MOUSE_BUTTON_LEFT);
                }
                return TRUE;

            // Bouton droit de la souris relâché
            case WM_RBUTTONUP :
                if(Mouse::GetInstance().IsClipped()) Mouse::GetInstance().MouseButtonUp(IMouseListener::MOUSE_BUTTON_RIGHT);
                return TRUE;

            // Bouton central de la souris relâché
            case WM_MBUTTONUP :
                if(Mouse::GetInstance().IsClipped()) Mouse::GetInstance().MouseButtonUp(IMouseListener::MOUSE_BUTTON_MIDDLE);
                return TRUE;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);;
    }
}
