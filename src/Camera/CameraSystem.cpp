//
// Created by alex on 4/9/25.
//

#include "CameraSystem.h"
#include "TransformSystem.h"
#include "WindowManager.h"

namespace Bcg {
    void CameraSystem::initialize(ApplicationContext *context) {
        this->context = context;

        auto camera_id = context->cameraSystem->createCamera();
        auto &camera = context->registry->get<CameraParametersComponent>(camera_id);

        camera.aspectRatio = context->windowManager->getWidth() / context->windowManager->getHeight();
        camera.dirtyProjection = true;
        context->cameraSystem->setCurrentCamera(&camera);
    }

    void CameraSystem::shutdown() {
    }

    entt::entity CameraSystem::createCamera() {
        auto entity = context->registry->create();
        context->registry->emplace<CameraParametersComponent>(entity);
        return entity;
    }

    CameraParametersComponent *CameraSystem::createCamera(entt::entity entity) {
        if (!context->registry->valid(entity)) {
            return nullptr;
        }
        return &context->registry->emplace_or_replace<CameraParametersComponent>(entity);
    }

    CameraParametersComponent *CameraSystem::createCamera(entt::entity entity, CameraParametersComponent &&camera) {
        if (!context->registry->valid(entity)) {
            return nullptr;
        }
        return &context->registry->emplace_or_replace<CameraParametersComponent>(entity,
            std::forward<CameraParametersComponent>(
                camera));
    }

    void CameraSystem::destroyCamera(entt::entity entity) {
        if (!context->registry->valid(entity)) {
            return;
        }
        if (!context->registry->all_of<CameraParametersComponent>(entity)) {
            return;
        }
        context->registry->remove<CameraParametersComponent>(entity);
    }

    CameraParametersComponent *CameraSystem::getCurrentCamera() {
        return m_camera;
    }

    const CameraParametersComponent *CameraSystem::getCurrentCamera() const {
        return m_camera;
    }

    void CameraSystem::setCurrentCamera(CameraParametersComponent *camera) {
        m_camera = camera;
    }

    void CameraSystem::update(CameraParametersComponent &camera) {
        if (camera.dirtyView) {
            LookAt(camera.viewMatrix, camera.position, camera.target, camera.up);
            camera.dirtyView = false;
        }

        if (camera.dirtyProjection) {
            Perspective(camera.projectionMatrix, camera.fovYDegrees, camera.aspectRatio, camera.nearPlane,
                        camera.farPlane);
            camera.dirtyProjection = false;
        }
    }

    void CameraSystem::setDistanceToTarget(CameraParametersComponent &camera, float distance) {
        Vector3f backwards = (camera.position - camera.target).normalized();
        camera.position = camera.target + distance * backwards;
        camera.distance = distance;
        camera.dirtyView = true;
    }

    void CameraSystem::setFromMatrix(CameraParametersComponent &camera, const Matrix4f &model_matrix) {
        TransformComponent transform;
        TransformSystem::setFromMatrix(transform, model_matrix);
        setFromTransform(camera, transform);
    }

    void CameraSystem::setFromTransform(CameraParametersComponent &camera, const TransformComponent &transform) {
        camera.position = transform.position;
        camera.target = transform.position + transform.rotation * Vector3f(0.0f, 0.0f, -1.0f);
        camera.up = transform.rotation * Vector3f(0.0f, 1.0f, 0.0f);
        camera.dirtyView = true;
    }

    void CameraSystem::zoom(CameraParametersComponent &camera, float delta) {
        auto backward = camera.position - camera.target;
        auto len = backward.norm();
        if (len < 0.1f) {
            // Prevent zooming in too close
            return;
        }
        camera.position = camera.target + (len - delta) * camera.zoomSensitivity * (backward / len);
        camera.dirtyView = true;
    }

    void CameraSystem::arcball(CameraParametersComponent &camera, const Mouse &mouse) {
        Vector2f delta = mouse.current.cursor_position - camera.lastMousePosition;
        camera.lastMousePosition = mouse.current.cursor_position;
        // Calculate rotation angles (in radians)
        float angle_x = delta.x() * camera.rotSensitivity;
        float angle_y = delta.y() * camera.rotSensitivity;

        // Compute vector from target to current camera position
        Vector3f v = camera.position - camera.target;
        Vector3f worldUp = Vector3f(0.0f, 1.0f, 0.0f);

        // --- Horizontal Rotation ---
        // Rotate around the world up (Y) axis.
        Eigen::AngleAxisf horiz_rot(angle_x, worldUp);
        v = horiz_rot * v;
        // Also rotate the current up vector
        Vector3f new_up = horiz_rot * camera.up;

        // --- Compute the Right Axis for Vertical Rotation ---
        // First, compute forward direction from camera toward the target.
        // Since v = camera->position - camera->target, the forward vector is -v (normalized).
        Vector3f forward = (-v).normalized();
        // Compute right vector using the conventional lookAt ordering:
        // right = normalize(cross(forward, new_up));
        Vector3f right = forward.cross(new_up).normalized();

        // --- Vertical Rotation ---
        // Rotate v and new_up about the right axis.
        Eigen::AngleAxisf vert_rot(angle_y, right);
        v = vert_rot * v;
        new_up = vert_rot * new_up;

        // Update camera parameters
        camera.position = camera.target + v;
        camera.up = new_up.normalized();

        // Mark the view matrix as needing update (if applicable).
        camera.dirtyView = true;
    }
}
