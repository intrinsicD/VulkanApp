//
// Created by alex on 4/16/25.
//

#include "AABBSystem.h"


#include "AABBUtils.h"
#include "GeometryPositionsComponent.h"
#include "TransformComponent.h"

namespace Bcg {
    void AABBSystem::initialize(ApplicationContext *context) {
        this->context = context;
    }

    void AABBSystem::shutdown() {
    }

    void AABBSystem::update() {
        auto &registry = context->registry;
        auto view = registry->view<GeometryPositionsComponent, AABBComponent, NeedsAABBUpdate, TransformComponent>();

        for (auto entity: view) {
            auto &aabb = view.get<AABBComponent>(entity);
            auto &geometry = view.get<GeometryPositionsComponent>(entity);
            auto &transform = view.get<TransformComponent>(entity);

            if (geometry.positions && !geometry.positions->empty()) {
                AABBUtils::build(aabb, *geometry.positions, transform.cachedModelMatrix);
            } else {
                AABBUtils::clear(aabb);
            }
            registry->remove<NeedsAABBUpdate>(entity);
        }
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
