#include "../Terminal.h"

namespace Log
{
    // Singleton
    Terminal* Terminal::instance = nullptr;

    Terminal::~Terminal()
    {
        // reset the standard streams
        std::cin.rdbuf(this->m_old_cin);
        std::cerr.rdbuf(this->m_old_cerr);
        std::cout.rdbuf(this->m_old_cout);

        // remove the console window
        FreeConsole();
    }

    void Terminal::Open(bool reopen)
    {
        // If console window il already opened, close it
        if(Terminal::instance != nullptr) {
            if(reopen) Terminal::Close();
            else return;
        }

        // create instance
        Terminal::instance = new Terminal;

        // create a console window
        AllocConsole();
        SetConsoleTitle(L"Terminal");

        HWND hwnd = GetConsoleWindow();
        if (hwnd != NULL) {
            HMENU hMenu = GetSystemMenu(hwnd, FALSE);
            if(hMenu != NULL) DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
        }

        // redirect std::cout to our console window
        Terminal::instance->m_old_cout = std::cout.rdbuf();
        Terminal::instance->m_out.open("CONOUT$");
        std::cout.rdbuf(Terminal::instance->m_out.rdbuf());

        // redirect std::cerr to our console window
        Terminal::instance->m_old_cerr = std::cerr.rdbuf();
        Terminal::instance->m_err.open("CONOUT$");
        std::cerr.rdbuf(instance->m_err.rdbuf());

        // redirect std::cin to our console window
        Terminal::instance->m_old_cin = std::cin.rdbuf();
        Terminal::instance->m_in.open("CONIN$");
        std::cin.rdbuf(Terminal::instance->m_in.rdbuf());
    }

    void Terminal::Close()
    {
        if(Terminal::instance != nullptr) {
            delete Terminal::instance;
            Terminal::instance = nullptr;
        }
    }

    void Terminal::Pause()
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

    void Terminal::SetTextColor(TEXT_COLOR color)
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }
}
