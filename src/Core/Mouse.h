//
// Created by alex on 4/10/25.
//

#ifndef MOUSECONTEXT_H
#define MOUSECONTEXT_H

#include "MatVec.h"

namespace Bcg {
    struct Mouse {
        struct Position {
            Vector2f cursor_position; //<< screen coordinates
            Vector3f world_position; //<< world coordinates using depth buffer
            Vector3f model_position; //<< model coordinates on the selected object
        };
        Position current;
        Position last_left_click;
        Position last_middle_click;
        Position last_right_click;
        Vector2f scrollxy;

        bool is_left_button_pressed = false;
        bool is_right_button_pressed = false;
        bool is_middle_button_pressed = false;

        bool is_dragging = false;
        bool is_moving = false;
        bool is_scrolling = false;
        bool is_idle = false;
    };
}

#endif //MOUSECONTEXT_H
