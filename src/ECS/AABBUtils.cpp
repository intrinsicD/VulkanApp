//
// Created by alex on 4/24/25.
//

#include "AABBUtils.h"

namespace Bcg::AABBUtils {
    void build(AABBComponent &aabb,
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