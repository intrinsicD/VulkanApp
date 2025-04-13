//
// Created by alex on 4/9/25.
//

#ifndef RENDERER_H
#define RENDERER_H

#include "System.h"
#include "VulkanContext.h"
#include "ShaderData.h"

namespace Bcg{
    struct VulkanContext;
    // Very basic renderer structure. Could be expanded with multiple passes, materials etc.
    class RendererSystem : public System{
    public:
        ~RendererSystem() override;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        void drawFrame();

        VulkanContext *getVulkanContext();

        // Called by Application or Systems to upload data
        void uploadMesh(entt::entity entity, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

        // void uploadPointCloud(...) etc.

        // TODO: Add methods to register/manage multiple render passes and pipelines

    private:
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        void updateUniformBuffer(uint32_t currentImage); // Update global uniforms (camera)

        VulkanContext *m_vkContext;
    };
}

#endif //RENDERER_H
