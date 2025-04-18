//
// Created by alex on 4/18/25.
//

#ifndef GEOMETRYPOSITIONSCOMPONENT_H
#define GEOMETRYPOSITIONSCOMPONENT_H

#include <vector>
#include <MatVec.h>

namespace Bcg {
    struct GeometryPositionsComponent {
        std::vector<Vector3f> *positions = nullptr;
    };
}
#endif //GEOMETRYPOSITIONSCOMPONENT_H
