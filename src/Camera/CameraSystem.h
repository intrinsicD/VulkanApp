//
// Created by alex on 4/9/25.
//

#ifndef CAMERASYSTEM_H
#define CAMERASYSTEM_H

#include "TransformComponent.h"

#include "Components.h"

#include "System.h"
#include "CameraComponent.h"
#include "Mouse.h"

namespace Bcg {
    class CameraSystem : public System {
    public:
        ~CameraSystem() override = default;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        entt::entity createCamera();

        CameraParametersComponent *createCamera(entt::entity entity);

        CameraParametersComponent *createCamera(entt::entity entity, CameraParametersComponent &&camera);

        void destroyCamera(entt::entity entity);

        CameraParametersComponent *getCurrentCamera();

        const CameraParametersComponent *getCurrentCamera() const;

        void setCurrentCamera(CameraParametersComponent *camera);

        static void update(CameraParametersComponent &camera);

        static void setDistanceToTarget(CameraParametersComponent &camera, float distance);

        static void setFromMatrix(CameraParametersComponent &camera, const Matrix4f &model_matrix);

        static void setFromTransform(CameraParametersComponent &camera, const TransformComponent &model_matrix);

        static void zoom(CameraParametersComponent &camera, float delta);

        static void arcball(CameraParametersComponent &camera, const Mouse &mouse);
    private:
        CameraParametersComponent *m_camera;
        Vector2f m_arcball_last = Vector2f(0, 0);
    };
}

#endif //CAMERASYSTEM_H
