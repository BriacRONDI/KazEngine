#include "Console.h"

namespace Engine
{
    // Singleton
    Console* Console::instance = nullptr;

    Console::~Console()
    {
        // reset the standard streams
        std::cin.rdbuf(m_old_cin);
        std::cerr.rdbuf(m_old_cerr);
        std::cout.rdbuf(m_old_cout);

        // remove the console window
        FreeConsole();
    }

    void Console::Start()
    {
        // create a console window
        AllocConsole();

        // create instance
        if(Console::instance == nullptr) Console::instance = new Console;

        HWND hwnd = ::GetConsoleWindow();
        if (hwnd != NULL)
        {
            HMENU hMenu = ::GetSystemMenu(hwnd, FALSE);
            if (hMenu != NULL) DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
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

    void Console::Release()
    {
        if(Console::instance != nullptr) {
            delete Console::instance;
                Console::instance = nullptr;
        }
    }
}
