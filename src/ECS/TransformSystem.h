//
// Created by alex on 4/9/25.
//

#ifndef TRANSFORMSYSTEM_H
#define TRANSFORMSYSTEM_H

#include "System.h"
#include "TransformComponent.h"

namespace Bcg {
    struct TransformNeedsUpdate {
        // This struct is used to mark entities that need transform updates
    };

    class TransformSystem : public System {
    public:
        ~TransformSystem() override = default;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        void update();
    };
}

#endif //TRANSFORMSYSTEM_H
