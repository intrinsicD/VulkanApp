//
// Created by alex on 4/9/25.
//

#ifndef VULKANCONTEXT_H
#define VULKANCONTEXT_H

#include <vector>
#include <optional>
#include <string>

#include "VulkanUtils.h"

#include <cuda_runtime.h>
#include <slang/slang.h> // Slang shader compilation API


struct GLFWwindow;

namespace Bcg{
#ifdef NDEBUG
    #define VK_CHECK(call) (call)
#else
    #define VK_CHECK(call)                                                    \
        do {                                                                  \
            VkResult result_ = (call);                                        \
            if (result_ != VK_SUCCESS) {                                      \
                fprintf(stderr, "Vulkan call failed: %s (%d) in %s:%d\n",     \
                Bcg::vkResultToString(result_), result_, __FILE__, __LINE__); \
                abort();                                                      \
            }                                                                 \
        } while (0)
#endif

#ifdef NDEBUG
    #define CUDA_CHECK(call) (call)
#else
    #define CUDA_CHECK(call)                                                    \
        do {                                                                    \
            cudaError_t err = (call);                                           \
            if (err != cudaSuccess) {                                           \
                fprintf(stderr, "CUDA error: %s (%d) in %s:%d\n",               \
                cudaGetErrorString(err), err, __FILE__, __LINE__);      \
                abort();                                                        \
            }                                                                   \
        } while (0)
#endif

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeFamily; // For Vulkan compute / CUDA interop

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
            // Add computeFamily if strictly needed
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct VulkanContext {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        QueueFamilyIndices queueFamilyIndices;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        VkQueue computeQueue = VK_NULL_HANDLE; // Optional

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers; // For the main render pass

        VkRenderPass renderPass = VK_NULL_HANDLE; // Default render pass
        VkDescriptorSetLayout globalSetLayout = VK_NULL_HANDLE; // Camera matrices etc.
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; // Default mesh pipeline layout
        VkPipeline graphicsPipeline = VK_NULL_HANDLE; // Default mesh pipeline

        VkCommandPool commandPool = VK_NULL_HANDLE; // For graphics commands
        VkCommandPool transferCommandPool = VK_NULL_HANDLE; // Optional: for transfer queue

        AllocatedImage depthImage; // Depth buffer

        // Descriptor Pool for global descriptors
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> globalDescriptorSets; // One per frame in flight
        std::vector<AllocatedBuffer> uniformBuffers; // For global uniforms like camera

        // Synchronization
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkCommandBuffer> commandBuffers;
        uint32_t currentFrame = 0; // Frame index for synchronization primitives
        const int MAX_FRAMES_IN_FLIGHT = 2;

        // --- ImGui Resources --- <<< ADD
        VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
        AllocatedImage fontTexture; // We can reuse AllocatedImage struct
        // --- End ImGui Resources ---

        // CUDA context (minimal)
        int cudaDeviceID = -1;
        cudaStream_t cudaStream = nullptr; // For potential async operations

        // Function pointers for CUDA interop (load dynamically if used)
        // PFN_vkGetMemoryCudaHandleNV ... etc.

        // Slang session
        slang::IGlobalSession *slangGlobalSession = nullptr;
        slang::ISession *slangSession = nullptr;

        // --- Methods ---
        void init(GLFWwindow *window);

        void cleanup();

        void recreateSwapChain(GLFWwindow *window);

        // Helpers
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);

        bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);

        VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                     VkFormatFeatureFlags features);

        VkFormat findDepthFormat();

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                          AllocatedBuffer &allocatedBuffer);

        VkCommandBuffer beginSingleTimeCommands(VkCommandPool pool);

        void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties, AllocatedImage &allocatedImage);

        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        VkShaderModule createShaderModule(const std::vector<uint32_t> &code);

        VkShaderModule compileSlangShader(const std::string &shaderPath, SlangStage stage);

    private:
        void initVulkan(GLFWwindow *window);

        void setupDebugMessenger();

        void createSurface(GLFWwindow *window);

        void pickPhysicalDevice();

        void createLogicalDevice();

        void createSwapChain(GLFWwindow *window);

        void createImageViews();

        void createRenderPass();

        void createDescriptorSetLayout();

        void createGraphicsPipeline();

        void createCommandPools();

        void createDepthResources();

        void createFramebuffers();

        void createUniformBuffers();

        void createDescriptorPool();

        void createDescriptorSets();

        void createSyncObjects();

        void initSlang();

        void initCuda(); // Basic CUDA init
        void initImGui(); // <<< ADD Declaration
        void uploadImGuiFonts(); // <<< ADD Declaration

        void cleanupSwapChain();

        void cleanupImGui();

        // Validation Layers
        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        // Add extensions needed for interop if used:
        // VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        // VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, etc.

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif

        bool checkValidationLayerSupport();

        std::vector<const char *> getRequiredExtensions();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                            void *pUserData);

        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                              const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                              const VkAllocationCallbacks *pAllocator,
                                              VkDebugUtilsMessengerEXT *pDebugMessenger);

        void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                           const VkAllocationCallbacks *pAllocator);

        bool isDeviceSuitable(VkPhysicalDevice device);
    };
}
#endif //VULKANCONTEXT_H
