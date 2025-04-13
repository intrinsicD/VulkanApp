//
// Created by alex on 4/9/25.
//

#ifndef EVENTS_H
#define EVENTS_H

#include <entt/entity/fwd.hpp>
#include <string>
#include <glm/glm.hpp>

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
        glm::vec3 initialPosition{0.0f};
        glm::vec3 initialScale{1.0f};
    };
}

#endif //EVENTS_H
