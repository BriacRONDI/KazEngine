#include <memory>
#include <array>
#include <cmath>

#if !defined(DEGREES_TO_RADIANS)
#define DEGREES_TO_RADIANS 0.01745329251994329576923690768489f
#endif

#if defined VEC_TYPE
#undef VEC_TYPE
#endif

#if VECTOR_N == 2 && !defined(VEC_TYPE_2)
#define VEC_TYPE Vector2
#define VEC_TYPE_2
#elif VECTOR_N == 3 && !defined(VEC_TYPE_3)
#define VEC_TYPE Vector3
#define VEC_TYPE_3
#elif VECTOR_N == 4 && !defined(VEC_TYPE_4)
#define VEC_TYPE Vector4
#define VEC_TYPE_4
#else
#undef VECTOR_N
#endif

#if defined(VEC_TYPE)

namespace Maths
{
    /**
     * Vertex class definition (vec2, vec3, vec4)
     */
    class VEC_TYPE
    {
        public :

            union
            {
                struct 
                {
                    float x;
                    float y;
                    #if VECTOR_N > 2
                    float z;
                    #endif
                    #if VECTOR_N > 3
                    float w;
                    #endif
                };
                
                #if VECTOR_N == 2
                std::array<float, 2> xy;
                #elif VECTOR_N == 3
                std::array<float, 3> xyz;
                #elif VECTOR_N == 4
                std::array<float, 4> xyzw;
                #endif
                std::array<float, VECTOR_N> value;
            };

            /// Initialization to zero
            inline VEC_TYPE() : value({}) {}

            /// Copy list initializer
            inline VEC_TYPE(std::array<float, VECTOR_N> const& list) : value(list) {}

            /// Move list initializer
            inline VEC_TYPE(std::array<float, VECTOR_N>&& list) : value(std::move(list)) {}

            /// Copy constructor
            inline VEC_TYPE(VEC_TYPE const& other) : value(other.value) {}

            /// Move constructor
            inline VEC_TYPE(VEC_TYPE&& other) : value(std::move(other.value)) {}

            /// Constructor from double values
            inline VEC_TYPE(std::array<double, VECTOR_N> value) { for(uint8_t i=0;i<VECTOR_N;i++) this->value[i] = static_cast<float>(value[i]); }

            /// Explicit values constructor
            /// {@
            #if VECTOR_N == 2
            inline VEC_TYPE(float x, float y) : x(x), y(y) {}
            #elif VECTOR_N == 3
            inline VEC_TYPE(float x, float y, float z) : x(x), y(y), z(z) {}
            #elif VECTOR_N == 4
            inline VEC_TYPE(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
            #endif
            /// @}

            /// Get reference or Set Vector3[i]
            inline float& operator[](uint32_t index){ return this->value[index]; }

            /// Get const Vector3[i]
            inline float operator[](uint32_t index) const { return this->value[index]; }

            /// Copy assignment
            inline VEC_TYPE& operator=(VEC_TYPE const& other) { if(this != &other) this->value = other.value; return *this; }

            /// Move assignment
            inline VEC_TYPE& operator=(VEC_TYPE&& other) { if(this != &other) this->value = std::move(other.value); return *this; }

            /// Equality operator
            inline bool operator==(VEC_TYPE const& other) const { return this->value == other.value; }

            /// Inequality operator
            inline bool operator!=(VEC_TYPE const& other) const { return this->value != other.value; }

            /// Base math operator
            /// @{
            #if VECTOR_N == 2
            inline VEC_TYPE operator*(VEC_TYPE const& other) const { return { this->x * other.x, this->y * other.y }; }
            inline VEC_TYPE operator*(float const scalar) const { return { this->x * scalar, this->y * scalar }; }
            inline VEC_TYPE operator/(float const scalar) const { return { this->x / scalar, this->y / scalar }; }
            inline VEC_TYPE operator+(VEC_TYPE const& other) const { return { this->x + other.x, this->y + other.y }; }
            inline VEC_TYPE operator-(VEC_TYPE const& other) const { return { this->x - other.x, this->y - other.y }; }
            inline VEC_TYPE operator-() const { return {-this->x, -this->y}; }
            inline float Length() const { return std::sqrt(this->x * this->x + this->y * this->y); }
            #elif VECTOR_N == 3
            inline VEC_TYPE operator*(VEC_TYPE const& other) const { return { this->x * other.x, this->y * other.y, this->z * other.z }; }
            inline VEC_TYPE operator*(float const scalar) const { return { this->x * scalar, this->y * scalar, this->z * scalar }; }
            inline VEC_TYPE operator/(float const scalar) const { return { this->x / scalar, this->y / scalar, this->z / scalar }; }
            inline VEC_TYPE operator+(VEC_TYPE const& other) const { return { this->x + other.x, this->y + other.y, this->z + other.z }; }
            inline VEC_TYPE operator-(VEC_TYPE const& other) const { return { this->x - other.x, this->y - other.y, this->z - other.z }; }
            inline VEC_TYPE operator-() const { return { -this->x, -this->y, -this->z }; }
            inline float Length() const { return std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z); }
            inline float Dot(VEC_TYPE const& other) const { return this->x * other.x + this->y * other.y + this->z * other.z; }
            inline VEC_TYPE Cross(VEC_TYPE const& other) const { return {this->y * other.z - other.y * this->z, this->z * other.x - other.z * this->x, this->x * other.y - other.x * this->y }; }
            static inline VEC_TYPE Min(VEC_TYPE const& a, VEC_TYPE const& b){ return { std::min<float>(a.x, b.x), std::min<float>(a.y, b.y), std::min<float>(a.z, b.z) }; }
            static inline VEC_TYPE Max(VEC_TYPE const& a, VEC_TYPE const& b){ return { std::max<float>(a.x, b.x), std::max<float>(a.y, b.y), std::max<float>(a.z, b.z) }; }
            #elif VECTOR_N == 4
            inline VEC_TYPE operator*(VEC_TYPE const& other) const { return { this->x * other.x, this->y * other.y, this->z * other.z, this->w * other.w }; }
            inline VEC_TYPE operator*(float const scalar) const { return { this->x * scalar, this->y * scalar, this->z * scalar, this->w * scalar }; }
            inline VEC_TYPE& operator/=(float const scalar) { this->x /= scalar; this->y /= scalar; this->z /= scalar; this->w /= scalar; return *this; }
            inline VEC_TYPE operator/(float const scalar) const { return { this->x / scalar, this->y / scalar, this->z / scalar, this->w / scalar }; }
            inline VEC_TYPE operator+(VEC_TYPE const& other) const { return { this->x + other.x, this->y + other.y, this->z + other.z, this->w + other.w }; }
            inline VEC_TYPE operator-(VEC_TYPE const& other) const { return { this->x - other.x, this->y - other.y, this->z - other.z, this->w + other.w }; }
            inline VEC_TYPE operator-() const { return { -this->x, -this->y, -this->z, -this->w }; }
            inline float Length() const { return std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w); }
            #endif
            /// @}

            /// Set vector length to 1
            VEC_TYPE Normalize() const { return *this / this->Length(); }

            /// Convert vector components from degrees to radians
            inline VEC_TYPE ToRadians() const { return *this * DEGREES_TO_RADIANS; }
            
            /// Compute the interpolated value between source and dest vector using ratio
            static inline VEC_TYPE Interpolate(VEC_TYPE const& source, VEC_TYPE const& dest, float ratio)
            {
                VEC_TYPE result;
                for(uint8_t i=0; i<VECTOR_N; i++) {
                    if(source[i] <= dest[i]) result[i] = source[i] + std::abs(dest[i] - source[i]) * ratio;
                    else result[i] = source[i] - std::abs(dest[i] - source[i]) * ratio;
                }
                return result;
            }

            #if defined(VEC_TYPE_4) && VECTOR_N == 3
            inline Vector3& operator=(Vector4 const& other) { this->value = {other.x, other.y, other.z}; return *this; }
            inline Vector3(Vector4 const& other) : value({other.x, other.y, other.z}) {}
            #endif

            // std::unique_ptr<char> Serialize() const;
            // size_t Deserialize(const char* data);
    };

    /// Base math operator
    /// @{
    #if VECTOR_N == 2
    inline VEC_TYPE operator/(float const scalar, VEC_TYPE const& vector) { return { scalar / vector.x, scalar / vector.y }; }
    inline VEC_TYPE operator*(float const scalar, VEC_TYPE const& vector) { return { scalar * vector.x, scalar * vector.y }; }
    #elif VECTOR_N == 3
    inline VEC_TYPE operator/(float const scalar, VEC_TYPE const& vector) { return { scalar / vector.x, scalar / vector.y, scalar / vector.z }; }
    inline VEC_TYPE operator*(float const scalar, VEC_TYPE const& vector) { return { scalar * vector.x, scalar * vector.y, scalar * vector.z }; }
    inline VEC_TYPE operator-(float const scalar, VEC_TYPE const& vector) { return { scalar - vector.x, scalar - vector.y, scalar - vector.z }; }
    inline VEC_TYPE operator+(float const scalar, VEC_TYPE const& vector) { return { scalar + vector.x, scalar + vector.y, scalar + vector.z }; }
    #elif VECTOR_N == 4
    inline VEC_TYPE operator/(float const scalar, VEC_TYPE const& vector) { return { scalar / vector.x, scalar / vector.y, scalar / vector.z, scalar / vector.w }; }
    inline VEC_TYPE operator*(float const scalar, VEC_TYPE const& vector) { return { scalar * vector.x, scalar * vector.y, scalar * vector.z, scalar * vector.w }; }
    #endif
    /// @}
}

#undef VECTOR_N
#endif