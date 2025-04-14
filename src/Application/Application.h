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




#include <entt/entt.hpp> // EnTT ECS and Dispatcher

#include "VulkanContext.h"
#include "MatVec.h"
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
        Application(int width = 1280, int height = 720, const std::string &title = "Vulkan EnTT App");

        ~Application();

        void run();
    private:
        // Accessors needed by other parts
        ApplicationContext *getApplicationContext() {
            return &m_applicationContext;
        }

        void initECS();

        void loadPlugins(); // Placeholder

        void mainLoop();

        void cleanup();

        // Event Handlers
        void onWindowResize(const WindowResizeEvent &event);

        void onLoadModelRequest(const LoadModelEvent &event); // Example event listener

        ApplicationContext m_applicationContext;

        entt::registry m_registry;
        entt::dispatcher m_dispatcher;

        // Plugins
        std::vector<std::unique_ptr<IPlugin> > m_plugins;

        // Timing
        std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    };
} // namespace Bcg

#endif //APPLICATION_H
