//
// Created by alex on 4/9/25.
//

#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <string>

#include "Manager.h"

struct GLFWwindow;

namespace Bcg{
    class WindowManager : public Manager{
    public:
        // Constructor takes initial dimensions, title, and a pointer to the
        // object that will handle events (in this case, Application).
        WindowManager(int width, int height, std::string  title);

        ~WindowManager() override;

        // Prevent copying/assignment
        WindowManager(const WindowManager&) = delete;
        WindowManager& operator=(const WindowManager&) = delete;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        // Core functionality
        bool shouldClose() const;
        static void pollEvents();

        // Accessors
        GLFWwindow* getGLFWHandle() const { return m_window; }
        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }

        // --- Static Callbacks (Implementation will be in WindowManager.cpp) ---
        // These are the functions GLFW will call directly.
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void charCallback(GLFWwindow* window, unsigned int c); // Optional but needed for ImGui text input

        bool m_framebufferResized = false;
    private:
        void initWindow(); // Private helper for constructor

        GLFWwindow* m_window = nullptr;
        int m_width;
        int m_height;
        std::string m_title;
    };
}

#endif //WINDOWMANAGER_H
