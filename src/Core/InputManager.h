//
// Created by alex on 4/9/25.
//

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <glm/glm.hpp>
#include "Manager.h"

namespace Bcg {
    struct InputState {
        bool m_mouseDragging = false;
        glm::dvec2 m_lastMousePos{0.0};
    };

    class InputManager : public Manager{
    public:
        InputManager();

        ~InputManager() override;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        void processInput(float detlaTime);

        InputState &getInputState();

        const InputState &getInputState() const;

        void handleKey(int key, int scancode, int action, int mods);

        void handleMouseButton(int button, int action, int mods);

        void handleCursorPos(double xpos, double ypos);

        void handleScroll(double xoffset, double yoffset);

        void handleChar(unsigned int c);

    private:
        InputState m_inputState;
    };
}

#endif //INPUTMANAGER_H
