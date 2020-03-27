#include "Matrix.h"

namespace Engine
{
    Matrix4x4::Matrix4x4() : value(IDENTITY_MATRIX) {}
    Matrix4x4::Matrix4x4(Matrix4x4 const& other) : value(other.value) {}
    Matrix4x4::Matrix4x4(std::array<float,16> const& other) : value(other) {}
    Matrix4x4::Matrix4x4(Matrix4x4&& other) {*this = std::move(other);}
    Matrix4x4::Matrix4x4(std::array<float,16>&& other) : value(other) {}

    Matrix4x4::Matrix4x4(float x1, float y1, float z1, float w1,
                         float x2, float y2, float z2, float w2,
                         float x3, float y3, float z3, float w3,
                         float x4, float y4, float z4, float w4)
    {
        this->value = {
            x1, y1, z1, w1,
            x2, y2, z2, w2,
            x3, y3, z3, w3,
            x4, y4, z4, w4
        };
    }

    Matrix4x4 Matrix4x4::operator*(Matrix4x4 const& other) const
    {
        return {
            this->value[0] * other[0] + this->value[4] * other[1] + this->value[8] * other[2] + this->value[12] * other[3],
            this->value[1] * other[0] + this->value[5] * other[1] + this->value[9] * other[2] + this->value[13] * other[3],
            this->value[2] * other[0] + this->value[6] * other[1] + this->value[10] * other[2] + this->value[14] * other[3],
            this->value[3] * other[0] + this->value[7] * other[1] + this->value[11] * other[2] + this->value[15] * other[3],

            this->value[0] * other[4] + this->value[4] * other[5] + this->value[8] * other[6] + this->value[12] * other[7],
            this->value[1] * other[4] + this->value[5] * other[5] + this->value[9] * other[6] + this->value[13] * other[7],
            this->value[2] * other[4] + this->value[6] * other[5] + this->value[10] * other[6] + this->value[14] * other[7],
            this->value[3] * other[4] + this->value[7] * other[5] + this->value[11] * other[6] + this->value[15] * other[7],

            this->value[0] * other[8] + this->value[4] * other[9] + this->value[8] * other[10] + this->value[12] * other[11],
            this->value[1] * other[8] + this->value[5] * other[9] + this->value[9] * other[10] + this->value[13] * other[11],
            this->value[2] * other[8] + this->value[6] * other[9] + this->value[10] * other[10] + this->value[14] * other[11],
            this->value[3] * other[8] + this->value[7] * other[9] + this->value[11] * other[10] + this->value[15] * other[11],

            this->value[0] * other[12] + this->value[4] * other[13] + this->value[8] * other[14] + this->value[12] * other[15],
            this->value[1] * other[12] + this->value[5] * other[13] + this->value[9] * other[14] + this->value[13] * other[15],
            this->value[2] * other[12] + this->value[6] * other[13] + this->value[10] * other[14] + this->value[14] * other[15],
            this->value[3] * other[12] + this->value[7] * other[13] + this->value[11] * other[14] + this->value[15] * other[15]
        };
    }

    Vector4 Matrix4x4::operator*(Vector4 const& vertex) const
    {
        return {
            this->value[0] * vertex[0] + this->value[4] * vertex[1] + this->value[8]  * vertex[2] + this->value[12] * vertex[3],
            this->value[1] * vertex[0] + this->value[5] * vertex[1] + this->value[9]  * vertex[2] + this->value[13] * vertex[3],
            this->value[2] * vertex[0] + this->value[6] * vertex[1] + this->value[10] * vertex[2] + this->value[14] * vertex[3],
            this->value[3] * vertex[0] + this->value[7] * vertex[1] + this->value[11] * vertex[2] + this->value[15] * vertex[3]
        };
    }

    /**
     * Création d'une matrice de projection en perspective
     */
    Matrix4x4 Matrix4x4::PerspectiveProjectionMatrix(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip)
    {
        float f = 1.0f / std::tan(field_of_view * 0.5f * DEGREES_TO_RADIANS);

        return {
            f / aspect_ratio,   0.0f,       0.0f,                                                   0.0f,
            0.0f,               f,          0.0f,                                                   0.0f,
            0.0f,               0.0f,       (far_clip + near_clip) / (near_clip - far_clip),        -1.0f,
            0.0f,               0.0f,       (near_clip * far_clip) / (near_clip - far_clip),        0.0f
        };
    }

    /**
     * Création d'une matrice de projection orthogonale
     */
    Matrix4x4 Matrix4x4::OrthographicProjectionMatrix(float const left_plane, float const right_plane, float const top_plane,
                                                      float const bottom_plane, float const near_plane, float const far_plane)
    {
        return {
            2.0f / (right_plane - left_plane), 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / (bottom_plane - top_plane), 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f / (near_plane - far_plane), 0.0f,
            -(right_plane + left_plane) / (right_plane - left_plane), -(bottom_plane + top_plane) / (bottom_plane - top_plane), near_plane / (near_plane - far_plane), 1.0f
        };
    }

    /**
    * Création d'une matrice de rotation
    */
    Matrix4x4 Matrix4x4::RotationMatrix(float angle, Vector3 const& axis, bool normalize_axis, bool to_radians)
    {
        float rad_angle = to_radians ? angle * DEGREES_TO_RADIANS : angle;

        float x;
        float y;
        float z;

        if(normalize_axis) {
            Vector3 normalized = axis.Normalize();
            x = normalized[0];
            y = normalized[1];
            z = normalized[2];
        }else{
            x = axis[0];
            y = axis[1];
            z = axis[2];
        }

        const float c = std::cos(rad_angle);
        const float _1_c = 1.0f - c;
        const float s = std::sin(rad_angle);

        return std::array<float, 16>({
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
        });
    }

    /**
    * Création d'une matrice de translation
    */
    Matrix4x4 Matrix4x4::TranslationMatrix(Vector3 const& vector)
    {
        return {
            1.0f,      0.0f,      0.0f,         0.0f,
            0.0f,      1.0f,      0.0f,         0.0f,
            0.0f,      0.0f,      1.0f,         0.0f,
            vector.x,  vector.y,  vector.z,     1.0f
        };
    }

    /**
     * Création d'une matrice de rotation suivant un ordre de rotation axe après axe
     */
    Matrix4x4 Matrix4x4::EulerRotation(Matrix4x4 const& matrix, Vector3 const& vector, EULER_ORDER order)
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

    /**
     * Construction d'une matrice de changement d'échelle
     */
    Matrix4x4 Matrix4x4::ScalingMatrix(Vector3 const& vector)
    {
        return {
            vector[0],  0.0f,       0.0f,       0.0f,
            0.0f,       vector[1],  0.0f,       0.0f,
            0.0f,       0.0f,       vector[2],  0.0f,
            0.0f,       0.0f,       0.0f,       1.0f
        };
    }

    /**
     * Extrait la partie de la matrice qui concerne la rotation
     */
    Matrix4x4 Matrix4x4::ExtractRotation() const
    {
        float scale_x = 1.0f / Vector3({this->value[0], this->value[4], this->value[8]}).Length();
        float scale_y = 1.0f / Vector3({this->value[1], this->value[5], this->value[9]}).Length();
        float scale_z = 1.0f / Vector3({this->value[2], this->value[6], this->value[10]}).Length();

        return {
            this->value[0] * scale_x,    this->value[1] * scale_y,    this->value[2] * scale_z,     0,
            this->value[4] * scale_x,    this->value[5] * scale_y,    this->value[6] * scale_z,     0,
            this->value[8] * scale_x,    this->value[9] * scale_y,    this->value[10] * scale_z,    0,
            0,                           0,                           0,                            1
        };
    }

    /**
     * Extrait la partie de la matrice qui concerne la translation
     */
    Matrix4x4 Matrix4x4::ExtractTranslation() const
    {
        return {
            1.0f,               0.0f,               0.0f,               0.0f,
            0.0f,               1.0f,               0.0f,               0.0f,
            0.0f,               0.0f,               1.0f,               0.0f,
            this->value[12],    this->value[13],    this->value[14],    1.0f
        };
    }

    /**
     * Change la partie translation de la matrice
     */
    void Matrix4x4::SetTranslation(Vector3 const& translation)
    {
        this->value[12] = translation.x;
        this->value[13] = translation.y;
        this->value[14] = translation.z;
    }

    /**
     * Récupère la partie translation de la matrice
     */
    Vector3 Matrix4x4::GetTranslation() const
    {
        return {
            this->value[12],
            this->value[13],
            this->value[14]
        };
    }

    /**
     * Ne laisse à la matrice que sa translation
     */
    Matrix4x4 Matrix4x4::ToTranslationMatrix() const
    {
        return {
            1.0f,               0.0f,               0.0f,               0.0f,
            0.0f,               1.0f,               0.0f,               0.0f,
            0.0f,               0.0f,               1.0f,               0.0f,
            this->value[12],    this->value[13],    this->value[14],    1.0f
        };
    }

    /**
     * Calcule l'inverse de la matrice
     */
    Matrix4x4 Matrix4x4::Inverse() const
    {
        Matrix4x4 result;

		float n11 = this->value[0],  n21 = this->value[1],  n31 = this->value[2],  n41 = this->value[3],
		      n12 = this->value[4],  n22 = this->value[5],  n32 = this->value[6],  n42 = this->value[7],
		      n13 = this->value[8],  n23 = this->value[9],  n33 = this->value[10], n43 = this->value[11],
		      n14 = this->value[12], n24 = this->value[13], n34 = this->value[14], n44 = this->value[15],

		      t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44,
		      t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44,
		      t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44,
		      t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34,

		      det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
        
        // Déterminant = 0 => invesion impossible
		if(det == 0) return IDENTITY_MATRIX;

		float detInv = 1.0f / det;

		result[0] = t11 * detInv;
		result[1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * detInv;
		result[2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * detInv;
		result[3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * detInv;

		result[4] = t12 * detInv;
		result[5] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * detInv;
		result[6] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * detInv;
		result[7] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * detInv;

		result[8] = t13 * detInv;
		result[9] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * detInv;
		result[10] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * detInv;
		result[11] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * detInv;

		result[12] = t14 * detInv;
		result[13] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * detInv;
		result[14] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * detInv;
		result[15] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * detInv;

		return result;
    }

    Matrix4x4 Matrix4x4::Transpose() const
    {
        return {
            this->value[0], this->value[4], this->value[8], this->value[12],
            this->value[1], this->value[5], this->value[9], this->value[13],
            this->value[2], this->value[6], this->value[10], this->value[14],
            this->value[3], this->value[7], this->value[11], this->value[15]
        };
    }
}