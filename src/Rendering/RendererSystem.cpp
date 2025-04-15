//
// Created by alex on 4/9/25.
//

#include <iostream>

#include "imgui.h"

#include "RendererSystem.h"
#include "Logger.h"
#include "CameraSystem.h"
#include "WindowManager.h"
#include "RenderComponents.h"
#include "ShaderData.h"
#include "UIManager.h"
#include "TransformComponent.h"
#include "entt/entity/registry.hpp"


namespace Bcg{
// --- Renderer Method Implementations ---

    RendererSystem::~RendererSystem() {
        // Shutdown called externally
    }

    void RendererSystem::initialize(ApplicationContext *context) {
        this->context = context;
        m_vkContext = &context->registry->ctx().emplace<VulkanContext>();
        // Renderer initialization (if any needed beyond VulkanContext)
        // Example: Create specific pipelines, render targets, etc.
        m_vkContext->init(context->windowManager->getGLFWHandle()); // Init Vulkan context
        Log::Info("Renderer Initialized.");
    }

    void RendererSystem::shutdown() {
        // Cleanup renderer-specific resources (pipelines, etc.)
        // Note: VulkanContext cleanup is handled by Application
        Log::Info("Renderer Shutdown.");
    }

    VulkanContext *RendererSystem::getVulkanContext() { return m_vkContext; }

    void RendererSystem::uploadMesh(entt::entity entity, const std::vector<Vertex> &vertices,
                              const std::vector<uint32_t> &indices) {
        if (vertices.empty() || indices.empty()) {
            Log::Warn("[Renderer::uploadMesh] Attempting to upload empty mesh for entity {}.", (uint32_t) entity);
            // Remove existing component if present
            context->registry->remove<VulkanMeshComponent>(entity);
            return;
        }

        auto registry = context->registry;

        VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
        VkDeviceSize totalSize = vertexBufferSize + indexBufferSize;

        // --- Create Staging Buffer (CPU Visible) ---
        AllocatedBuffer stagingBuffer;
        m_vkContext->createBuffer(totalSize,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  stagingBuffer);

        // --- Map and Copy Data to Staging Buffer ---
        void *data;
        VK_CHECK(vkMapMemory(m_vkContext->device, stagingBuffer.memory, 0, totalSize, 0, &data));
        memcpy(data, vertices.data(), static_cast<size_t>(vertexBufferSize));
        memcpy(static_cast<char *>(data) + vertexBufferSize, indices.data(), static_cast<size_t>(indexBufferSize));
        vkUnmapMemory(m_vkContext->device, stagingBuffer.memory);

        // --- Create Device Local Buffers (GPU Only) ---
        // Get or create the VulkanMeshComponent for the entity
        auto &meshComp = registry->get_or_emplace<VulkanMeshComponent>(entity);

        // Destroy old buffers if they exist before creating new ones
        meshComp.vertexBuffer.destroy(m_vkContext->device);
        meshComp.indexBuffer.destroy(m_vkContext->device);

        m_vkContext->createBuffer(vertexBufferSize,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  meshComp.vertexBuffer);

        m_vkContext->createBuffer(indexBufferSize,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  meshComp.indexBuffer);

        // --- Copy from Staging to Device Local ---
        VkCommandBuffer commandBuffer = m_vkContext->beginSingleTimeCommands(m_vkContext->commandPool);
        // Use graphics pool

        VkBufferCopy vertexCopyRegion{};
        vertexCopyRegion.srcOffset = 0;
        vertexCopyRegion.dstOffset = 0;
        vertexCopyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, meshComp.vertexBuffer.buffer, 1, &vertexCopyRegion);

        VkBufferCopy indexCopyRegion{};
        indexCopyRegion.srcOffset = vertexBufferSize; // Index data starts after vertex data in staging buffer
        indexCopyRegion.dstOffset = 0;
        indexCopyRegion.size = indexBufferSize;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, meshComp.indexBuffer.buffer, 1, &indexCopyRegion);

        m_vkContext->endSingleTimeCommands(commandBuffer, m_vkContext->graphicsQueue, m_vkContext->commandPool);

        // --- Cleanup Staging Buffer ---
        stagingBuffer.destroy(m_vkContext->device);

        // --- Store Mesh Info in Component ---
        meshComp.vertexCount = static_cast<uint32_t>(vertices.size());
        meshComp.indexCount = static_cast<uint32_t>(indices.size());

        // Remove the dirty flag if it exists
        registry->remove<DirtyGPUResource>(entity);

        Log::Info("[Renderer::uploadMesh] Uploading mesh for entity {}\n (Vertices: {}, Indices: {}).", (uint32_t) entity,
                  meshComp.vertexCount, meshComp.indexCount);
    }


    void RendererSystem::drawFrame() {
        // --- Wait for Previous Frame ---
        // Wait for the fence associated with the frame we are about to render
        VK_CHECK(
            vkWaitForFences(m_vkContext->device, 1, &m_vkContext->inFlightFences[m_vkContext->currentFrame], VK_TRUE
                ,
                UINT64_MAX));

        // --- Acquire Image from Swapchain ---
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_vkContext->device, m_vkContext->swapChain, UINT64_MAX,
                                                m_vkContext->imageAvailableSemaphores[m_vkContext->currentFrame],
                                                VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            // Swapchain is outdated (e.g., window resize), recreate and try again next frame
            m_vkContext->recreateSwapChain(context->windowManager->getGLFWHandle());
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            // Suboptimal is okay, but other errors are fatal
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        // --- Reset Fence ---
        // Only reset the fence if we are submitting work, ensures fence is signaled before waiting
        VK_CHECK(vkResetFences(m_vkContext->device, 1, &m_vkContext->inFlightFences[m_vkContext->currentFrame]));

        // --- Update Uniform Buffers ---
        updateUniformBuffer(m_vkContext->currentFrame); // Update UBO for the current frame in flight

        context->uiManager->beginFrame();

        // --- Build ImGui UI --- <<< ADD (Do this in Application::mainLoop or a dedicated function)
        // Example: Call a function in Application to build the UI
        context->uiManager->buildUI(); // We will create this function next
        // --- End Build ImGui UI ---

        context->uiManager->endFrame();

        // --- Record Command Buffer ---
        // Reuse primary command buffers if possible, or re-record each frame
        // For simplicity here, we just allocate and record one primary CB per frame
        // A better approach uses command buffer pools and vkResetCommandBuffer

        // Allocate a command buffer for this frame
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_vkContext->commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(m_vkContext->device, &allocInfo, &commandBuffer));

        // Begin recording
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional flags (e.g., ONE_TIME_SUBMIT)
        beginInfo.pInheritanceInfo = nullptr; // Optional (for secondary command buffers)

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        // --- Begin Render Pass ---
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_vkContext->renderPass; // The render pass to begin
        renderPassInfo.framebuffer = m_vkContext->swapChainFramebuffers[imageIndex]; // Target framebuffer
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_vkContext->swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.1f, 0.1f, 0.15f, 1.0f}}; // Clear color for attachment 0
        clearValues[1].depthStencil = {1.0f, 0}; // Clear depth for attachment 1 (1.0 = far plane)
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // Commands in primary CB

        // --- Bind Pipeline and Global Descriptors ---
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkContext->graphicsPipeline);

        // Bind the global descriptor set (camera UBO etc.) to set 0
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkContext->pipelineLayout,
                                0, 1, &m_vkContext->globalDescriptorSets[m_vkContext->currentFrame],
                                0, nullptr);


        // --- Set Dynamic State (Viewport and Scissor) ---
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_vkContext->swapChainExtent.width);
        viewport.height = static_cast<float>(m_vkContext->swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_vkContext->swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


        // --- Render Scene Geometry ---
        // Iterate through entities with Transform and VulkanMesh
        auto view = context->registry->view<TransformComponent, VulkanMeshComponent>();
        for (auto entity: view) {
            auto &transform = view.get<TransformComponent>(entity);
            auto &mesh = view.get<VulkanMeshComponent>(entity);

            if (mesh.vertexBuffer.buffer == VK_NULL_HANDLE || mesh.indexBuffer.buffer == VK_NULL_HANDLE || mesh.indexCount == 0)
                continue;

            // Bind vertex and index buffers
            VkBuffer vertexBuffers[] = {mesh.vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            // --- Push the model matrix --- <<< UNCOMMENT THIS LINE
            // Ensure the transform component's matrix is up-to-date if needed
            // transform.updateModelMatrix(); // If position/rotation/scale changed
            vkCmdPushConstants(
                commandBuffer,
                m_vkContext->pipelineLayout,    // Pipeline layout that defines the push constant range
                VK_SHADER_STAGE_VERTEX_BIT,     // Stage(s) accessing the push constants
                0,                              // Offset within the push constant block
                sizeof(Matrix4f),              // Size of the data being pushed
                &transform.cachedModelMatrix                // Pointer to the data (model matrix)
            );
            // --- End Push Constant ---

            // Draw indexed geometry
            vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
        }

        // --- TODO: Render Point Clouds ---
        // Bind point cloud pipeline
        // Iterate through PointCloudComponent entities
        // Bind vertex buffer (positions, colors)
        // vkCmdDraw(...)

        // --- TODO: Execute Other Render Passes ---
        // vkCmdNextSubpass(...) or vkCmdEndRenderPass() and vkCmdBeginRenderPass(...)

        context->uiManager->recordDrawCommands(commandBuffer);

        // --- End Render Pass ---
        vkCmdEndRenderPass(commandBuffer);

        // --- End Recording Command Buffer ---
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        // --- Submit Command Buffer ---
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_vkContext->imageAvailableSemaphores[m_vkContext->currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // Wait at this stage
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer; // Submit the recorded command buffer

        VkSemaphore signalSemaphores[] = {m_vkContext->renderFinishedSemaphores[m_vkContext->currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores; // Signal this semaphore when done

        // Submit to the graphics queue, signal the fence when done
        VK_CHECK(
            vkQueueSubmit(m_vkContext->graphicsQueue, 1, &submitInfo, m_vkContext->inFlightFences[m_vkContext->
                currentFrame]));

        // --- Presentation ---
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; // Wait for rendering to finish

        VkSwapchainKHR swapChains[] = {m_vkContext->swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex; // Index of the swapchain image to present
        presentInfo.pResults = nullptr; // Optional

        result = vkQueuePresentKHR(m_vkContext->presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context->windowManager->m_framebufferResized) {
            // Swapchain needs recreation (or was suboptimal)
            context->windowManager->m_framebufferResized = false; // Reset resize flag
            m_vkContext->recreateSwapChain(context->windowManager->getGLFWHandle());
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        // --- Advance Frame Index ---
        m_vkContext->currentFrame = (m_vkContext->currentFrame + 1) % m_vkContext->MAX_FRAMES_IN_FLIGHT;

        // --- Free the command buffer used for this frame ---
        // Note: A more robust system might use command buffer pools and reuse buffers.
        // For simplicity, we allocate/free per frame.
        //vkFreeCommandBuffers(m_vkContext->device, m_vkContext->commandPool, 1, &commandBuffer);
    }


    void RendererSystem::updateUniformBuffer(uint32_t currentImage) {
        // Get camera data from the application/camera system
        auto camera = context->cameraSystem->getCurrentCamera();
        if (!camera) return;

        CameraSystem::update(*camera);
        const auto &view = camera->viewMatrix; // Assuming Application has getCamera()

        GlobalUBO ubo{};
        ubo.view = view;
        ubo.proj = camera->projectionMatrix;

        // Calculate camera position from view matrix (inverse)
        Matrix4f invView = view.inverse();
        ubo.cameraPos.head<3>() = camera->position;
        ubo.cameraPos[3] = 1.0f; // Homogeneous coordinate

        // TODO: Update light direction or other global params if needed

        // Copy data to the mapped buffer for the current frame in flight
        if (m_vkContext->uniformBuffers[currentImage].mappedData) {
            memcpy(m_vkContext->uniformBuffers[currentImage].mappedData, &ubo, sizeof(ubo));
        } else {
            // If not persistently mapped (should be in this setup, but handle just in case)
            void *data;
            vkMapMemory(m_vkContext->device, m_vkContext->uniformBuffers[currentImage].memory, 0, sizeof(ubo), 0,
                        &data);
            memcpy(data, &ubo, sizeof(ubo));
            vkUnmapMemory(m_vkContext->device, m_vkContext->uniformBuffers[currentImage].memory);
        }
    }
}
