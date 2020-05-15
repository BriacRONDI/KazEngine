#pragma once

#include <cmath>
#include "Plane.hpp"
// #define VECTOR_N 3
// #include "../Vector/Vector.hpp"

// #define DISPLAY_LOGS
#if defined(DISPLAY_LOGS)
#include <iostream>
#endif

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
    inline bool ray_box_aabb_intersect(Vector3 const& ray_origin, Vector3 const& ray_direction, Vector3 const& box_min, Vector3 const& box_max, float min_range, float max_range)
    {
        Vector3 inverse_ray = 1.0f / ray_direction;
        Vector3 t0s = (box_min - ray_origin) * inverse_ray;
        Vector3 t1s = (box_max - ray_origin) * inverse_ray;

        Vector3 tsmaller = Vector3::Min(t0s, t1s);
        Vector3 tbigger  = Vector3::Max(t0s, t1s);

        auto tmin = std::max<float>(min_range, std::max<float>(tsmaller[0], std::max<float>(tsmaller[1], tsmaller[2])));
        auto tmax = std::min<float>(max_range, std::min<float>(tbigger[0], std::min<float>(tbigger[1], tbigger[2])));

        return (tmin < tmax);
    }

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
    inline bool intersect_plane(Vector3 const& plane_normal, Vector3 const& plane_origin, Vector3 const& ray_origin, Vector3 const& ray_direction, float &ray_length) 
    { 
        // assuming vectors are all normalized
        float denom = plane_normal.Dot(ray_direction); 
        if(denom > 1e-6) { 
            Vector3 distance = plane_origin - ray_origin; 
            ray_length = distance.Dot(plane_normal) / denom; 
            return (ray_length >= 0); 
        } 
 
        return false; 
    }

    /**
     * https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
     */
    inline bool aabb_inside_half_space(Plane plane, Vector3 const& box_min, Vector3 const& box_max)
    {
        Vector3 center = (box_min + box_max) * 0.5f;
        // auto extents = box_max - center;
        float distance_from_plane = plane.normal.Dot(center - plane.origin);
        return distance_from_plane >= 0;
        /*float projection_radius = extents.x * std::abs(plane.normal.x) + extents.y * std::abs(plane.normal.y) + extents.z * std::abs(plane.normal.z);
        #if defined(DISPLAY_LOGS)
        std::cout << "distance_from_plane : " << distance_from_plane << ", projection_radius : " << projection_radius << std::endl;
        #endif
        return distance_from_plane >= projection_radius;*/
    }

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