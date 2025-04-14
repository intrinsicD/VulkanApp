//
// Created by alex on 4/9/25.
//

#ifndef EVENTS_H
#define EVENTS_H

#include <entt/entity/fwd.hpp>
#include <string>
#include "MatVec.h"

namespace Bcg{
    struct WindowResizeEvent {
        int width;
        int height;
    };

    struct EntitySelectedEvent {
        // Example event
        entt::entity selectedEntity;
    };

    struct LoadModelEvent {
        // Example event
        std::string filepath;
        Vector3f initialPosition = Vector3f::Zero();
        Vector3f initialScale = Vector3f::Ones();
    };
}

#endif //EVENTS_H
