//
// Created by alex on 4/9/25.
//

#include "CameraSystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan depth range is [0, 1]
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp> // For debugging if needed

namespace Bcg {
    void CameraSystem::initialize(ApplicationContext *context) {
        this->context = context;
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
        return &context->registry->emplace_or_replace<CameraParametersComponent>(entity, std::forward<CameraParametersComponent>(camera));
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

    void CameraSystem::update(CameraParametersComponent &camera) const {
        if (camera.dirtyView) {
            camera.viewMatrix = glm::lookAt(camera.position, camera.target, camera.up);
            camera.dirtyView = false;
        }

        if (camera.dirtyProjection) {
            camera.projectionMatrix = glm::perspective(glm::radians(camera.fovYDegrees),
                                                       camera.aspectRatio,
                                                       camera.nearPlane,
                                                       camera.farPlane);
            camera.dirtyProjection = false;
        }
    }

    void CameraSystem::setDistance(CameraParametersComponent &camera, float distance) {
        glm::vec3 backwards = glm::normalize(camera.position - camera.target);
        camera.position = camera.target + distance * backwards;
        camera.distance = distance;
        camera.dirtyView = true;
    }
}
