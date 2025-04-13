//
// Created by alex on 4/8/25.
//

#include "Application.h"
#include "Logger.h"
#include "RendererSystem.h"
#include "RenderComponents.h"
#include "IPlugin.h"
#include "WindowManager.h"
#include "UIManager.h"
#include "InputManager.h"
#include "CameraSystem.h"
#include "SceneManager.h"

#include <imgui_internal.h>
#include <iostream> // Needed for Vertex Attribute Descriptions
#include <RendererSystem.h>

#include <glm/gtx/string_cast.hpp>

#include <GLFW/glfw3.h>


// Link Slang library
// #pragma comment(lib, "slang.lib") // Example for MSVC

// Optional: Link CUDA runtime library
// #pragma comment(lib, "cudart.lib") // Example for MSVC


namespace Bcg {
    // --- Application Method Implementations ---

    Application::Application() {
        Log::Init();
        m_lastFrameTime = std::chrono::high_resolution_clock::now();
    }

    Application::~Application() {
        // Cleanup happens in reverse order of initialization
        cleanup();
    }

    void Application::run() {
        auto context = getApplicationContext();
        context->windowManager = std::make_unique<WindowManager>(m_width, m_height, "Vulkan EnTT App");
        initECS(); // Includes event setup
        context->sceneManager = std::make_unique<SceneManager>();
        context->cameraSystem = std::make_unique<CameraSystem>();
        context->uiManager = std::make_unique<UIManager>();
        context->rendererSystem = std::make_unique<RendererSystem>();
        context->inputManager = std::make_unique<InputManager>();

        context->windowManager->initialize(context);
        context->sceneManager->initialize(context);
        context->cameraSystem->initialize(context);
        context->uiManager->initialize(context);
        context->rendererSystem->initialize(context); // Renderer performs its specific setup
        context->uiManager->initGLFWBackend(); // Initialize ImGui GLFW backend
        context->inputManager->initialize(context);

        auto vkContext = context->rendererSystem->getVulkanContext();

        //TODO move to CameraSystem
        auto camera_id = context->cameraSystem->createCamera();
        auto &camera = context->registry->get<CameraParametersComponent>(camera_id);
        camera.aspectRatio = m_width / m_height;
        camera.dirtyProjection = true;
        context->cameraSystem->setCurrentCamera(&camera);

        loadPlugins(); // Init plugins after core systems are ready

        // Trigger initial load event or load default scene
        //m_dispatcher.trigger<LoadModelEvent>({"models/viking_room.obj", {0, 0, 0}, {1, 1, 1}}); // Example load
        m_dispatcher.trigger<LoadModelEvent>({"models/star.obj", {0, 0, 0}, {1, 1, 1}}); // Example load

        mainLoop();

        // --- ImGui Cleanup --- <<<
        // 1. Ensure GPU is idle before ANY cleanup
        if (vkContext->device != VK_NULL_HANDLE) {
            VK_CHECK(vkDeviceWaitIdle(vkContext->device));
        }


        // 3. Call Application's main cleanup, which includes:
        //    - Plugin shutdown
        //    - Renderer shutdown
        //    - ECS resource cleanup
        //    - VulkanContext cleanup (which calls ImGui_ImplVulkan_Shutdown) <<<< This is key
        //    - GLFW cleanup
        cleanup(); // <<< Call the existing cleanup function
    }

    void Application::initECS() {
        Log::Info("Initializing ECS...");
        // EnTT registry and dispatcher are already members

        // Connect event listeners
        m_dispatcher.sink<WindowResizeEvent>().connect<&Application::onWindowResize>(this);
        m_dispatcher.sink<LoadModelEvent>().connect<&Application::onLoadModelRequest>(this);

        // TODO: Connect more application-level event listeners
        // m_dispatcher.sink<EntitySelectedEvent>()...
        m_applicationContext.registry = &m_registry;
        m_applicationContext.dispatcher = &m_dispatcher;
    }

    void Application::initInput() {
        Log::Info("Initializing Input...");
        m_applicationContext.inputManager = std::make_unique<InputManager>();
    }

    void Application::loadPlugins() {
        Log::Info("Loading Plugins...");
        // --- Plugin Loading ---
        // For this example, we'll just create plugin instances directly.
        // A real system might use dynamic loading (dlopen/LoadLibrary)
        // or a registration mechanism.

        // Example: Create a hypothetical 'GizmoPlugin'
        // class GizmoPlugin : public IPlugin { ... };
        // m_plugins.push_back(std::make_unique<GizmoPlugin>());

        // Initialize loaded plugins
        for (auto &plugin: m_plugins) {
            Log::Info("Loading Plugin: {}", plugin->getName());
            plugin->initialize(&m_applicationContext);
            // Allow plugins to interact during setup
            plugin->registerComponents();
            plugin->connectEvents();
            plugin->registerRenderPasses();
        }
    }

    void Application::mainLoop() {
        auto vkContext = m_applicationContext.rendererSystem->getVulkanContext();

        while (!m_applicationContext.windowManager->shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(
                        currentTime - m_lastFrameTime).
                    count();
            m_lastFrameTime = currentTime;

            // --- Input ---
            m_applicationContext.windowManager->pollEvents(); // Check for window events, input, etc.
            m_applicationContext.inputManager->processInput(deltaTime); // Handle continuous input (e.g., key holds)

            // --- Update ---

            // Update entity transforms (simple example)
            auto view = m_registry.view<TransformComponent>();
            for (auto entity: view) {
                // Example: Simple rotation
                // if (m_registry.all_of<RotateMe>(entity)) { ... }
                // view.get<TransformComponent>(entity).updateModelMatrix();
            }

            // Update Plugins
            for (auto &plugin: m_plugins) {
                plugin->update(deltaTime);
            }

            // TODO: Run other application/ECS systems (Physics, Animation, AI...)

            // --- Process Dirty Resources ---
            // Example: If a system marked a mesh as dirty, re-upload it
            auto dirtyView = m_registry.view<DirtyGPUResource, VulkanMeshComponent /*, OtherDataComponent*/>();
            for (auto entity: dirtyView) {
                // Re-fetch data from source components (e.g., CPU-side mesh data)
                // const auto& cpuMeshData = m_registry.get<CPUMeshData>(entity);
                // m_renderer->uploadMesh(entity, cpuMeshData.vertices, cpuMeshData.indices);
                m_registry.remove<DirtyGPUResource>(entity); // Clear flag
            }

            // --- Render ---
            if (m_applicationContext.rendererSystem) {
                m_applicationContext.rendererSystem->drawFrame();
            }
        }

        // Wait for device to finish operations before cleanup
        VK_CHECK(vkDeviceWaitIdle(vkContext->device));
    }

    void Application::cleanup() {
        Log::Info("Cleaning up...");
        // Shutdown plugins first
        if (!m_plugins.empty()) {
            Log::Info("Shutting down plugins...");
            for (auto &plugin: m_plugins) {
                if (plugin) {
                    Log::Info("Shutting down plugin: {}", plugin->getName());
                    plugin->shutdown();
                }
            }
            m_plugins.clear(); // Destroys plugin instances
        }


        // --- Clean up Vulkan resources managed by ECS components ---
        if (m_applicationContext.sceneManager) {
            m_applicationContext.sceneManager->clearScene();
        }

        // Shutdown renderer (cleans its specific Vulkan resources)
        if (m_applicationContext.rendererSystem) {
            m_applicationContext.rendererSystem->shutdown();
            auto vkContext = m_applicationContext.rendererSystem->getVulkanContext();
            vkContext->cleanup();
            m_applicationContext.rendererSystem.reset(); // Destroys renderer instance
        }

        m_applicationContext.uiManager->shutdown();

        Log::Info("Application cleanup complete.");
    }


    // --- Event Handlers ---

    void Application::onWindowResize(const WindowResizeEvent &event) {
        // Set flag for Vulkan context to handle swapchain recreation
        m_framebufferResized = true;
        m_width = event.width;
        m_height = event.height;

        // Update camera aspect ratio if dimensions are valid
        if (event.width > 0 && event.height > 0) {
            auto camera = m_applicationContext.cameraSystem->getCurrentCamera();
            camera->aspectRatio = (float) m_width / (float) m_height;
            camera->dirtyProjection = true;
            std::cout << "Window resized: " << event.width << "x" << event.height << std::endl;
        }
    }

    void Application::onLoadModelRequest(const LoadModelEvent &event) {
        // Delegate to the loading function
        entt::entity loadedEntity = m_applicationContext.sceneManager->loadModel(event.filepath, event.initialPosition, event.initialScale);

        if (loadedEntity == entt::null) {
            Log::Error("Model loading failed for '{}', cannot set focus.", event.filepath);
            return;
        }
    }
} // namespace Bcg
