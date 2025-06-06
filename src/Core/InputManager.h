//
// Created by alex on 4/9/25.
//

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "Manager.h"
#include "Mouse.h"
#include "MatVec.h"

namespace Bcg {
    class InputManager : public Manager{
    public:
        InputManager();

        ~InputManager() override;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        void processInput(float detlaTime);

        [[nodiscard]] const Mouse &getMouse() const;

        void handleKey(int key, int scancode, int action, int mods);

        void handleMouseButton(int button, int action, int mods);

        void handleCursorPos(double xpos, double ypos);

        void handleScroll(double xoffset, double yoffset);

        void handleChar(unsigned int c);

    private:
        Mouse m_mouse;
    };
}

#endif //INPUTMANAGER_H
