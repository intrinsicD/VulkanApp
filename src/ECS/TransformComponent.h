//
// Created by alex on 4/9/25.
//

#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include "MatVec.h"

namespace Bcg {
    struct TransformComponent {
        Translation position = Translation::Identity();
        Rotation rotation = Rotation::Identity();
        Sscaling scale = Vector3f::Zero();

        Matrix4f cachedModelMatrix = Matrix4f::Identity();
        bool dirty = true;
    };
}

#endif //TRANSFORMCOMPONENT_H
