#include "Maths.h"

namespace Engine
{
    namespace Maths
    {
        // Intersection entre une ligne et une hit-box (ray-box intersection)
        // https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
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
    }
}