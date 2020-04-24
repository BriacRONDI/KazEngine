#pragma once

#include <cmath>

#define VECTOR_N 3
#include "../Vector/Vector.hpp"

namespace Maths
{
    /**
     * Intersection between a ray and a hit-box : <a href="https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525">Source link</a>
     * @param ray_origin Ray origin point
     * @param ray_direction Ray direction vector, must be normalized
     * @param box_min Near-bottom-left hit-box corner
     * @param box_max Far-top-right hit-box corner
     * @param min_range Intersection minimal limit
     * @param max_range Intersection maximal limit
     * @retval true Ray intersects hit-box
     * @retval false Ray does not intersect hit-box
     */
    bool ray_box_aabb_intersect(Vector3 const& ray_origin, Vector3 const& ray_direction, Vector3 const& box_min, Vector3 const& box_max, float min_range, float max_range);

    /**
     * Intersection between a ray and a plane : <a href="https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection">Source link</a> 
     * @param plane_normal Plane normal, must be normalized
     * @param plane_origin Any point on the plane
     * @param ray_origin Ray origin point
     * @param ray_direction Ray direction vector, must be normalized
     * @param[out] ray_length Distance between ray origin and plane, if intersects
     * @retval true Ray intersects plane
     * @retval false Ray does not intersect plane
     */
    bool intersect_plane(Vector3 const& plane_normal, Vector3 const& plane_origin, Vector3 const& ray_origin, Vector3 const& ray_direction, float &ray_length);

    /**
     * Linear interpolation of a float value
     * @param source Initial value
     * @param dest Final value
     * @param ratio Interpolation progression
     * @return Interpolated value
     */
    inline float Interpolate(float source, float dest, float ratio)
    {
        if(source <= dest) return source + std::abs(dest - source) * ratio;
        else return source - std::abs(dest - source) * ratio;
    }
}