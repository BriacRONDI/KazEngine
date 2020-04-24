#include "Others.h"

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
    bool ray_box_aabb_intersect(Vector3 const& ray_origin, Vector3 const& ray_direction, Vector3 const& box_min, Vector3 const& box_max, float min_range, float max_range)
    {
        Vector3 inverse_ray = 1.0f / ray_direction;
        Vector3 t0s = (box_min - ray_origin) * inverse_ray;
        Vector3 t1s = (box_max - ray_origin) * inverse_ray;

        Vector3 tsmaller = Vector3::Min(t0s, t1s);
        Vector3 tbigger  = Vector3::Max(t0s, t1s);

        float tmin = std::max(min_range, std::max(tsmaller[0], std::max(tsmaller[1], tsmaller[2])));
        float tmax = std::min(max_range, std::min(tbigger[0], std::min(tbigger[1], tbigger[2])));

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
    bool intersect_plane(Vector3 const& plane_normal, Vector3 const& plane_origin, Vector3 const& ray_origin, Vector3 const& ray_direction, float &ray_length) 
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
}