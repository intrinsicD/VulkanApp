//
// Created by alex on 4/9/25.
//

#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <string>
#include <entt/entt.hpp> // Include EnTT registry
#include <glm/glm.hpp>

#include "Manager.h"

namespace Bcg{
    class RendererSystem; // Needs Renderer to upload mesh data
    struct LoadModelEvent; // If handling the event directly

    class SceneManager : public Manager{
    public:
        // Constructor can optionally take initial registry/renderer references
        SceneManager() = default; // Recommended: Pass registry by reference

        ~SceneManager() override = default; // Might handle scene cleanup if needed

        // Prevent copying
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        // --- Scene Operations ---
        entt::entity loadModel(const std::string& filepath,
                               glm::vec3 position = glm::vec3(0.0f),
                               glm::vec3 scale = glm::vec3(1.0f));

        void clearScene(); // Destroys all entities and their GPU resources

        // --- Optional Future Additions ---
        // void saveScene(const std::string& filepath);
        // void loadScene(const std::string& filepath);
        // entt::entity findEntityByName(const std::string& name); // Requires NameComponent
        // std::vector<entt::entity> findEntitiesWithComponent<T>();

        // --- Event Handling (Alternative to Application handling it) ---
        // void onLoadModelRequest(const LoadModelEvent& event);

    private:
        friend class Application;

        bool calculateWorldBounds(entt::entity entity, glm::vec3& outMin, glm::vec3& outMax);
    };
}

#endif //SCENEMANAGER_H
