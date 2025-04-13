//
// Created by alex on 4/9/25.
//

#include <iostream>
#include <limits> // For numeric_limits
#include <unordered_map> // For vertex deduplication

// Include TinyObjLoader implementation detail ONLY here if not done elsewhere
#define TINYOBJLOADER_IMPLEMENTATION // Should be defined once, e.g., in Application.cpp
#include <tiny_obj_loader.h>

#include "SceneManager.h"
#include "Logger.h"
#include "RendererSystem.h" // Include Renderer definition
#include "RenderComponents.h" // Include Renderer definition
#include "ShaderData.h" // For Vertex struct definition
#include "Components.h" // Assuming components like TransformComponent are here
#include "VulkanContext.h" // For VulkanMeshComponent
#include "Application.h" // Potentially needed to get CameraSystem, or use events

namespace Bcg {
    void SceneManager::initialize(ApplicationContext *context) {
        Log::Info("SceneManager Initialized.");
        this->context = context;
    }

    void SceneManager::shutdown() {
        Log::Info("SceneManager Shutdown.");
        this->context->sceneManager.reset();
    }

    // --- Scene Operations ---

    entt::entity SceneManager::loadModel(const std::string &filepath, glm::vec3 position, glm::vec3 scale) {
        if (!context->rendererSystem) {
            Log::Error( "[SceneManager::loadModel] Renderer not set! Cannot load model.");
            return entt::null;
        }

        Log::Info("[SceneManager::loadModel] Loading model...");

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        std::string dir = filepath.substr(0, filepath.find_last_of("/\\") + 1);

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), dir.c_str())) {
            Log::Error("[SceneManager::loadModel::TinyObjLoader::Warning] {}", warn);
            Log::Error("[SceneManager::loadModel::TinyObjLoader::Error] {}", err);
            Log::Error("[SceneManager::loadModel::TinyObjLoader] Failed to load model {}", filepath);
            return entt::null;
        }
        if (!warn.empty()) {
            Log::Error("[SceneManager::loadModel::TinyObjLoader::Warning] {}", warn);
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        glm::vec3 modelMinLocal(std::numeric_limits<float>::max());
        glm::vec3 modelMaxLocal(std::numeric_limits<float>::lowest());
        bool hasVertices = false;

        for (const auto &shape: shapes) {
            for (const auto &index: shape.mesh.indices) {
                Vertex vertex{};
                int vertIdxBase = 3 * index.vertex_index;
                if (vertIdxBase + 2 >= attrib.vertices.size()) continue; // Basic bounds check

                vertex.pos = {
                    attrib.vertices[vertIdxBase + 0],
                    attrib.vertices[vertIdxBase + 1],
                    attrib.vertices[vertIdxBase + 2]
                };
                modelMinLocal = glm::min(modelMinLocal, vertex.pos);
                modelMaxLocal = glm::max(modelMaxLocal, vertex.pos);
                hasVertices = true;

                // Normals
                if (index.normal_index >= 0) {
                    int normIdxBase = 3 * index.normal_index;
                    if (normIdxBase + 2 < attrib.normals.size()) {
                        // Bounds check
                        vertex.normal = {
                            attrib.normals[normIdxBase + 0],
                            attrib.normals[normIdxBase + 1],
                            attrib.normals[normIdxBase + 2]
                        };
                    } else { vertex.normal = {0.0f, 1.0f, 0.0f}; }
                } else {
                    vertex.normal = {0.0f, 1.0f, 0.0f};
                }

                // TexCoords
                if (index.texcoord_index >= 0) {
                    int tcIdxBase = 2 * index.texcoord_index;
                    if (tcIdxBase + 1 < attrib.texcoords.size()) {
                        // Bounds check
                        vertex.texCoord = {
                            attrib.texcoords[tcIdxBase + 0],
                            1.0f - attrib.texcoords[tcIdxBase + 1] // Flip Y
                        };
                    } else { vertex.texCoord = {0.0f, 0.0f}; }
                } else {
                    vertex.texCoord = {0.0f, 0.0f};
                }

                // Colors
                if (!attrib.colors.empty() && vertIdxBase + 2 < attrib.colors.size()) {
                    // Bounds check
                    vertex.color = {
                        attrib.colors[vertIdxBase + 0],
                        attrib.colors[vertIdxBase + 1],
                        attrib.colors[vertIdxBase + 2]
                    };
                } else {
                    vertex.color = {1.0f, 1.0f, 1.0f};
                }

                // Deduplicate
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }

        if (!hasVertices) {
            Log::Warn( "[SceneManager::loadModel] Model has no vertices: {}", filepath);
            return entt::null;
        }

        // --- Create Entity and Components ---
        auto entity = context->registry->create();
        auto &transform = context->registry->emplace<TransformComponent>(entity);
        transform.position = position;
        transform.scale = scale;
        transform.updateModelMatrix();

        // Add mesh component and upload data via Renderer
        context->registry->emplace<VulkanMeshComponent>(entity);
        context->rendererSystem->uploadMesh(entity, vertices, indices);

        context->cameraFocusEntity = entity; // Set focus to the new model

        // Optional: Store local bounds in a component for later use
        // m_registry.emplace<MeshBoundsComponent>(entity, modelMinLocal, modelMaxLocal);
        Log::Info("[SceneManager::loadModel] Model has been updated and entity created: {}",(uint32_t) entity);

        // --- Trigger Camera Framing (or return bounds) ---
        // Option 1: If SceneManager knows about CameraSystem (less ideal coupling)
        // if (m_cameraSystem) { // Requires m_cameraSystem pointer
        //      if (!m_registry.view<CameraFocusTarget>().size()) { // Check if no focus target yet
        //           m_registry.emplace<CameraFocusTarget>(entity);
        //           glm::vec3 worldMin, worldMax;
        //           if (calculateWorldBounds(entity, worldMin, worldMax)) {
        //                m_cameraSystem->frameBoundingBox(worldMin, worldMax);
        //                m_cameraSystem->setTarget(transform.position); // Center on object origin initially
        //           }
        //      }
        // }
        // Option 2: Return bounds or emit an event for Application/CameraSystem to handle framing
        // We'll assume Application still handles the focus logic for now

        return entity;
    }

    void SceneManager::clearScene() {
        Log::Info("[SceneManager::clearScene]: Clearing scene...");

        // Need Vulkan device access, usually via Renderer or VulkanContext
        // If Renderer is guaranteed to be valid here:
        if (!context->rendererSystem || !context->rendererSystem->getVulkanContext()) {
            Log::Warn("[SceneManager::clearScene]: Cannot clear GPU resources without Renderer/VulkanContext.");
        } else {
            VkDevice device = context->rendererSystem->getVulkanContext()->device; // Assuming getter exists
            auto view = context->registry->view<VulkanMeshComponent>();
            for (auto entity: view) {
                auto &meshComp = view.get<VulkanMeshComponent>(entity);
                meshComp.vertexBuffer.destroy(device);
                meshComp.indexBuffer.destroy(device);
                // Also destroy texture resources if managed per-entity
            }
        }

        // Destroy all entities in the registry
        context->registry->clear();
    }


    // Helper to calculate world bounds (needed for framing)
    bool SceneManager::calculateWorldBounds(entt::entity entity, glm::vec3 &outMin, glm::vec3 &outMax) {
        auto *transform = context->registry->try_get<TransformComponent>(entity);
        // We'd need the local bounds, maybe from a MeshBoundsComponent:
        // auto* bounds = m_registry.try_get<MeshBoundsComponent>(entity);
        // if (!transform || !bounds) return false;

        // Placeholder: Needs local bounds to be stored/accessed
        // For now, just use the transform position as a proxy (INACCURATE!)
        if (!transform) return false;
        outMin = transform->position - glm::vec3(0.5f * transform->scale); // Rough guess
        outMax = transform->position + glm::vec3(0.5f * transform->scale); // Rough guess
        return true;


        /* // Correct implementation using stored local bounds:
        if (!transform || !bounds) return false;

        glm::vec3 localCorners[8];
        localCorners[0] = glm::vec3(bounds->localMin.x, bounds->localMin.y, bounds->localMin.z);
        // ... calculate other 7 corners ...
        localCorners[7] = glm::vec3(bounds->localMax.x, bounds->localMax.y, bounds->localMax.z);

        outMin = glm::vec3(std::numeric_limits<float>::max());
        outMax = glm::vec3(std::numeric_limits<float>::lowest());

        for (int i = 0; i < 8; ++i) {
            glm::vec4 transformedCorner = transform->model * glm::vec4(localCorners[i], 1.0f);
            outMin = glm::min(outMin, glm::vec3(transformedCorner));
            outMax = glm::max(outMax, glm::vec3(transformedCorner));
        }
        return true;
        */
    }


    // --- Event Handling ---
    // void SceneManager::onLoadModelRequest(const LoadModelEvent& event) {
    //     loadModel(event.filepath, event.initialPosition, event.initialScale);
    // }
} // namespace Bcg
