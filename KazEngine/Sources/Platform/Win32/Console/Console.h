#pragma once

#include <fstream>
#include <cstdio>
#include <iostream>
#include <windows.h>
#include <cctype>

#define TEXT_COLOR_ORANGE   0x06
#define TEXT_COLOR_NEUTRAL  0x07
#define TEXT_COLOR_BLUE     0x09
#define TEXT_COLOR_GREEN    0x0A
#define TEXT_COLOR_RED      0x0C
#define TEXT_COLOR_WHITE    0x0F

namespace Engine
{
    class Console
    {
        public:
            static void Open();
            static void Close();
            static void Pause();
            static inline void SetTextColor(uint8_t color = TEXT_COLOR_NEUTRAL);

        private:
            std::ofstream m_out;
            std::ofstream m_err;
            std::ifstream m_in;

            std::streambuf* m_old_cout;
            std::streambuf* m_old_cerr;
            std::streambuf* m_old_cin;

            static Console* instance;

            Console();
            ~Console();
    };
}
