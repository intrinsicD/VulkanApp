//
// Created by alex on 22.04.25.
//

#ifndef VULKANENTTAPP_VULKANWRAPPERS_H
#define VULKANENTTAPP_VULKANWRAPPERS_H

#include <vulkan/vulkan.h>
#include <vector>

namespace Bcg {
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

// A small RAII wrapper around VkInstance
    class Instance {
    public:
        Instance(
                const char *app_name,
                const std::vector<const char *> &extensions,
                bool enable_layers = false,
                const std::vector<const char *> &layers = {}
        );

        ~Instance() noexcept;

        // No copy
        Instance(const Instance &) = delete;

        Instance &operator=(const Instance &) = delete;

        // Move
        Instance(Instance &&) noexcept;

        Instance &operator=(Instance &&) noexcept;

        VkInstance get() const noexcept { return instance_; }

    private:
        VkInstance instance_{VK_NULL_HANDLE};
    };

    class DebugMessenger {
    public:
        DebugMessenger();

        ~DebugMessenger();


    };


    class Device {

    };

    class CommandBuffer {

    };

    class CommandPool {

    };

    class DescriptorSet {

    };

    class DescriptorSetLayout {

    };

    class Framebuffer {

    };

    class Image {

    };

    class ImageView {

    };

    class Pipeline {

    };

    class RenderPass {

    };

    class ShaderModule {

    };

    class SwapChain {

    };

    class Buffer {

    };

    class Memory {

    };

    class Semaphore {

    };

    class Fence {

    };

    class Queue {

    };

    class PhysicalDevice {

    };

    class Surface {

    };


    class Sampler {

    };

    class PipelineLayout {

    };

    class PipelineCache {

    };

    class Event {

    };

    class AccelerationStructure {

    };


}

#endif //VULKANENTTAPP_VULKANWRAPPERS_H
