//
// Created by alex on 4/9/25.
//

#ifndef SHADERDATA_H
#define SHADERDATA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan depth range is [0, 1]
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>

namespace Bcg{
    // Structure for Global Uniform Buffer Object
    struct GlobalUBO {
        glm::mat4 view;
        glm::mat4 proj;
        // Add light direction, camera position etc. if needed
        glm::vec4 lightDir{0.5f, -1.0f, 0.3f, 0.0f}; // w=0 for directional
        glm::vec4 cameraPos;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec3 color; // Optional: can be derived or default

        static VkVertexInputBindingDescription getBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();

        bool operator==(const Vertex &other) const {
            return pos == other.pos && normal == other.normal && texCoord == other.texCoord && color == other.color;
        }
    };


}

namespace std {
    template<>
    struct hash<Bcg::Vertex> {
        size_t operator()(Bcg::Vertex const &vertex) const noexcept {
            size_t h1 = hash<glm::vec3>()(vertex.pos);
            size_t h2 = hash<glm::vec3>()(vertex.normal);
            size_t h3 = hash<glm::vec2>()(vertex.texCoord);
            size_t h4 = hash<glm::vec3>()(vertex.color);
            // Combine hashes (boost::hash_combine style)
            size_t seed = 0;
            seed ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
} // namespace std
#endif //SHADERDATA_H
