//
// Created by alex on 4/9/25.
//

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "MatVec.h"

#include "VulkanUtils.h"

namespace Bcg {
    // Tag component to mark the entity the camera focuses on
    struct CameraFocusTarget {
    };

    // Tag component to mark entities needing GPU buffer updates
    struct DirtyGPUResource {
    };
}

#endif //COMPONENTS_H
