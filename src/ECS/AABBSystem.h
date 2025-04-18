//
// Created by alex on 4/16/25.
//

#ifndef AABBSYSTEM_H
#define AABBSYSTEM_H

#include "System.h"
#include "AABBComponent.h"

namespace Bcg {
    struct NeedsAABBUpdate {
        // This struct is used to mark entities that need AABB updates
    };

    class AABBSystem : public System {
    public:
        ~AABBSystem() override = default;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        void update();

        static void update(AABBComponent &aabb);

        static void grow(AABBComponent &aabb, const Vector3f &point);
    };
}
#endif //AABBSYSTEM_H
