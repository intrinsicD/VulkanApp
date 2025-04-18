//
// Created by alex on 4/18/25.
//

#ifndef AABBUTILS_H
#define AABBUTILS_H

#include "AABBComponent.h"

namespace Bcg::AABBUtils {
    inline void clear(AABBComponent &aabb) {
        aabb.min = Vector3f::Constant(std::numeric_limits<float>::max());
        aabb.max = Vector3f::Constant(std::numeric_limits<float>::lowest());
    }

    inline void grow(AABBComponent &aabb, const Vector3f &point) {
        aabb.min = point.cwiseMin(aabb.min);
        aabb.max = point.cwiseMax(aabb.max);
        aabb.dirty = true;
    }

    inline void build(AABBComponent &aabb,
                      const std::vector<Vector3f> &points,
                      const Eigen::Affine3f &world_xf) {
        if (points.empty()) {
            clear(aabb);
            return;
        }

        // transform the first point
        Vector3f p0 = world_xf * points[0];
        aabb.min = aabb.max = p0;

        for (size_t i = 1, n = points.size(); i < n; ++i) {
            Vector3f pw = world_xf * points[i];
            aabb.min = pw.cwiseMin(aabb.min);
            aabb.max = pw.cwiseMax(aabb.max);
        }
    }
}

#endif //AABBUTILS_H
