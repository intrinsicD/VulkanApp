//
// Created by alex on 4/9/25.
//

#ifndef CAMERASYSTEM_H
#define CAMERASYSTEM_H

#include <Components.h>

#include "System.h"
#include "CameraComponent.h"

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

        void update(CameraParametersComponent &camera) const;

        void setDistance(CameraParametersComponent &camera, float distance);

    private:
        CameraParametersComponent *m_camera;
    };
}

#endif //CAMERASYSTEM_H
