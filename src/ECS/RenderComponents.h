//
// Created by alex on 4/9/25.
//

#ifndef RENDERCOMPONENTS_H
#define RENDERCOMPONENTS_H

#include "VulkanUtils.h"

namespace Bcg{

    struct VulkanMeshComponent {
        AllocatedBuffer vertexBuffer;
        AllocatedBuffer indexBuffer;
        uint32_t indexCount = 0;
        uint32_t vertexCount = 0;
        // Material ID / reference could go here
    };

    struct PointCloudComponent {
        // Example for another render type
        AllocatedBuffer vertexBuffer; // Only positions, maybe colors
        uint32_t pointCount = 0;
        // ... potentially other attributes
    };
}
#endif //RENDERCOMPONENTS_H
