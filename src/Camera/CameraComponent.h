//
// Created by alex on 4/9/25.
//

#ifndef CAMERACOMPONENT_H
#define CAMERACOMPONENT_H

#include "MatVec.h"

namespace Bcg{
    struct CameraParametersComponent{
        Eigen::Affine3f viewMatrix = Eigen::Affine3f::Identity();
        Matrix4f projectionMatrix = Matrix4f::Identity();

        Vector3f position = Vector3f(0.0f, 0.0f, 5.0f);
        Vector3f target = Vector3f::Zero();
        Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);

        float fovYDegrees = 45.0f;
        float aspectRatio = 1.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        float movementSpeed = 1.0f;
        float zoomSensitivity = 0.5f;
        float rotSensitivity = 0.005f;
        float distance = 1.0f;

        bool dirtyProjection;
        bool dirtyView;

        Vector2f lastMousePosition = Vector2f::Zero();;
    };

    inline void Perspective(Matrix4f &proj, float fovYDegrees, float aspectRatio, float nearPlane, float farPlane) {
        float tanHalfFovY = std::tan(radians(fovYDegrees) * 0.5f);
        proj = Eigen::Matrix4f::Zero();
        proj(0, 0) = 1.0f / (aspectRatio * tanHalfFovY);
        proj(1, 1) = 1.0f / tanHalfFovY; //DO i need to flip this in vulkan?
        proj(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        proj(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        proj(3, 2) = -1.0f;
    }

    inline void LookAt(Eigen::Affine3f &view, const Vector3f &position, const Vector3f &target, const Vector3f &up) {
        Eigen::Vector3f forward = (target - position).normalized();
        Eigen::Vector3f right = forward.cross(up).normalized();
        Eigen::Vector3f up_ = right.cross(forward);

        view = Eigen::Affine3f::Identity();
        view.linear().row(0) = right.transpose();
        view.linear().row(1) = up_.transpose();
        view.linear().row(2) = -forward.transpose();
        view(0, 3) = -right.dot(position);
        view(1, 3) = -up.dot(position);
        view(2, 3) = forward.dot(position);
    }
}

#endif //CAMERACOMPONENT_H
