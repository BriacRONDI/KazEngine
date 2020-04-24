#pragma once

#include <fstream>
#include <cstdio>
#include <iostream>
#include <cctype>

#if defined(_WIN32)
#include <windows.h>
#define TERMINAL_COLOR_ORANGE   0x06
#define TERMINAL_COLOR_NEUTRAL  0x07
#define TERMINAL_COLOR_BLUE     0x09
#define TERMINAL_COLOR_GREEN    0x0A
#define TERMINAL_COLOR_RED      0x0C
#define TERMINAL_COLOR_WHITE    0x0F
#endif

namespace Log
{
    class Terminal
    {
        public:

            enum TEXT_COLOR : uint8_t {
                NEUTRAL     = TERMINAL_COLOR_NEUTRAL,
                ORANGE      = TERMINAL_COLOR_ORANGE,
                BLUE        = TERMINAL_COLOR_BLUE,
                GREEN       = TERMINAL_COLOR_GREEN,
                RED         = TERMINAL_COLOR_RED,
                WHITE       = TERMINAL_COLOR_WHITE
            };

            /// Open terminal window
            static void Open(bool reopen = false);

            /// Close terminal window
            static void Close();

            /// Ask user to press any key
            static void Pause();

            /// Change text color
            static void SetTextColor(TEXT_COLOR color = TEXT_COLOR::NEUTRAL);

        private:
            std::ofstream m_out;
            std::ofstream m_err;
            std::ifstream m_in;

            std::streambuf* m_old_cout;
            std::streambuf* m_old_cerr;
            std::streambuf* m_old_cin;

            /// Singleton instance
            static Terminal* instance;

            Terminal();
            ~Terminal();
    };
}
