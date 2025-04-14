//
// Created by alex on 4/9/25.
//

#include <GLFW/glfw3.h>

#include "InputManager.h"
#include "CameraSystem.h"
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

    InputState &InputManager::getInputState() {
        return m_inputState;
    }

    const InputState &InputManager::getInputState() const {
        return m_inputState;
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
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                m_inputState.m_mouseDragging = true;
                // Need to get cursor pos differently now
                double x, y;
                glfwGetCursorPos(context->windowManager->getGLFWHandle(), &x, &y);
                m_inputState.m_lastMousePos = {x, y};
            } else if (action == GLFW_RELEASE) {
                m_inputState.m_mouseDragging = false;
            }
        }
    }

    void InputManager::handleCursorPos(double xpos, double ypos) {
        if (m_inputState.m_mouseDragging) {
            Vector2f currentMousePos = Vector2f(xpos, ypos);
            Vector2f delta = currentMousePos - m_inputState.m_lastMousePos;
            m_inputState.m_lastMousePos = currentMousePos;

            float rotSensitivity = 0.005f;
            auto camera = context->cameraSystem->getCurrentCamera();
            if (!camera) return; // Safety check

            // Calculate rotation angles (in radians)
            float angle_x = delta.x() * rotSensitivity;
            float angle_y = delta.y() * rotSensitivity;

            // Compute vector from target to current camera position
            Vector3f v = camera->position - camera->target;
            Vector3f worldUp = Vector3f(0.0f, 1.0f, 0.0f);

            // --- Horizontal Rotation ---
            // Rotate around the world up (Y) axis.
            Eigen::AngleAxisf horiz_rot(angle_x, worldUp);
            v = horiz_rot * v;
            // Also rotate the current up vector
            Vector3f new_up = horiz_rot * camera->up;

            // --- Compute the Right Axis for Vertical Rotation ---
            // First, compute forward direction from camera toward the target.
            // Since v = camera->position - camera->target, the forward vector is -v (normalized).
            Vector3f forward = (-v).normalized();
            // Compute right vector using the conventional lookAt ordering:
            // right = normalize(cross(forward, new_up));
            Vector3f right = forward.cross(new_up).normalized();

            // --- Vertical Rotation ---
            // Rotate v and new_up about the right axis.
            Eigen::AngleAxisf vert_rot(angle_y, right);
            v = vert_rot * v;
            new_up = vert_rot * new_up;

            // Update camera parameters
            camera->position = camera->target + v;
            camera->up = new_up.normalized();

            // Mark the view matrix as needing update (if applicable).
            camera->dirtyView = true;
        }
    }

    void InputManager::handleScroll(double xoffset, double yoffset) {
        float zoomSensitivity = 0.5f;
        auto camera = context->cameraSystem->getCurrentCamera();
        if (!camera) return; // Safety check
        // Adjust the camera distance based on scroll input
        auto backward = camera->position - camera->target;
        auto len = backward.norm();
        if (len < 0.1f) {
            // Prevent zooming in too close
            return;
        }
        camera->position = camera->target + (len - static_cast<float>(yoffset)) * zoomSensitivity * (backward / len);
        camera->dirtyView = true;
    }

    void InputManager::handleChar(unsigned int c) {
    }
}
