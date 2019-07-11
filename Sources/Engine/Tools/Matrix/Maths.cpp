#include "Maths.h"

namespace Engine
{
    Matrix4x4 OrthographicProjectionMatrix(float const left_plane, float const right_plane, float const top_plane, float const bottom_plane, float const near_plane, float const far_plane)
    {
        return {
            2.0f / (right_plane - left_plane),
            0.0f,
            0.0f,
            0.0f,

            0.0f,
            2.0f / (bottom_plane - top_plane),
            0.0f,
            0.0f,

            0.0f,
            0.0f,
            1.0f / (near_plane - far_plane),
            0.0f,

            -(right_plane + left_plane) / (right_plane - left_plane),
            -(bottom_plane + top_plane) / (bottom_plane - top_plane),
            near_plane / (near_plane - far_plane),
            1.0f
        };
    }

    Matrix4x4 PerspectiveProjectionMatrix(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip)
    {
        float f = 1.0f / std::tan(field_of_view * 0.5f * 0.01745329251994329576923690768489f);

        return {
            f / aspect_ratio,
            0.0f,
            0.0f,
            0.0f,

            0.0f,
            f,
            0.0f,
            0.0f,

            0.0f,
            0.0f,
            far_clip / (near_clip - far_clip),
            -1.0f,

            0.0f,
            0.0f,
            (near_clip * far_clip) / (near_clip - far_clip),
            0.0f
        };
    }

    Matrix4x4 TranslationMatrix(float x, float y, float z)
    {
        return {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            x,    y,    z,    1.0f
        };
    }

    Matrix4x4 operator*(Matrix4x4 const & left, Matrix4x4 const & right)
    {
        return {
            left[0] * right[0] + left[4] * right[1] + left[8] * right[2] + left[12] * right[3],
            left[1] * right[0] + left[5] * right[1] + left[9] * right[2] + left[13] * right[3],
            left[2] * right[0] + left[6] * right[1] + left[10] * right[2] + left[14] * right[3],
            left[3] * right[0] + left[7] * right[1] + left[11] * right[2] + left[15] * right[3],

            left[0] * right[4] + left[4] * right[5] + left[8] * right[6] + left[12] * right[7],
            left[1] * right[4] + left[5] * right[5] + left[9] * right[6] + left[13] * right[7],
            left[2] * right[4] + left[6] * right[5] + left[10] * right[6] + left[14] * right[7],
            left[3] * right[4] + left[7] * right[5] + left[11] * right[6] + left[15] * right[7],

            left[0] * right[8] + left[4] * right[9] + left[8] * right[10] + left[12] * right[11],
            left[1] * right[8] + left[5] * right[9] + left[9] * right[10] + left[13] * right[11],
            left[2] * right[8] + left[6] * right[9] + left[10] * right[10] + left[14] * right[11],
            left[3] * right[8] + left[7] * right[9] + left[11] * right[10] + left[15] * right[11],

            left[0] * right[12] + left[4] * right[13] + left[8] * right[14] + left[12] * right[15],
            left[1] * right[12] + left[5] * right[13] + left[9] * right[14] + left[13] * right[15],
            left[2] * right[12] + left[6] * right[13] + left[10] * right[14] + left[14] * right[15],
            left[3] * right[12] + left[7] * right[13] + left[11] * right[14] + left[15] * right[15]
        };
    }

    Vector3 Normalize(Vector3 const & vector)
    {
        float length = std::sqrt( vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2] );
        return {
            vector[0] / length,
            vector[1] / length,
            vector[2] / length
        };
}

    Matrix4x4 RotationMatrix(float angle, Vector3 const & axis, float normalize_axis)
    {
        float rad_angle = angle * 0.01745329251994329576923690768489f;

        float x;
        float y;
        float z;

        if(normalize_axis) {
            Vector3 normalized = Normalize( axis );
            x = normalized[0];
            y = normalized[1];
            z = normalized[2];
        }else{
            x = axis[0];
            y = axis[1];
            z = axis[2];
        }

        const float c = cos(rad_angle);
        const float _1_c = 1.0f - c;
        const float s = sin(rad_angle);

        return {
            x * x * _1_c + c,
            y * x * _1_c - z * s,
            z * x * _1_c + y * s,
            0.0f,

            x * y * _1_c + z * s,
            y * y * _1_c + c,
            z * y * _1_c - x * s,
            0.0f,

            x * z * _1_c - y * s,
            y * z * _1_c + x * s,
            z * z * _1_c + c,
            0.0f,

            0.0f,
            0.0f,
            0.0f,
            1.0f
        };
    }

    Matrix4x4 ScalingMatrix(float x, float y, float z)
    {
        return {
            x,     0.0f,   0.0f,   0.0f,
            0.0f,     y,   0.0f,   0.0f,
            0.0f,  0.0f,      z,   0.0f,
            0.0f,  0.0f,   0.0f,   1.0f
        };
    }
}