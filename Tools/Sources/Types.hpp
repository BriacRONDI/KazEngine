#pragma once

namespace Engine
{
    template <typename T>
    struct Area {
        T Width;
        T Height;
    };

    template <typename T>
    struct Point {
        T X;
        T Y;
    };

    template <typename T>
    struct Rect {
        T X;
        T Y;
        T Width;
        T Height;
    };
}
