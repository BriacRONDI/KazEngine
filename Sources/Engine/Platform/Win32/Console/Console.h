#pragma once

#include <fstream>
#include <cstdio>
#include <iostream>
#include <windows.h>
#include <cctype>

namespace Engine
{
    class Console
    {
        public:
            static void Open();
            static void Close();
            static void Pause();

        private:
            std::ofstream m_out;
            std::ofstream m_err;
            std::ifstream m_in;

            std::streambuf* m_old_cout;
            std::streambuf* m_old_cerr;
            std::streambuf* m_old_cin;

            static Console* instance;

            ~Console();
    };
}
