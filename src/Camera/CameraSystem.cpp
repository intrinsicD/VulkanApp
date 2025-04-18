//
// Created by alex on 4/9/25.
//

#include "CameraSystem.h"
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

}
