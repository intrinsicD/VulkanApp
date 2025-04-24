//
// Created by alex on 4/18/25.
//

#ifndef GEOMETRYACCESSCOMPONENT_H
#define GEOMETRYACCESSCOMPONENT_H

#include <vector>
#include <MatVec.h>

namespace Bcg {
    struct GeometryVertexPositionsComponent {
        std::vector<Vector3f> *positions = nullptr;
    };

    struct GeometryVertexNormalsComponent {
        std::vector<Vector3f> *normals = nullptr;
    };
}
#endif //GEOMETRYACCESSCOMPONENT_H
