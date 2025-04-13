//
// Created by alex on 4/9/25.
//

#include "ShaderData.h"

namespace Bcg {

    VkVertexInputBindingDescription Vertex::getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0; // Binding index
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Per-vertex data
        return bindingDescription;
    }

    std::array<VkVertexInputAttributeDescription, 4> Vertex::getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        // Position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0; // layout(location = 0) in shader
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        // Normal
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1; // layout(location = 1) in shader
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        // Texture Coordinates
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2; // layout(location = 2) in shader
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        // Color
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3; // layout(location = 3) in shader
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[3].offset = offsetof(Vertex, color);


        return attributeDescriptions;
    }
}