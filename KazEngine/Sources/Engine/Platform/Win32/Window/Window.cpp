#include "../../Common/Window/Window.h"
#include <strsafe.h>

namespace Engine
{
    // Instanciation de la liste des fen�tres
    std::map<HWND, Window*> Window::WindowList;

    Window::Window(Area<uint32_t> window_size, std::string title, FULL_SCREEN_MODE full_screen)
    {
        // Initialisation
        this->Title = title;
        this->hWnd = NULL;

        //Cr�ation de la fen�tre
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

        // �chec lors de l'enregistrement de la classe, on s'arr�te
        LPCSTR lpClassName = (LPCTSTR)(uintptr_t)RegisterClassEx(&wcex);
        if(!lpClassName) return;

        //Initialisation
        DWORD dwExStyle;
        DWORD dwStyle;

        //Calcule de la taille r�elle de la fen�tre
        RECT WindowRect;
        WindowRect.left = 0;
        WindowRect.right = window_size.Width;
        WindowRect.top = 0;
        WindowRect.bottom = window_size.Height;

        if(full_screen == Window::FULLSCREEN_MODE_ENABLED)
        {
            //Mode plein �cran, changement de la r�solution de l'�cran si n�cessaire
            DEVMODE dmScreenSettings;
            memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
            dmScreenSettings.dmSize = sizeof(dmScreenSettings);
            dmScreenSettings.dmPelsWidth	= window_size.Width;
            dmScreenSettings.dmPelsHeight	= window_size.Height;
            dmScreenSettings.dmBitsPerPel	= 32;
            dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

            //On indique que la fen�tre est sans bords
            dwExStyle = WS_EX_APPWINDOW;
            dwStyle = WS_POPUP;

            //On cache la souris
            ShowCursor(FALSE);

            // La fen�tre est en mode plein �cran
            this->WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_FULLSCREEN;

        }else{ //Mode fen�tr�

            //Style de la fen�te (avec ou sans bords, taille fixe ou non etc...)
            dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
            // dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            dwStyle = WS_OVERLAPPEDWINDOW;

            // La fen�tre est en mode fen�tr�
            this->WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED;
        }

        //Calcul de la surface de rendu graphique en fonction de la pr�sence des bords
        AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

        // Cr�ation de la fen�tre
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

        //Centrage et affichage de la fen�tre
        this->Center();
        this->Show();

        // On ajoute la fen�tre � la liste
        Window::WindowList[this->hWnd] = this;
    }

    /**
     * Destructeur
     */
    Window::~Window()
    {
        // Destruction de la fen�tre
        if(this->hWnd != NULL) DestroyWindow(this->hWnd);

        // Retrait de la liste
        Window::WindowList.erase(this->hWnd);
    }

    /**
     * R�cup�re une surface de pr�sentation pour Vulkan, ind�pendante de l'OS
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
     * Centrage de la fen�tre
     */
    void Window::Center()
    {
        RECT rect;
        GetWindowRect(this->hWnd, &rect);
        SetWindowPos(this->hWnd, NULL, (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, 0, 0, SWP_NOSIZE);
    }

    /**
     * Dissimulation de la fen�tre
     */
    void Window::Hide()
    {
        ShowWindow(this->hWnd, SW_HIDE);
    }

    /**
     * Affichage de la fen�tre
     */
    void Window::Show()
    {
        ShowWindow(this->hWnd, SW_SHOW);
    }

    /**
     * Change le titre de la fen�tre
     */
    void Window::SetTitle(std::string const& title)
    {
        SetWindowText(this->hWnd, title.data());
    }

    /**
     * Boucle principale permettant le traitement des messages envoy�s par le syst�me d'exploitation aux fen�tres
     * Si la fonction r�pond false, c'est que l'application a re�u le signal d'arr�t
     */
    bool Window::Loop()
    {
        MSG msg;

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                // Si le message WM_QUIT est re�u, on signale la fermeture de l'application
                return false;
            }else{
                //Gestion des messages de fen�tre
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // La boucle peut se poursuivre
        return true;
    }

    /**
     * Callback des messages Windows adress�s aux fen�tres de l'application
     * R�ception et traitement des messages Windows
     */
    LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            //La fen�tre est ferm�e par l'utilisateur
            case WM_CLOSE :
                PostQuitMessage(0);
                break;

            //La fen�tre est d�truite
            case WM_DESTROY :
                return FALSE;

            //La fen�tre est est cours de redimensionnement
            case WM_SIZING :
            {
                // On retrouve la fen�tre appelante
                Window* self = Window::WindowList[hwnd];

                // La fen�tre dispose d'un ratio forc�
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
                break;
            }

            //La fen�tre est redimensionn�e
            case WM_SIZE :
            {
                //On retrouve l'objet Window qui a pr�voqu� l'�v�nement
                Window* self = Window::WindowList[hwnd];

                if(self != NULL)
                {
                    //On parcours ses listeners pour les avertir du changement
                    std::vector<IWindowListener*> Listeners = self->GetListeners();
                    for(auto ListenerIterator = Listeners.begin(); ListenerIterator != Listeners.end(); ++ListenerIterator)
                    {
                        IWindowListener* Listener = *ListenerIterator;
                        IWindowListener::E_WINDOW_STATE WindowState = self->GetWindowState();

                        // La fen�tre a �t� minimis�e
                        if(wParam == SIZE_MINIMIZED && WindowState != IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MINIMIZED)
                        {
                            WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MINIMIZED;
                            Listener->StateChanged(IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MINIMIZED);
                        // La fen�tre a �t� r�staur�e
                        }else if((wParam == SIZE_RESTORED) && WindowState != IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED){
                            WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED;
                            Listener->StateChanged(IWindowListener::E_WINDOW_STATE::WINDOW_STATE_WINDOWED);
                        // La fen�tre a �t� maximis�e
                        }else if((wParam == SIZE_MAXIMIZED) && WindowState != IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MAXIMIZED){
                            WindowState = IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MAXIMIZED;
                            Listener->StateChanged(IWindowListener::E_WINDOW_STATE::WINDOW_STATE_MAXIMIZED);
                        }
                        //On notifie les listeners de la nouvelle taille de la fen�tre
                        Area<uint32_t> NewSize = {LOWORD(lParam), HIWORD(lParam)};
                        Listener->SizeChanged(NewSize);
                    }
                }
                break;
            }

            //La touche VK_DOWN est enfonc�e
            case WM_KEYDOWN :
                Keyboard::GetInstance()->PressKey(static_cast<unsigned char>(wParam));
                switch(wParam)
                {
                    case VK_ESCAPE:
                        PostQuitMessage(0);
                        break;
                }
                break;

            //La touche VK_UP est enfonc�e
            case WM_KEYUP :
                Keyboard::GetInstance()->ReleaseKey(static_cast<unsigned char>(wParam));
                break;

            // La souris de s�place sur la fen�tre
            case WM_MOUSEMOVE :
                Mouse::GetInstance().MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                break;

            // La molette est utilis�e
            case WM_MOUSEWHEEL :
                if(GET_WHEEL_DELTA_WPARAM(wParam) > 0) Mouse::GetInstance().MouseScrollUp();
                else Mouse::GetInstance().MouseScrollDown();
                break;

            // Bouton gauche de la souris enfonc�
            case WM_LBUTTONDOWN :
                Mouse::GetInstance().MouseButtonDown(IMouseListener::MOUSE_BUTTON_LEFT);
                break;

            // Bouton droit de la souris enfonc�
            case WM_RBUTTONDOWN :
                Mouse::GetInstance().MouseButtonDown(IMouseListener::MOUSE_BUTTON_RIGHT);
                break;

            // Bouton central de la souris enfonc�
            case WM_MBUTTONDOWN :
                Mouse::GetInstance().MouseButtonDown(IMouseListener::MOUSE_BUTTON_MIDDLE);
                break;

            // Bouton gauche de la souris rel�ch�
            case WM_LBUTTONUP :
                Mouse::GetInstance().MouseButtonUp(IMouseListener::MOUSE_BUTTON_LEFT);
                break;

            // Bouton droit de la souris rel�ch�
            case WM_RBUTTONUP :
                Mouse::GetInstance().MouseButtonUp(IMouseListener::MOUSE_BUTTON_RIGHT);
                break;

            // Bouton central de la souris rel�ch�
            case WM_MBUTTONUP :
                Mouse::GetInstance().MouseButtonUp(IMouseListener::MOUSE_BUTTON_MIDDLE);
                break;

            //Message non g�r�
            default :
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        return FALSE;
    }
}
