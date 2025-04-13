//
// Created by alex on 4/9/25.
//

#ifndef CAMERACOMPONENT_H
#define CAMERACOMPONENT_H

#include <glm/glm.hpp>

namespace Bcg{
    struct CameraParametersComponent{
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

        glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
        glm::vec3 target = glm::vec3(0.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        float fovYDegrees = 45.0f;
        float aspectRatio = 1.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        float movementSpeed = 1.0f;
        float distance = 1.0f;

        bool dirtyProjection;
        bool dirtyView;
    };
}

#endif //CAMERACOMPONENT_H
