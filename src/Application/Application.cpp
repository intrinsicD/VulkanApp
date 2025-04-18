//
// Created by alex on 4/8/25.
//

#include "Application.h"
#include "Logger.h"
#include "RenderComponents.h"
#include "IPlugin.h"
#include "WindowManager.h"
#include "UIManager.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "RendererSystem.h"
#include "CameraSystem.h"
#include "TransformSystem.h"
#include "AABBSystem.h"

#include <iostream> // Needed for Vertex Attribute Descriptions

// Link Slang library
// #pragma comment(lib, "slang.lib") // Example for MSVC

// Optional: Link CUDA runtime library
// #pragma comment(lib, "cudart.lib") // Example for MSVC


namespace Bcg {
    Application::Application(int width, int height, const std::string &title) {
        Log::Init();
        Log::setLevel(spdlog::level::debug);

        m_lastFrameTime = std::chrono::high_resolution_clock::now();

        auto context = getApplicationContext();
        context->windowManager = std::make_unique<WindowManager>(width, height, title);
        context->registry = &m_registry;
        context->dispatcher = &m_dispatcher;
        context->sceneManager = std::make_unique<SceneManager>();
        context->cameraSystem = std::make_unique<CameraSystem>();
        context->uiManager = std::make_unique<UIManager>();
        context->rendererSystem = std::make_unique<RendererSystem>();
        context->inputManager = std::make_unique<InputManager>();
        context->transformSystem = std::make_unique<TransformSystem>();
        context->aabbSystem = std::make_unique<AABBSystem>();
    }

    Application::~Application() {

    }

    void Application::run() {
        auto context = getApplicationContext();

        context->windowManager->initialize(context);
        context->sceneManager->initialize(context);
        context->cameraSystem->initialize(context);
        context->uiManager->initialize(context);
        context->rendererSystem->initialize(context); // Renderer performs its specific setup
        context->uiManager->initGLFWBackend(); // Initialize ImGui GLFW backend
        context->inputManager->initialize(context);
        context->transformSystem->initialize(context);
        context->aabbSystem->initialize(context);

        initECS();

        loadPlugins(); // Init plugins after core systems are ready

        m_dispatcher.trigger<LoadModelEvent>({"models/star.obj"}); // Example load

        mainLoop();

        cleanup();
    }

    void Application::initECS() {
        Log::Info("Initializing ECS...");

        // Connect event listeners
        m_dispatcher.sink<WindowResizeEvent>().connect<&Application::onWindowResize>(this);
        m_dispatcher.sink<LoadModelEvent>().connect<&Application::onLoadModelRequest>(this);
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
            m_applicationContext.aabbSystem->update();
            m_applicationContext.transformSystem->update();

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
        auto vkContext = m_applicationContext.rendererSystem->getVulkanContext();
        if (vkContext->device != VK_NULL_HANDLE) {
            VK_CHECK(vkDeviceWaitIdle(vkContext->device));
        }
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
        m_applicationContext.windowManager->m_framebufferResized = true;
        // Update window dimensions
        m_applicationContext.windowManager->setWindowSize(event.width, event.height);

        // Update camera aspect ratio if dimensions are valid
        if (event.width > 0 && event.height > 0) {
            auto camera = m_applicationContext.cameraSystem->getCurrentCamera();
            camera->aspectRatio = (float) event.width / (float) event.height;
            camera->dirtyProjection = true;
            Log::Trace("Window resized: {}x{}, aspect ratio updated to: {}", event.width, event.height,
                       camera->aspectRatio);
        }
    }

    void Application::onLoadModelRequest(const LoadModelEvent &event) {
        // Delegate to the loading function
        entt::entity loadedEntity = m_applicationContext.sceneManager->loadModel(event.filepath);

        auto &transform = m_registry.emplace<TransformComponent>(loadedEntity);
        transform.position = event.initialPosition;
        transform.rotation = event.initialRot;
        transform.scale = event.initialScale;
        transform.dirty = true;



        if (loadedEntity == entt::null) {
            Log::Error("Model loading failed for '{}', cannot set focus.", event.filepath);
            return;
        }
    }
} // namespace Bcg
