#include "Maths.h"

namespace Engine
{
    Matrix4x4 PerspectiveProjectionMatrix(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip)
    {
        float f = 1.0f / std::tan(field_of_view * 0.5f * DEGREES_TO_RADIANS);

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

    Matrix4x4 TranslationMatrix(Vector3 const& vector)
    {
        return {
            1.0f,      0.0f,      0.0f,         0.0f,
            0.0f,      1.0f,      0.0f,         0.0f,
            0.0f,      0.0f,      1.0f,         0.0f,
            vector[0], vector[1], vector[2],    1.0f
        };
    }

    Matrix4x4 TranslationMatrix(Vector4 const& vector)
    {
        return {
            1.0f,      0.0f,      0.0f,         0.0f,
            0.0f,      1.0f,      0.0f,         0.0f,
            0.0f,      0.0f,      1.0f,         0.0f,
            vector[0], vector[1], vector[2],    1.0f
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

    Vector4 operator*(Matrix4x4 const& matrix, Vector4 const& vertex)
    {
        return {
            matrix[0] *  vertex[0] + matrix[1] *  vertex[1] + matrix[2] *  vertex[2] + matrix[3] *  vertex[3],
            matrix[4] *  vertex[0] + matrix[5] *  vertex[1] + matrix[6] *  vertex[2] + matrix[7] *  vertex[3],
            matrix[8] *  vertex[0] + matrix[9] *  vertex[1] + matrix[10] * vertex[2] + matrix[11] * vertex[3],
            matrix[12] * vertex[0] + matrix[13] * vertex[1] + matrix[14] * vertex[2] + matrix[15] * vertex[3],
        };
    }

    Vector4 MatrixMultT(Matrix4x4 const& matrix, Vector4 const& vertex)
    {
        return {
            matrix[0] * vertex[0] + matrix[4] * vertex[1] + matrix[8] * vertex[2] + matrix[12],
            matrix[1] * vertex[0] + matrix[5] * vertex[1] + matrix[9] * vertex[2] + matrix[13],
            matrix[2] * vertex[0] + matrix[6] * vertex[1] + matrix[10] * vertex[2] + matrix[14],
            vertex[3]
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

    Matrix4x4 RotationMatrix(float angle, Vector3 const & axis, bool normalize_axis)
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

    Matrix4x4 EulerRotation(Matrix4x4 const& matrix, Vector3 const& vector, EULER_ORDER order)
    {
        Matrix4x4 result = matrix;

        float x = vector[0], y = vector[1], z = vector[2];
		float a = std::cos( x ), b = std::sin( x );
		float c = std::cos( y ), d = std::sin( y );
		float e = std::cos( z ), f = std::sin( z );

		if (order == EULER_ORDER::XYZ) {

			float ae = a * e, af = a * f, be = b * e, bf = b * f;

			result[ 0 ] = c * e;
			result[ 4 ] = - c * f;
			result[ 8 ] = d;

			result[ 1 ] = af + be * d;
			result[ 5 ] = ae - bf * d;
			result[ 9 ] = - b * c;

			result[ 2 ] = bf - ae * d;
			result[ 6 ] = be + af * d;
			result[ 10 ] = a * c;

		} else if ( order == EULER_ORDER::YXZ ) {

			float ce = c * e, cf = c * f, de = d * e, df = d * f;

			result[ 0 ] = ce + df * b;
			result[ 4 ] = de * b - cf;
			result[ 8 ] = a * d;

			result[ 1 ] = a * f;
			result[ 5 ] = a * e;
			result[ 9 ] = - b;

			result[ 2 ] = cf * b - de;
			result[ 6 ] = df + ce * b;
			result[ 10 ] = a * c;

		} else if ( order == EULER_ORDER::ZXY ) {

			float ce = c * e, cf = c * f, de = d * e, df = d * f;

			result[ 0 ] = ce - df * b;
			result[ 4 ] = - a * f;
			result[ 8 ] = de + cf * b;

			result[ 1 ] = cf + de * b;
			result[ 5 ] = a * e;
			result[ 9 ] = df - ce * b;

			result[ 2 ] = - a * d;
			result[ 6 ] = b;
			result[ 10 ] = a * c;

		} else if ( order == EULER_ORDER::ZYX ) {

			float ae = a * e, af = a * f, be = b * e, bf = b * f;

			result[ 0 ] = c * e;
			result[ 4 ] = be * d - af;
			result[ 8 ] = ae * d + bf;

			result[ 1 ] = c * f;
			result[ 5 ] = bf * d + ae;
			result[ 9 ] = af * d - be;

			result[ 2 ] = - d;
			result[ 6 ] = b * c;
			result[ 10 ] = a * c;

		} else if ( order == EULER_ORDER::YZX ) {

			float ac = a * c, ad = a * d, bc = b * c, bd = b * d;

			result[ 0 ] = c * e;
			result[ 4 ] = bd - ac * f;
			result[ 8 ] = bc * f + ad;

			result[ 1 ] = f;
			result[ 5 ] = a * e;
			result[ 9 ] = - b * e;

			result[ 2 ] = - d * e;
			result[ 6 ] = ad * f + bc;
			result[ 10 ] = ac - bd * f;

		} else if ( order == EULER_ORDER::XZY ) {

			float ac = a * c, ad = a * d, bc = b * c, bd = b * d;

			result[ 0 ] = c * e;
			result[ 4 ] = - f;
			result[ 8 ] = d * e;

			result[ 1 ] = ac * f + bd;
			result[ 5 ] = a * e;
			result[ 9 ] = ad * f - bc;

			result[ 2 ] = bc * f - ad;
			result[ 6 ] = b * e;
			result[ 10 ] = bd * f + ac;

		}

		// bottom row
		result[ 3 ] = 0;
		result[ 7 ] = 0;
		result[ 11 ] = 0;

		// last column
		result[ 12 ] = 0;
		result[ 13 ] = 0;
		result[ 14 ] = 0;
		result[ 15 ] = 1;

		return result;
    }

    float Length(Vector3 const& vector)
    {
        return std::sqrt(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
    }

    float Distance(Vector2 const& u, Vector2 const& v)
    {
        float segment_x = u[0] - v[0];
        float segment_y = u[1] - v[1];
        return std::sqrt(segment_x * segment_x + segment_y * segment_y);
    }

    Matrix4x4 ExtractRotationMatrix(Matrix4x4 const& matrix)
    {
        float scale_x = 1.0f / Length({matrix[0], matrix[4], matrix[8]});
        float scale_y = 1.0f / Length({matrix[1], matrix[5], matrix[9]});
        float scale_z = 1.0f / Length({matrix[2], matrix[6], matrix[10]});

        return {
            matrix[0] * scale_x,    matrix[1] * scale_y,    matrix[2] * scale_z,    0,
            matrix[4] * scale_x,    matrix[5] * scale_y,    matrix[6] * scale_z,    0,
            matrix[8] * scale_x,    matrix[9] * scale_y,   matrix[10] * scale_z,    0,
                              0,                      0,                      0,    1
        };
    }

    Matrix4x4 ExtractTranslationMatrix(Matrix4x4 const& matrix)
    {
        return {
            1.0f,         0.0f,         0.0f,         0.0f,
            0.0f,         1.0f,         0.0f,         0.0f,
            0.0f,         0.0f,         1.0f,         0.0f,
            matrix[12],   matrix[13],   matrix[14],   1.0f
        };
    }

    Matrix4x4 InverseMatrix(Matrix4x4 const& matrix)
    {
        Matrix4x4 result;

		float n11 = matrix[ 0 ],  n21 = matrix[ 1 ],  n31 = matrix[ 2 ],  n41 = matrix[ 3 ],
		      n12 = matrix[ 4 ],  n22 = matrix[ 5 ],  n32 = matrix[ 6 ],  n42 = matrix[ 7 ],
		      n13 = matrix[ 8 ],  n23 = matrix[ 9 ],  n33 = matrix[ 10 ], n43 = matrix[ 11 ],
		      n14 = matrix[ 12 ], n24 = matrix[ 13 ], n34 = matrix[ 14 ], n44 = matrix[ 15 ],

		      t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44,
		      t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44,
		      t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44,
		      t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34,

		      det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
        
        // Déterminant = 0 => invesion impossible
		if(det == 0) return IDENTITY_MATRIX;

		float detInv = 1.0f / det;

		result[ 0 ] = t11 * detInv;
		result[ 1 ] = ( n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44 ) * detInv;
		result[ 2 ] = ( n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44 ) * detInv;
		result[ 3 ] = ( n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43 ) * detInv;

		result[ 4 ] = t12 * detInv;
		result[ 5 ] = ( n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44 ) * detInv;
		result[ 6 ] = ( n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44 ) * detInv;
		result[ 7 ] = ( n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43 ) * detInv;

		result[ 8 ] = t13 * detInv;
		result[ 9 ] = ( n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44 ) * detInv;
		result[ 10 ] = ( n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44 ) * detInv;
		result[ 11 ] = ( n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43 ) * detInv;

		result[ 12 ] = t14 * detInv;
		result[ 13 ] = ( n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34 ) * detInv;
		result[ 14 ] = ( n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34 ) * detInv;
		result[ 15 ] = ( n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33 ) * detInv;

		return result;
    }

    void CopyTranslation(Matrix4x4 const& source, Matrix4x4& destination)
    {
        destination[12] = source[12];
        destination[13] = source[13];
        destination[14] = source[14];
    }

    Vector3 FromDoubleVector(Vector3d vector)
    {
        return {
            static_cast<float>(vector[0]),
            static_cast<float>(vector[1]),
            static_cast<float>(vector[2])
        };
    }

    Matrix4x4 MatrixAdd(Matrix4x4 const& A, Matrix4x4 const& B)
    {
        Matrix4x4 result;
        for(uint8_t i=0; i<16; i++)
            result[i] = A[i] + B[i];
        return result;
    }

    Matrix4x4 operator*(Matrix4x4 const& matrix, float const& scalar)
    {
        Matrix4x4 result;
        for(uint8_t i=0; i<16; i++)
            result[i] = matrix[i] * scalar;
        return result;
    }

    Vector3 Interpolate(Vector3 const& source, Vector3 const& dest, float ratio)
    {
        Vector3 result;

        for(uint8_t i=0; i<3; i++) {
            if(source[i] <= dest[i]) result[i] = source[i] + std::abs(dest[i] - source[i]) * ratio;
            else result[i] = source[i] - std::abs(dest[i] - source[i]) * ratio;
        }

        return result;
    }

    Matrix4x4 ScalingMatrix(Vector3 const& vector)
    {
        return {
            vector[0],  0.0f,       0.0f,       0.0f,
            0.0f,       vector[1],  0.0f,       0.0f,
            0.0f,       0.0f,       vector[2],  0.0f,
            0.0f,       0.0f,       0.0f,       1.0f
        };
    }

    Vector3 operator*(Vector3 const& vector, float const& scalar)
    {
        return {
            vector[0] * scalar,
            vector[1] * scalar,
            vector[2] * scalar
        };
    }

    Vector3 CrossProduct(Vector3 const& u, Vector3 const& v)
    {
        return {
            u[1] * v[2] - u[2] * v[1],
            u[2] * v[0] - u[0] * v[2],
            u[0] * v[1] - u[1] * v[0]
        };
    }

    Vector3 ComputeNormal(std::array<Vector3, 3> triangle)
    {
        return {};
    }

    float Dot(Vector3 const& u, Vector3 const& v)
    {
        return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
    }

    Vector3 operator*(Vector3 const& vector, Matrix4x4 const& matrix)
    {
        return {
            vector[0] * matrix[0] + vector[1] * matrix[1] + vector[2] * matrix[2],
            vector[0] * matrix[4] + vector[1] * matrix[5] + vector[2] * matrix[6],
            vector[0] * matrix[8] + vector[1] * matrix[9] + vector[2] * matrix[10]
        };
    }

    Vector3 operator+(Vector3 const& u, Vector3 const& v)
    {
        return {
            u[0] + v[0],
            u[1] + v[1],
            u[2] + v[2]
        };
    }

    Vector3 operator-(Vector3 const& u, Vector3 const& v)
    {
        return {
            u[0] - v[0],
            u[1] - v[1],
            u[2] - v[2]
        };
    }
}