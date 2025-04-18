//
// Created by alex on 4/9/25.
//

#include <GLFW/glfw3.h>

#include "InputManager.h"
#include "CameraSystem.h"
#include "CameraUtils.h"
#include "Application.h"
#include "WindowManager.h"

namespace Bcg {
    InputManager::InputManager() : Manager() {

    }

    InputManager::~InputManager() {
    }

    void InputManager::initialize(ApplicationContext *context)  {
        this->context = context;
    }

    void InputManager::shutdown() {
        context->inputManager.reset();
    }

    void InputManager::processInput(float deltaTime) {
        // Reset movement flag at the start of input processing
        auto camera = context->cameraSystem->getCurrentCamera();
        if (!camera) return;
        bool moved = false;


        // Calculate forward and right directions in the XZ plane based on yaw
        // Forward vector (along camera's look direction projected onto XZ)
        Vector3f forwardDirXZ = (camera->target - camera->position).normalized();
        // Right vector (perpendicular to forward in XZ plane)
        Vector3f rightDirXZ = (forwardDirXZ.cross(camera->up)).normalized();

        Vector3f moveDirection = Vector3f::Zero(); // Accumulate movement

        // Check WASD keys
        auto h_window = context->windowManager->getGLFWHandle();
        if (glfwGetKey(h_window, GLFW_KEY_W) == GLFW_PRESS) {
            moveDirection += forwardDirXZ;
            moved = true;
        }
        if (glfwGetKey(h_window, GLFW_KEY_S) == GLFW_PRESS) {
            moveDirection -= forwardDirXZ;
            moved = true;
        }
        if (glfwGetKey(h_window, GLFW_KEY_A) == GLFW_PRESS) {
            moveDirection -= rightDirXZ; // Move left
            moved = true;
        }
        if (glfwGetKey(h_window, GLFW_KEY_D) == GLFW_PRESS) {
            moveDirection += rightDirXZ; // Move right
            moved = true;
        }

        // If any movement key was pressed, update the target position
        if (moved) {
            // Normalize the combined direction if moving diagonally
            if (moveDirection.norm() > 0.1f) {
                // Avoid normalizing zero vector
                moveDirection = moveDirection.normalized();
            }

            camera->position += moveDirection * deltaTime * camera->movementSpeed;
            camera->target += moveDirection * deltaTime * camera->movementSpeed;

            // Optional: If WASD moves, explicitly remove the focus target entity association
            // This makes WASD permanently break the lock until a new focus is set.
            // if (m_cameraFocusEntity != entt::null) {
            //    std::cout << "Camera focus lock broken by WASD movement." << std::endl;
            //    m_registry.remove<CameraFocusTarget>(m_cameraFocusEntity); // Remove tag
            //    m_cameraFocusEntity = entt::null;
            // }
        }
    }

    const Mouse &InputManager::getMouse() const {
        return m_mouse;
    }

    void InputManager::handleKey(int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            // Need a way to signal shutdown, maybe:
            // m_windowManager->setShouldClose(true); // If Window class has such a method
            // Or set an internal flag:
            // m_shouldQuit = true;
            glfwSetWindowShouldClose(context->windowManager->getGLFWHandle(), GLFW_TRUE); // Access via Window object
        }
        if (key == GLFW_KEY_L && action == GLFW_PRESS) {
            static int model_idx = 0;
            std::string models[] = {"models/star.obj", "models/suzanne.obj", "models/sphere.obj"};
            int num_models = sizeof(models) / sizeof(models[0]);
            context->dispatcher->trigger<LoadModelEvent>({models[model_idx % num_models]});
            model_idx++;
        }
        // Add other key handling
    }

    void InputManager::handleMouseButton(int button, int action, int mods) {
        m_mouse.is_left_button_pressed = button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS;
        m_mouse.is_middle_button_pressed = button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS;
        m_mouse.is_right_button_pressed = button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS;
        if (m_mouse.is_left_button_pressed || m_mouse.is_middle_button_pressed || m_mouse.is_right_button_pressed) {
            m_mouse.is_dragging = true;
        } else {
            m_mouse.is_dragging = false;
        }

        if (m_mouse.is_left_button_pressed) {
            m_mouse.last_left_click = m_mouse.current;
        }

        if (m_mouse.is_middle_button_pressed) {
            m_mouse.last_middle_click = m_mouse.current;
        }

        if (m_mouse.is_right_button_pressed) {
            m_mouse.last_right_click = m_mouse.current;
        }
    }

    void InputManager::handleCursorPos(double xpos, double ypos) {
        m_mouse.is_moving = true;
        m_mouse.is_idle = false;
        m_mouse.current.cursor_position = Vector2f(xpos, ypos);

        if (m_mouse.is_dragging) {
            auto camera = context->cameraSystem->getCurrentCamera();
            CameraUtils::arcball(*camera, m_mouse);
        }
    }

    void InputManager::handleScroll(double xoffset, double yoffset) {
        m_mouse.is_scrolling = true;
        m_mouse.is_idle = false;
        m_mouse.scrollxy = Vector2f(xoffset, yoffset);

        auto camera = context->cameraSystem->getCurrentCamera();
        if (camera) CameraUtils::zoom(*camera, yoffset);
    }

    void InputManager::handleChar(unsigned int c) {
    }
}
