#pragma once

#include <fstream>
#include <cstdio>
#include <iostream>
#include <windows.h>

namespace Engine
{
    class Console
    {
        public:
            static void Start();
            static void Release();

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
