#pragma once

#include <array>

#define IDENTITY_MATRIX {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}

namespace Engine
{
    using Matrix4x4 = std::array<float, 16>;
    using Matrix1x4 = std::array<float, 4>;
    using Vector3 = std::array<float, 3>;
    using Vector4 = std::array<float, 4>;
    using TexCood = std::array<float, 2>;

    Matrix4x4 OrthographicProjectionMatrix(
        float const left_plane,
        float const right_plane,
        float const top_plane,
        float const bottom_plane,
        float const near_plane,
        float const far_plane
    );

    Matrix4x4 PerspectiveProjectionMatrix(
        float const aspect_ratio,
        float const field_of_view,
        float const near_clip,
        float const far_clip
    );

    Matrix4x4 TranslationMatrix(float x, float y, float z);
    Matrix4x4 RotationMatrix(float angle, Vector3 const & axis, float normalize_axis = false);
    Matrix4x4 operator*(Matrix4x4 const & left, Matrix4x4 const & right);
    Vector3 Normalize(Vector3 const & vector);

    //using Engine::operator*;
}