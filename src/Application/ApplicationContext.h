//
// Created by alex on 4/9/25.
//

#ifndef APPLICATIONCONTEXT_H
#define APPLICATIONCONTEXT_H

#include <entt/entt.hpp>
#include <memory>

namespace Bcg{

    //Managers
    class WindowManager;
    class UIManager;
    class InputManager;
    class SceneManager;

    //Systems
    class TransformSystem;
    class CameraSystem;
    class RendererSystem;
    class AABBSystem;

    struct ApplicationContext{
        //Managers
        std::unique_ptr<WindowManager> windowManager;
        std::unique_ptr<UIManager> uiManager;
        std::unique_ptr<InputManager> inputManager;
        std::unique_ptr<SceneManager> sceneManager;

        //Systems
        std::unique_ptr<RendererSystem> rendererSystem;
        std::unique_ptr<CameraSystem> cameraSystem;
        std::unique_ptr<TransformSystem> transformSystem;
        std::unique_ptr<AABBSystem> aabbSystem;

        entt::registry* registry;
        entt::dispatcher* dispatcher;

        entt::entity cameraFocusEntity = entt::null;
    };
}

#endif //APPLICATIONCONTEXT_H
