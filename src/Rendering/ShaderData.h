//
// Created by alex on 4/9/25.
//

#ifndef SHADERDATA_H
#define SHADERDATA_H

#include <vulkan/vulkan.h>
#include "MatVec.h"

namespace Bcg{
    // Structure for Global Uniform Buffer Object
    struct GlobalUBO {
        Matrix4f view;
        Matrix4f proj;
        // Add light direction, camera position etc. if needed
        Vector4f lightDir = Vector4f(0.5f, -1.0f, 0.3f, 0.0f); // w=0 for directional
        Vector4f cameraPos;
    };

    struct Vertex {
        Vector3f pos;
        Vector3f normal;
        Vector2f texCoord;
        Vector3f color; // Optional: can be derived or default

        static VkVertexInputBindingDescription getBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();

        bool operator==(const Vertex &other) const {
            return pos == other.pos && normal == other.normal && texCoord == other.texCoord && color == other.color;
        }
    };
}// namespace Bcg

namespace std {
    template<>
    struct hash<Bcg::Vector3f> {
        size_t operator()(const Bcg::Vector3f &vec) const noexcept {
            size_t seed = 0;
            for (int i = 0; i < vec.size(); ++i) {
                seed ^= std::hash<float>()(vec[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };

    template<>
    struct hash<Bcg::Vector2f> {
        size_t operator()(const Bcg::Vector2f &vec) const noexcept {
            size_t seed = 0;
            for (int i = 0; i < vec.size(); ++i) {
                seed ^= std::hash<float>()(vec[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };

    template<>
    struct hash<Bcg::Vertex> {
        size_t operator()(Bcg::Vertex const &vertex) const noexcept {
            size_t h1 = hash<Bcg::Vector3f>()(vertex.pos);
            size_t h2 = hash<Bcg::Vector3f>()(vertex.normal);
            size_t h3 = hash<Bcg::Vector2f>()(vertex.texCoord);
            size_t h4 = hash<Bcg::Vector3f>()(vertex.color);
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
