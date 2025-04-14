//
// Created by alex on 4/9/25.
//

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "MatVec.h"

#include "VulkanUtils.h"

namespace Bcg {



    struct CameraComponent {
        Matrix4f view = Matrix4f::Identity();
        Matrix4f projection = Matrix4f::Identity();
        float fovY = radians(45.0f);
        float aspectRatio = 1.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        void updateProjectionMatrix() {
            float tanHalfFovY = std::tan(fovY / 2.0f);

            projection.setZero();
            projection(0, 0) = 1.0f / (aspectRatio * tanHalfFovY);
            projection(1, 1) = -1.0f / tanHalfFovY; // Vulkan flips Y
            projection(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
            projection(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
            projection(3, 2) = -1.0f;
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
