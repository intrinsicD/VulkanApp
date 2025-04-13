//
// Created by alex on 4/9/25.
//

#ifndef APPLICATIONCONTEXT_H
#define APPLICATIONCONTEXT_H

#include <entt/entt.hpp>
#include <memory>

namespace Bcg{
    class RendererSystem;
    class WindowManager;
    class UIManager;
    class InputManager;
    class CameraSystem;
    class SceneManager;

    struct ApplicationContext{
        std::unique_ptr<RendererSystem> rendererSystem;
        std::unique_ptr<WindowManager> windowManager;
        std::unique_ptr<UIManager> uiManager;
        std::unique_ptr<InputManager> inputManager;
        std::unique_ptr<CameraSystem> cameraSystem;
        std::unique_ptr<SceneManager> sceneManager;
        entt::registry* registry;
        entt::dispatcher* dispatcher;

        entt::entity cameraFocusEntity = entt::null;
    };
}

#endif //APPLICATIONCONTEXT_H
