//
// Created by alex on 4/8/25.
//

#ifndef APPLICATION_H
#define APPLICATION_H

// --- Standard Libraries ---
#include <vector>
#include <chrono>
#include <memory>

// --- External Libraries ---


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan depth range is [0, 1]
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <entt/entt.hpp> // EnTT ECS and Dispatcher

#include "VulkanContext.h"

#include "Components.h"
#include "Events.h"
#include "ApplicationContext.h"

// --- Forward Declarations ---
namespace Bcg {
    struct VulkanContext;
    class RendererSystem;
    class Application;
    class IPlugin;
    class WindowManager;
    class UIManager;
    class InputManager;
    class CameraSystem;
    class SceneManager;
} // namespace Bcg

namespace Bcg {
    class Application {
    public:
        Application();

        ~Application();

        void run();

        // Accessors needed by other parts
        ApplicationContext *getApplicationContext() {
            return &m_applicationContext;
        }

        entt::registry &getRegistry() { return m_registry; }
        entt::dispatcher &getDispatcher() { return m_dispatcher; }

    private:
        void initECS();

        void initScene();

        void initCamera();

        void initInput();

        void initRenderer();

        void initImGui();

        void initImGuiGLFWBackend();

        void loadPlugins(); // Placeholder

        void mainLoop();

        void cleanup();

        // Event Handlers
        void onWindowResize(const WindowResizeEvent &event);

        void onLoadModelRequest(const LoadModelEvent &event); // Example event listener



        int m_width = 1280;
        int m_height = 720;
        bool m_framebufferResized = false;

        ApplicationContext m_applicationContext;

        entt::registry m_registry;
        entt::dispatcher m_dispatcher;

        // Basic Camera State
        entt::entity m_cameraFocusEntity = entt::null;

        // Input State
        bool m_mouseDragging = false;
        glm::dvec2 m_lastMousePos{0.0};

        // Plugins
        std::vector<std::unique_ptr<IPlugin> > m_plugins;

        // Timing
        std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    };
} // namespace Bcg

#endif //APPLICATION_H
