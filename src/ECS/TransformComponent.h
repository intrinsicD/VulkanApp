//
// Created by alex on 4/9/25.
//

#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include "MatVec.h"

namespace Bcg {
    struct TransformComponent {
        Vector3f position = Vector3f::Zero();
        Vector3f rotation = Vector3f::Zero();
        Vector3f scale = Vector3f::Zero();
        Matrix4f modelMatrix = Matrix4f::Identity();
        bool dirty = true;
    };
}

#endif //TRANSFORMCOMPONENT_H
