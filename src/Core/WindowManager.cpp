//
// Created by alex on 4/9/25.
//

#include <stdexcept>
#include <iostream>
#include <utility>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui.h>


#include "WindowManager.h"
#include "Logger.h"
#include "Application.h"
#include "InputManager.h"
#include "ApplicationContext.h"


namespace Bcg {
    WindowManager::WindowManager(int width, int height, std::string title)
        : Manager(),  m_width(width), m_height(height), m_title(std::move(title)) {

    }

    WindowManager::~WindowManager() {
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        // GLFW termination should happen *after* all windows are destroyed.
        // Consider putting glfwTerminate() in the Application destructor or run() cleanup.
        // For simplicity here, we might put it here, assuming only one window.
        glfwTerminate();
        std::cout << "GLFW terminated." << std::endl;
    }

    void WindowManager::initialize(ApplicationContext *context)  {
        Log::Info("WindowManager Initialized.");
        this->context = context;
        initWindow();
    }

    void WindowManager::shutdown()  {
        Log::Info("WindowManager Shutdown.");
        this->context->windowManager.reset();
    }

    // --- Public Methods ---

    bool WindowManager::shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void WindowManager::pollEvents() {
        glfwPollEvents();
    }

    // --- Private Methods ---

    void WindowManager::initWindow() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
        Log::Info("GLFW Initialized.");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create OpenGL context

        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
        Log::Info("GLFW Window Created ({}x{}, '{}').", m_width, m_height, m_title);

        // Store pointer to OUR event context (Application instance) for callbacks
        glfwSetWindowUserPointer(m_window, context);

        // Set GLFW static callbacks
        glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
        glfwSetKeyCallback(m_window, keyCallback);
        glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
        glfwSetCursorPosCallback(m_window, cursorPosCallback);
        glfwSetScrollCallback(m_window, scrollCallback);
        glfwSetCharCallback(m_window, charCallback); // Needed for ImGui text
    }

    void WindowManager::setWindowSize(int width, int height) {
        m_width = width;
        m_height = height;
        glfwSetWindowSize(m_window, width, height);
    }


    // --- Static Callback Implementations ---
    // These functions retrieve the Application pointer and forward the event

    void WindowManager::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
        Log::Trace("[WindowManager::framebufferResizeCallback] {}x{}", width, height);
        auto context = reinterpret_cast<ApplicationContext *>(glfwGetWindowUserPointer(window));
        if (context) {
            // Option 1: Call a specific context method in Application
            // context->handleFramebufferResize(width, height);

            // Option 2: Use the existing event dispatcher (more flexible)
            context->dispatcher->trigger(WindowResizeEvent{width, height});
        }
    }

    void WindowManager::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        Log::Trace("[WindowManager::keyCallback] Key: {}", key);
        // Always forward to ImGui first if it's initialized
        if (ImGui::GetCurrentContext() != nullptr) {
            // Ensure ImGui is setup
            //ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
            ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureKeyboard) {
                return; // ImGui handled it
            }
        }

        // Forward to application if ImGui didn't handle it
        auto context = reinterpret_cast<ApplicationContext *>(glfwGetWindowUserPointer(window));
        if (context) {
            context->inputManager->handleKey(key, scancode, action, mods); // Call a new context method in Application
        }
    }

    void WindowManager::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
        Log::Trace("[WindowManager::mouseButtonCallback] Button: {}, Action: {}, Mods: {}", button, action, mods);
        if (ImGui::GetCurrentContext() != nullptr) {
            //ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
            ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureMouse) {
                return;
            }
        }

        auto context = reinterpret_cast<ApplicationContext *>(glfwGetWindowUserPointer(window));
        if (context) {
            context->inputManager->handleMouseButton(button, action, mods);
        }
    }

    void WindowManager::cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
        Log::Trace("[WindowManager::cursorPosCallback] ({}, {})", xpos, ypos);
        // ImGui captures this implicitly via NewFrame reading mouse state
        if (ImGui::GetCurrentContext() != nullptr) {
            ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureMouse) {
                return;
            }
        }

        auto context = reinterpret_cast<ApplicationContext *>(glfwGetWindowUserPointer(window));
        if (context) {
            context->inputManager->handleCursorPos(xpos, ypos);
        }
    }

    void WindowManager::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
        Log::Trace("[WindowManager::scrollCallback] ({}, {})", xoffset, yoffset);
        if (ImGui::GetCurrentContext() != nullptr) {
            //ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
            ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureMouse) {
                return;
            }
        }

        auto context = reinterpret_cast<ApplicationContext *>(glfwGetWindowUserPointer(window));
        if (context) {
            context->inputManager->handleScroll(xoffset, yoffset);
        }
    }

    void WindowManager::charCallback(GLFWwindow *window, unsigned int c) {
        Log::Trace("[WindowManager::charCallback] {}", c);
        if (ImGui::GetCurrentContext() != nullptr) {
            //ImGui_ImplGlfw_CharCallback(window, c);
            ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureKeyboard) {
                return;
            }
        }

        auto context = reinterpret_cast<ApplicationContext *>(glfwGetWindowUserPointer(window));
        if (context) {
            context->inputManager->handleChar(c);
        }
    }
}
