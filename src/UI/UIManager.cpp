//
// Created by alex on 4/9/25.
//

#include <iostream>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "UIManager.h"
#include "Logger.h"
#include "CameraSystem.h"
#include "RendererSystem.h"
#include "SceneManager.h"
#include "Application.h"
#include "WindowManager.h"
#include "TransformComponent.h"
#include "UICameraComponent.h"

namespace Bcg {
    UIManager::~UIManager() {
        // Check if context exists before destroying (safety)

        if (ImGui::GetCurrentContext() != nullptr) {
            ImGui::DestroyContext();
            Log::Info("[UIManager] ImGui Context  destroyed");
        } else {
            Log::Warn("[UIManager] ImGui Context was already null.");
        }
    }

    void UIManager::initialize(ApplicationContext *context) {
        Log::Info("UIManager Initialized.");
        this->context = context;
        initImGui();
    }

    void UIManager::shutdown() {
        Log::Info("UIManager Shutdown.");
        Log::Info("[UIManager::shuttdown] ImGui GLFW backend Shutdown.");
        ImGui_ImplGlfw_Shutdown();
        // 4. Destroy ImGui Context AFTER Vulkan backend is shut down
        Log::Info("[UIManager::shuttdown] ImGui Context destroyed.");
        ImGui::DestroyContext();
    }

    void UIManager::initImGui() {
        Log::Info("[UIManager::initImGui] ImGui Context Initialize...");
        // --- ImGui Setup Context --- <<< ADD
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking (requires imgui_internal.h adjustments usually)
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewports
        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // When viewports are enabled, tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        // ImGuiStyle& style = ImGui::GetStyle();
        // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        // style.WindowRounding = 0.0f;
        // style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        // }
        // --- End ImGui Setup ---
    }

    void UIManager::initGLFWBackend() {
        // --- ImGui Setup Platform/Renderer Bindings --- <<< ADD
        Log::Info("[UIManager::initGLFWBackend] ImGui GLFW backend Initialize...");
        ImGui_ImplGlfw_InitForVulkan(context->windowManager->getGLFWHandle(), true); // true = install callbacks
        // ImGui Vulkan backend is initialized inside m_vkContext.init() now
        // --- End ImGui Bindings ---
    }

    void UIManager::beginFrame() {
        // --- Start ImGui Frame --- <<< ADD
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // --- End Start ImGui Frame ---
    }

    void UIManager::endFrame() {
        // --- Render ImGui --- <<< ADD (Prepares ImGui draw data)
        ImGui::Render();
        // --- End Render ImGui ---
    }

    void UIManager::recordDrawCommands(VkCommandBuffer commandBuffer) {
        // --- Render ImGui Draw Data --- <<< ADD (Records ImGui draw commands)
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        // --- End Render ImGui Draw Data ---
    }


    void UIManager::buildUI() {
        // Example ImGui Window

        // Uncomment to show the full ImGui demo window
        ImGui::ShowDemoWindow();

        ImGui::Begin("Controls"); // Create a window called "Controls"

        ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);


        if (ImGui::CollapsingHeader("Camera State", ImGuiTreeNodeFlags_DefaultOpen)) {
            // -- Projection --
            auto *camera = context->cameraSystem->getCurrentCamera();

            UICameraComponent(*camera);
        } // End CollapsingHeader


        if (ImGui::CollapsingHeader("Scene")) {
            // Example: Button to reload model
            if (ImGui::Button("Reload Star Model")) {
                std::cout << "Waiting for GPU idle before reloading model..." << std::endl;
                VK_CHECK(vkDeviceWaitIdle(context->rendererSystem->getVulkanContext()->device));
                std::cout << "GPU idle. Proceeding with reload." << std::endl;

                if (context->sceneManager) context->sceneManager->clearScene(); // Use SceneManager to clear
                context->cameraFocusEntity = entt::null; // Reset focus state in Application

                // Trigger load event
                context->dispatcher->trigger<LoadModelEvent>({"models/star.obj"}); // Assuming LoadModelEvent exists

                auto transform = context->registry->try_get<TransformComponent>(context->cameraFocusEntity);
                auto camera = context->cameraSystem->getCurrentCamera();
                camera->target = transform->position;
                camera->dirtyView = true;
            }
            // Add other scene controls here
        }

        // TODO: Add controls for lights, materials, entity selection/properties etc.

        ImGui::End(); // End the window
    }
}
