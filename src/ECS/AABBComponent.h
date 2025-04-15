//
// Created by alex on 4/16/25.
//

#ifndef AABBCOMPONENT_H
#define AABBCOMPONENT_H

#include "MatVec.h"

namespace Bcg {
    struct AABBComponent {
        Vector3f min = Vector3f::Constant(std::numeric_limits<float>::max());
        Vector3f max = Vector3f::Constant(std::numeric_limits<float>::lowest());
        bool dirty = false;
    };
}

#endif //AABBCOMPONENT_H
