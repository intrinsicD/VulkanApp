//
// Created by alex on 4/16/25.
//

#include "AABBSystem.h"

namespace Bcg {
    void AABBSystem::initialize(ApplicationContext *context) {
    }

    void AABBSystem::shutdown() {
    }

    void AABBSystem::update(AABBComponent &aabb) {
        if (!aabb.dirty) return;
    }


    void AABBSystem::grow(AABBComponent &aabb, const Vector3f &point) {
        aabb.min = point.cwiseMin(aabb.min);
        aabb.max = point.cwiseMax(aabb.max);
        aabb.dirty = true;
    }
}
