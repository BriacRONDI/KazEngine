#pragma once

#define DEGREES_TO_RADIANS 0.01745329251994329576923690768489f
#define IDENTITY_MATRIX {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}
#define ZERO_MATRIX {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

#include <array>

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix.h"

namespace Engine
{
    namespace Maths
    {
        // Renvoie la valeur comprise entre source et dest, positionnée suivant un ratio allant de 0 à 1
        inline float Interpolate(float source, float dest, float ratio)
        {
            if(source <= dest) return source + std::abs(dest - source) * ratio;
            else return source - std::abs(dest - source) * ratio;
        }

        // Intersection entre une ligne et une hit-box (ray-box intersection)
        // https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
        bool ray_box_aabb_intersect(Vector3 const& ray_origin, Vector3 const& ray_direction, Vector3 const& box_min, Vector3 const& box_max, float min_range, float max_range);

        // Intersection entre une ligne et un plan
        // https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection
        bool intersect_plane(Vector3 const& plane_normal, Vector3 const& plane_origin, Vector3 const& ray_origin, Vector3 const& ray_direction, float &ray_length);
    }
}