//
// Created by alex on 4/9/25.
//

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanUtils.h"

namespace Bcg{
    struct TransformComponent {
        glm::mat4 model{1.0f}; // Local-to-world transform
        // Add position, rotation, scale if needed for easier manipulation
        glm::vec3 position{0.0f};
        glm::quat rotation{glm::identity<glm::quat>()}; // Use quaternions for rotation
        glm::vec3 scale{1.0f};

        void updateModelMatrix() {
            model = glm::translate(glm::mat4(1.0f), position);
            model *= glm::mat4_cast(rotation);
            model = glm::scale(model, scale);
        }
    };


    struct CameraComponent {
        glm::mat4 view{1.0f};
        glm::mat4 projection{1.0f};
        float fovY = glm::radians(45.0f);
        float aspectRatio = 1.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        void updateProjectionMatrix() {
            projection = glm::perspective(fovY, aspectRatio, nearPlane, farPlane);
            projection[1][1] *= -1; // GLM was designed for OpenGL, Vulkan flips Y
        }
    };

    // Tag component to mark the entity the camera focuses on
    struct CameraFocusTarget {
    };

    // Tag component to mark entities needing GPU buffer updates
    struct DirtyGPUResource {
    };
}

#endif //COMPONENTS_H
