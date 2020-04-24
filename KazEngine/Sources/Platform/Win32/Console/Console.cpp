#include "Console.h"

namespace Engine
{
    // Singleton
    Console* Console::instance = nullptr;

    Console::Console()
    {
    }

    Console::~Console()
    {
        // reset the standard streams
        std::cin.rdbuf(this->m_old_cin);
        std::cerr.rdbuf(this->m_old_cerr);
        std::cout.rdbuf(this->m_old_cout);

        // remove the console window
        FreeConsole();
    }

    /**
     * Création de la console
     */
    void Console::Open()
    {
        // create a console window
        AllocConsole();
        SetConsoleTitle(L"KazEngine");

        // create instance
        if(Console::instance == nullptr) Console::instance = new Console;

        HWND hwnd = GetConsoleWindow();
        if (hwnd != NULL) {
            HMENU hMenu = GetSystemMenu(hwnd, FALSE);
            if(hMenu != NULL) DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
        }

        // redirect std::cout to our console window
        Console::instance->m_old_cout = std::cout.rdbuf();
        Console::instance->m_out.open("CONOUT$");
        std::cout.rdbuf(Console::instance->m_out.rdbuf());

        // redirect std::cerr to our console window
        Console::instance->m_old_cerr = std::cerr.rdbuf();
        Console::instance->m_err.open("CONOUT$");
        std::cerr.rdbuf(instance->m_err.rdbuf());

        // redirect std::cin to our console window
        Console::instance->m_old_cin = std::cin.rdbuf();
        Console::instance->m_in.open("CONIN$");
        std::cin.rdbuf(Console::instance->m_in.rdbuf());
    }

    /**
     * Destruction de la console
     */
    void Console::Close()
    {
        if(Console::instance != nullptr) {
            delete Console::instance;
            Console::instance = nullptr;
        }
    }

    /**
     * Affichage d'un message en attendant l'appui d'une touche
     */
    void Console::Pause()
    {
        std::cout << "Press any key to continue..." << std::endl;

        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

        DWORD mode;
        GetConsoleMode(h, &mode);
        SetConsoleMode(h, mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));

        TCHAR output = 0;
        DWORD length;
        ReadConsole(h, &output, 10, &length, NULL);
        SetConsoleMode(h, mode);
    }

    /**
     * Modifie la couleur du texte
     */
    void Console::SetTextColor(uint8_t color)
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }
}
