#include "Maths.h"

namespace Engine
{
    namespace Maths
    {
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
}