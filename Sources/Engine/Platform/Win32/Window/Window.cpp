#include "../../Common/Window/Window.h"
#include <strsafe.h>

namespace Engine
{
    Window::Window(Area<uint32_t> window_size, std::string title, FULL_SCREEN_MODE full_screen)
    {
        // Initialisation
        this->Title = title;
        this->hWnd = NULL;

        //Création de la fenêtre
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
        wcex.lpszClassName = this->Title.c_str();
        wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

        // Échec lors de l'enregistrement de la classe, on s'arrête
        LPCSTR lpClassName = (LPCTSTR)(uintptr_t)RegisterClassEx(&wcex);
        if(!lpClassName) return;

        //Initialisation
        DWORD dwExStyle;
        DWORD dwStyle;

        //Calcule de la taille réelle de la fenêtre
        RECT WindowRect;
        WindowRect.left = 0;
        WindowRect.right = window_size.Width;
        WindowRect.top = 0;
        WindowRect.bottom = window_size.Height;

        if(full_screen == Window::FULLSCREEN_MODE_ENABLED)
        {
            //Mode plein écran, changement de la résolution de l'écran si nécessaire
            DEVMODE dmScreenSettings;
            memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
            dmScreenSettings.dmSize = sizeof(dmScreenSettings);
            dmScreenSettings.dmPelsWidth	= window_size.Width;
            dmScreenSettings.dmPelsHeight	= window_size.Height;
            dmScreenSettings.dmBitsPerPel	= 32;
            dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

            //On indique que la fenêtre est sans bords
            dwExStyle = WS_EX_APPWINDOW;
            dwStyle = WS_POPUP;

            //On cache la souris
            ShowCursor(FALSE);

            // La fenêtre est en mode plein écran
            this->WindowState = WINDOW_STATE_FULLSCREEN;

        }else{ //Mode fenêtré

            //Style de la fenête (avec ou sans bords, taille fixe ou non etc...)
            dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
            dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            //dwStyle = WS_OVERLAPPEDWINDOW;

            // La fenêtre est en mode fenêtré
            this->WindowState = WINDOW_STATE_WINDOWED;
        }

        //Calcul de la surface de rendu graphique en fonction de la présence des bords
        AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

        // Création de la fenêtre
        this->hWnd = CreateWindowEx(dwExStyle,
                                    lpClassName,
                                    this->Title.c_str(),
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
    }

    /**
     * Destructeur
     */
    Window::~Window()
    {
        // Destruction de la fenêtre
        if(this->hWnd != NULL) DestroyWindow(this->hWnd);
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
    E_WINDOW_STATE Window::GetWindowState()
    {
        return this->WindowState;
    }

    /**
     * Centrage de la fenêtre
     */
    void Window::Center()
    {
        RECT rect;
        GetWindowRect(this->hWnd, &rect);
        SetWindowPos(this->hWnd, NULL, (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, 0, 0, SWP_NOSIZE);
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

    /**
     * Callback des messages Windows adressés aux fenêtres de l'application
     * Réception et traitement des messages Windows
     */
    LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            //La fenêtre est fermée par l'utilisateur
            case WM_CLOSE:
                PostQuitMessage(0);
                break;

            //La fenêtre est détruite
            case WM_DESTROY:
                return FALSE;

            //La fenêtre est redimensionnée
            case WM_SIZE:
            {
                //On retrouve l'objet Window qui a prévoqué l'évènement
                /*Window* Caller = WindowManager::WindowList[hwnd];

                if(Caller != NULL)
                {
                    //On parcours ses listeners pour les avertir du changement
                    std::vector<IWindowListener*> Listeners = Caller->GetListeners();
                    for(auto ListenerIterator = Listeners.begin(); ListenerIterator != Listeners.end(); ++ListenerIterator)
                    {
                        IWindowListener* Listener = *ListenerIterator;
                        E_WINDOW_STATE WindowState = Caller->GetWindowState();

                        //La fenêtre a été minimisée
                        if(wParam == SIZE_MINIMIZED && WindowState != WINDOW_STATE_MINIMIZED)
                        {
                            WindowState = WINDOW_STATE_MINIMIZED;
                            Listener->StateChanged(WINDOW_STATE_MINIMIZED);
                        //La fenêtre a été réstaurée ou maximisée
                        }else if((wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) && WindowState != WINDOW_STATE_WINDOWED){
                            WindowState = WINDOW_STATE_WINDOWED;
                            Listener->StateChanged(WINDOW_STATE_WINDOWED);
                        }
                        //On notifie les listeners de la nouvelle taille de la fenêtre
                        Types::Surface<uint32_t> NewSize = {LOWORD(lParam), HIWORD(lParam)};
                        Listener->SizeChanged(NewSize);
                    }
                }*/
                break;
            }

            //La touche VK_DOWN est enfoncée
            case WM_KEYDOWN:
                //Keyboard::GetInstance()->PressKey((unsigned char)wParam);
                switch(wParam)
                {
                    case VK_ESCAPE:
                        PostQuitMessage(0);
                        break;
                }
                break;

            //La touche VK_UP est enfoncée
            case WM_KEYUP:
                //Keyboard::GetInstance()->ReleaseKey((unsigned char)wParam);
                break;

            //Message non géré
            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        return FALSE;
    }
}
