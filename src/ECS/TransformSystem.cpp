//
// Created by alex on 4/9/25.
//

#include "TransformSystem.h"
#include "TransformUtils.h"

namespace Bcg {
    void TransformSystem::initialize(ApplicationContext *context) {
        this->context = context;
    }

    void TransformSystem::shutdown() {
    }

    void TransformSystem::update() {
        auto &registry = context->registry;
        auto view = registry->view<TransformComponent, TransformNeedsUpdate>();
        for (auto entity: view) {
            auto &transform = view.get<TransformComponent>(entity);
            TransformUtils::update(transform);
            registry->remove<TransformNeedsUpdate>(entity);
        }
    }

}
