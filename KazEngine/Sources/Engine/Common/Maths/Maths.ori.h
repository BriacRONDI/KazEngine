#pragma once

#include <array>
#include <cmath>

#define DEGREES_TO_RADIANS 0.01745329251994329576923690768489f
#define IDENTITY_MATRIX {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}
#define ZERO_MATRIX {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define VECTOR_COMPONENT_X 0
#define VECTOR_COMPONENT_Y 1
#define VECTOR_COMPONENT_Z 2

namespace Engine
{
    enum EULER_ORDER {ZYX, YZX, XZY, ZXY, YXZ, XYZ};

    using Matrix4x4 = std::array<float, 16>;
    using Matrix1x4 = std::array<float, 4>;
    using Vector2 = std::array<float, 3>;
    using Vector3 = std::array<float, 3>;
    using Vector3d = std::array<double, 3>;
    using Vector4 = std::array<float, 4>;
    using TexCood = std::array<float, 2>;
    using ColorRGB = std::array<uint8_t, 3>;

    Matrix4x4 PerspectiveProjectionMatrix(
        float const aspect_ratio,
        float const field_of_view,
        float const near_clip,
        float const far_clip
    );

    Matrix4x4 TranslationMatrix(float x, float y, float z);
    Matrix4x4 TranslationMatrix(Vector3 const& vector);
    Matrix4x4 TranslationMatrix(Vector4 const& vector);
    Matrix4x4 RotationMatrix(float angle, Vector3 const& axis, bool normalize_axis = false);
    Matrix4x4 ScalingMatrix(float x, float y, float z);
    Matrix4x4 ScalingMatrix(Vector3 const& vector);
    Matrix4x4 operator*(Matrix4x4 const& left, Matrix4x4 const& right);
    Matrix4x4 EulerRotation(Matrix4x4 const& matrix, Vector3 const& vector, EULER_ORDER order);
    Matrix4x4 ExtractRotationMatrix(Matrix4x4 const& matrix);
    Matrix4x4 ExtractTranslationMatrix(Matrix4x4 const& matrix);
    Matrix4x4 InverseMatrix(Matrix4x4 const& matrix);
    Matrix4x4 operator*(Matrix4x4 const& matrix, float const& scalar);
    Matrix4x4 MatrixAdd(Matrix4x4 const& A, Matrix4x4 const& B);
    Vector4 operator*(Matrix4x4 const& transform, Vector4 const& vertex);
    Vector3 operator*(Vector3 const& vector, float const& scalar);
    Vector3 operator*(Vector3 const& vector, Matrix4x4 const& matrix);
    Vector3 operator+(Vector3 const& u, Vector3 const& v);
    Vector3 operator-(Vector3 const& u, Vector3 const& v);
    Vector4 MatrixMultT(Matrix4x4 const& matrix, Vector4 const& vertex);
    Vector3 FromDoubleVector(Vector3d vector);
    Vector3 Normalize(Vector3 const& vector);
    Vector3 Interpolate(Vector3 const& source, Vector3 const& dest, float ratio);
    Vector3 CrossProduct(Vector3 const& u, Vector3 const& v);
    Vector3 ComputeNormal(std::array<Vector3, 3> triangle);
    float Dot(Vector3 const& u, Vector3 const& v);
    float Length(Vector3 const& vector);
    float Distance(Vector2 const& u, Vector2 const& v);
    void CopyTranslation(Matrix4x4 const& source, Matrix4x4& destination);
    

    //using Engine::operator*;
}