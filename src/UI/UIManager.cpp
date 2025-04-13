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

namespace Bcg {
    UIManager::~UIManager() {
        // Check if context exists before destroying (safety)

        if (ImGui::GetCurrentContext() != nullptr) {
            ImGui::DestroyContext();
            std::cout << "  ImGui Context destroyed." << std::endl;
        } else {
            std::cout << "  ImGui Context was already null." << std::endl;
        }
    }

    void UIManager::initialize(ApplicationContext *context) {
        Log::Info("WindowManager Initialized.");
        this->context = context;
        initImGui();
    }

    void UIManager::shutdown() {
        Log::Info("WindowManager Shutdown.");
        Log::Info("[WindowManager::shuttdown] ImGui GLFW backend Shutdown.");
        ImGui_ImplGlfw_Shutdown();
        // 4. Destroy ImGui Context AFTER Vulkan backend is shut down
        Log::Info("[WindowManager::shuttdown] ImGui Context destroyed.");
        ImGui::DestroyContext();
    }

    void UIManager::initImGui() {
        Log::Info("[WindowManager::initImGui] ImGui Context Initialize...");
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
        Log::Info("[WindowManager::initGLFWBackend] ImGui GLFW backend Initialize...");
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

            ImGui::SeparatorText("Projection");
            float fovDeg = camera->fovYDegrees;
            float aspect = camera->aspectRatio;
            float nearP = camera->nearPlane;
            float farP = camera->farPlane;

            bool projChanged = false;
            projChanged |= ImGui::SliderFloat("FOV (Degrees)", &fovDeg, 10.0f, 120.0f);
            // Display Aspect Ratio (usually not editable directly here)
            ImGui::Text("Aspect Ratio: %.2f", aspect);
            // Allow editing near/far planes
            projChanged |= ImGui::DragFloat("Near Plane", &nearP, 0.01f, 0.001f, farP - 0.01f, "%.3f"); // Add limits
            projChanged |= ImGui::DragFloat("Far Plane", &farP, 0.1f, nearP + 0.01f, 10000.0f, "%.1f"); // Add limits

            if (projChanged) {
                // Ensure near < far after edits
                nearP = glm::min(nearP, farP - 0.001f);
                farP = glm::max(farP, nearP + 0.001f);
                camera->fovYDegrees = fovDeg;
                camera->aspectRatio = aspect;
                camera->nearPlane = nearP;
                camera->farPlane = farP;
                camera->dirtyProjection = true;
            }
            // Display Projection Matrix (Read-only, potentially long)
            // const glm::mat4 projMat = cameraSystem->getProjectionMatrix(); // Force update if needed
            // ImGui::Text("Projection Matrix:");
            // ImGui::Text("%s", glm::to_string(projMat).c_str()); // Can be verbose

            // -- View (Orbit Mode Focus) --
            ImGui::SeparatorText("View (Orbit)");
            float distance = camera->distance;
            glm::vec3 position = camera->position; // Get a copy
            glm::vec3 targetPos = camera->target; // Get a copy

            bool viewChanged = false;
            viewChanged |= ImGui::DragFloat("Distance", &distance, 0.1f, 0.1f, 1000.0f, "%.2f");
            viewChanged |= ImGui::InputFloat3("Target Position", &targetPos[0], "%.2f"); // Edit target position

            if (viewChanged) {
                if (distance != camera->distance) {
                    context->cameraSystem->setDistance(*camera, distance);
                }
                camera->target = targetPos;
                camera->dirtyView = true;
            }

            // Display Calculated Camera Position (Read-only)
            glm::vec3 camPos = camera->position; // Force update if needed
            ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);

            // Display View Matrix (Read-only, potentially long)
            // const glm::mat4 viewMat = cameraSystem->getViewMatrix(); // Force update if needed
            // ImGui::Text("View Matrix:");
            // ImGui::Text("%s", glm::to_string(viewMat).c_str()); // Can be verbose

            // -- Movement --
            ImGui::SeparatorText("Movement");
            float moveSpeed = camera->movementSpeed;
            if (ImGui::InputFloat("WASD Move Speed", &moveSpeed, 0.1f, 1.0f, "%.1f")) {
                camera->movementSpeed = moveSpeed;
            }
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
