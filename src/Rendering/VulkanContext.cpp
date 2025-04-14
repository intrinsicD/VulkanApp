//
// Created by alex on 4/9/25.
//

#include <iostream>
#include <set>
#include <array>
#include <fstream>
#include <sstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#include "VulkanContext.h"
#include "Logger.h"
#include "ShaderData.h"


namespace Bcg {
    void VulkanContext::init(GLFWwindow *window) {
        initVulkan(window);
        setupDebugMessenger();
        createSurface(window);
        pickPhysicalDevice();
        createLogicalDevice();
        initSlang(); // Needs logical device
        initCuda(); // Needs logical device/physical device info
        createSwapChain(window);
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout(); // Before pipeline
        createGraphicsPipeline();
        createCommandPools();
        createDepthResources();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createSyncObjects();

        initImGui(); // <<< ADD Init ImGui pool etc.
        uploadImGuiFonts(); // <<< ADD Upload fonts after init
    }

    void VulkanContext::cleanup() {
        cleanupImGui(); // Cleans ImGui resources
        cleanupSwapChain(); // Cleans swapchain-dependent resources

        // Destroy pipelines and layouts
        if (graphicsPipeline != VK_NULL_HANDLE) {
            // Add null checks for safety
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            graphicsPipeline = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
        if (globalSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, globalSetLayout, nullptr);
            globalSetLayout = VK_NULL_HANDLE;
        }

        // Destroy uniform buffers
        for (auto &buffer: uniformBuffers) {
            buffer.destroy(device);
        }
        uniformBuffers.clear();

        // Destroy main descriptor pool
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }

        // Destroy sync objects (Semaphores and Fences are okay to destroy even if device is gone later)
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // Check handles before destroying
            if (renderFinishedSemaphores.size() > i && renderFinishedSemaphores[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            if (imageAvailableSemaphores.size() > i && imageAvailableSemaphores[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            if (inFlightFences.size() > i && inFlightFences[i] != VK_NULL_HANDLE)
                vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        // Clear vectors after destroying contents if needed
        renderFinishedSemaphores.clear();
        imageAvailableSemaphores.clear();
        inFlightFences.clear();


        // Command Buffers are implicitly freed by destroying the pool
        commandBuffers.clear(); // Just clear the vector

        // Destroy command pools
        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, commandPool, nullptr);
            commandPool = VK_NULL_HANDLE;
        }
        if (transferCommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, transferCommandPool, nullptr);
            transferCommandPool = VK_NULL_HANDLE;
        }

        // Cleanup Slang
        if (slangSession) slangSession->release();
        if (slangGlobalSession) slangGlobalSession->release();
        slangSession = nullptr;
        slangGlobalSession = nullptr;


        // Cleanup CUDA (minimal)
        if (cudaStream)
            CUDA_CHECK(cudaStreamDestroy(cudaStream));
        cudaStream = nullptr;

        // --- Destroy logical device --- (Happens AFTER cleaning dependent objects)
        if (device != VK_NULL_HANDLE) {
            Log::Info("[VulkanContext::cleanup] Destroying Vulkan Logical Device...");
            vkDestroyDevice(device, nullptr);
            device = VK_NULL_HANDLE; // Nullify handle after destruction
        }

        // Destroy debug messenger (Needs instance)
        if (enableValidationLayers && debugMessenger != VK_NULL_HANDLE && instance != VK_NULL_HANDLE) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            debugMessenger = VK_NULL_HANDLE;
        }

        // Destroy surface (Needs instance)
        if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            surface = VK_NULL_HANDLE;
        }

        // Destroy instance (Last Vulkan step)
        if (instance != VK_NULL_HANDLE) {
            Log::Info("[VulkanContext::cleanup] Destroying Vulkan Surface Instance...");
            vkDestroyInstance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }
    }

    void VulkanContext::recreateSwapChain(GLFWwindow *window) {
        // Handle minimization
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        VK_CHECK(vkDeviceWaitIdle(device)); // Wait until device is idle

        cleanupSwapChain(); // Clean old swapchain and dependent resources

        // Recreate swapchain and dependent resources
        createSwapChain(window);
        createImageViews();
        // Render pass usually doesn't need recreation unless format changes drastically
        // Pipeline might need recreation if viewport/scissor changes (or make dynamic)
        createDepthResources(); // Depends on swapchain extent
        createFramebuffers(); // Depends on image views and depth view
        // Descriptor sets usually okay unless resource bindings change
        // Command buffers will be re-recorded anyway
    }

    void VulkanContext::cleanupSwapChain() {
        depthImage.destroy(device); // Destroy depth image and view

        // Destroy framebuffers
        for (auto framebuffer: swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        swapChainFramebuffers.clear();

        // Destroy image views
        for (auto imageView: swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        swapChainImageViews.clear();

        // Destroy swapchain itself
        if (swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device, swapChain, nullptr);
            swapChain = VK_NULL_HANDLE;
        }
    }


    // --- VulkanContext Private Helper Implementations ---

    void VulkanContext::initVulkan(GLFWwindow *window) {
        // --- Create Instance ---
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "EnTT Vulkan App";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Custom Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2; // Request Vulkan 1.2 or higher if needed

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // Needed even if layers disabled
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // Add debug messenger create info for instance creation/destruction debugging
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debugCallback;
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
        Log::Info("[VulkanContext::initVulkan] Vulkan Instance created.");
    }

    void VulkanContext::setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT; // Add verbose for more info
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional

        VK_CHECK(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
        Log::Info("[VulkanContext::setupDebugMessenger] Vulkan Debug Messenger created.");
    }

    void VulkanContext::createSurface(GLFWwindow *window) {
        VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
        Log::Info("[VulkanContext::createSurface] Vulkan surface created.");
    }

    void VulkanContext::pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        int bestScore = -1;

        Log::Info("[VulkanContext::pickPhysicalDevice] Available Physical Devices: {}", deviceCount);

        for (const auto &device: devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            Log::Info("[VulkanContext::pickPhysicalDevice] Device Name: {}", deviceProperties.deviceName);

            if (isDeviceSuitable(device)) {
                int currentScore = 0;
                // Give high score to discrete GPUs
                if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    currentScore += 1000;
                }
                // Add score for integrated GPUs (lower than discrete)
                else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                    currentScore += 500;
                }
                // Add other scoring metrics if needed (e.g., memory size)

                Log::Info("[VulkanContext::pickPhysicalDevice] Device: {} (Type: {}) Score: {}",
                          deviceProperties.deviceName,
                          deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                              ? "Discrete"
                              : deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
                                    ? "Integrated"
                                    : "Other", currentScore);

                if (currentScore > bestScore) {
                    bestScore = currentScore;
                    bestDevice = device;
                }
            } else {
                Log::Error("[VulkanContext::pickPhysicalDevice] Device is not a GPU!");
            }
        }

        if (bestDevice == VK_NULL_HANDLE) {
            // Changed from physicalDevice
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        physicalDevice = bestDevice; // Assign the best scored device

        VkPhysicalDeviceProperties selectedProps;
        vkGetPhysicalDeviceProperties(physicalDevice, &selectedProps);
        Log::Info("[VulkanContext::pickPhysicalDevice] Selected Physical Device: {}", selectedProps.deviceName);

        // Store queue family indices for the selected device
        queueFamilyIndices = findQueueFamilies(physicalDevice);
    }

    void VulkanContext::createLogicalDevice() {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            queueFamilyIndices.graphicsFamily.value(),
            queueFamilyIndices.presentFamily.value()
        };
        if (queueFamilyIndices.computeFamily.has_value()) {
            uniqueQueueFamilies.insert(queueFamilyIndices.computeFamily.value());
        }


        float queuePriority = 1.0f;
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{}; // Enable features needed, e.g., samplerAnisotropy
        // vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures); // Query defaults if needed
        deviceFeatures.samplerAnisotropy = VK_TRUE; // Example feature
        deviceFeatures.geometryShader = VK_FALSE; // Example: disabling a feature
        deviceFeatures.wideLines = VK_TRUE; // Example: Needed for line width > 1.0f
        deviceFeatures.fillModeNonSolid = VK_TRUE; // Example: Needed for wireframe


        // Vulkan 1.2 Features (Example: buffer device address)
        VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        features12.bufferDeviceAddress = VK_TRUE; // Example: If using buffer device addresses

        // Vulkan 1.1 Features (Example: external memory for CUDA interop)
        VkPhysicalDeviceVulkan11Features features11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures; // Enable standard features

        // Chain the feature structures using pNext
        createInfo.pNext = &features11;
        features11.pNext = &features12;
        // Add more feature structs here if needed (e.g., Ray Tracing)

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // Device layers (deprecated, use instance layers)
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
        Log::Info("[VulkanContext::createLogicalDevice] Vulkan Logical Device created.");

        // Get queue handles
        vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
        if (queueFamilyIndices.computeFamily.has_value()) {
            vkGetDeviceQueue(device, queueFamilyIndices.computeFamily.value(), 0, &computeQueue);
        } else {
            // Fallback: Use graphics queue for compute if no dedicated queue
            computeQueue = graphicsQueue;
            queueFamilyIndices.computeFamily = queueFamilyIndices.graphicsFamily;
        }
    }

    void VulkanContext::initSlang() {
        SlangResult result = slang::createGlobalSession(&slangGlobalSession);
        if (SLANG_FAILED(result)) {
            throw std::runtime_error("Failed to create Slang global session");
        }

        slang::SessionDesc sessionDesc = {};
        // Specify target format and profile if needed, otherwise Slang tries to detect
        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV; // Target SPIR-V
        targetDesc.profile = slangGlobalSession->findProfile("spirv_1_5"); // Or appropriate version
        // targetDesc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        // Optional: Add include paths
        // const char* includePath = "shaders/";
        // sessionDesc.searchPaths = &includePath;
        // sessionDesc.searchPathCount = 1;

        result = slangGlobalSession->createSession(sessionDesc, &slangSession);
        if (SLANG_FAILED(result)) {
            if (slangGlobalSession) slangGlobalSession->release();
            throw std::runtime_error("Failed to create Slang session");
        }
        Log::Info("[VulkanContext::initSlang] Slang session created.");
    }

    void VulkanContext::initCuda() {
        // --- Basic CUDA Initialization ---
        // This part is minimal. Real integration requires Vulkan-CUDA interop setup
        // using external memory/semaphores.

        int deviceCount = 0;
        CUDA_CHECK(cudaGetDeviceCount(&deviceCount));
        if (deviceCount == 0) {
            Log::Warn("[VulkanContext::initCuda] No CUDA-capable devices found.");
            return;
        }

        // Optional: Find CUDA device corresponding to the Vulkan physical device
        // This requires VK_KHR_external_memory_capabilities and VK_NV_external_memory_cuda extensions
        // and querying device UUIDs or LUIDs to match them.
        // For simplicity, we'll just pick the first CUDA device for now.
        cudaDeviceID = 0; // Or find the matching one
        CUDA_CHECK(cudaSetDevice(cudaDeviceID));

        // Create a CUDA stream (optional, for async operations)
        CUDA_CHECK(cudaStreamCreate(&cudaStream));

        Log::Info("[VulkanContext::initCuda] CUDA context initialized for device : {}", cudaDeviceID);

        // Placeholder for loading Vulkan-CUDA interop functions if needed
        // auto vkGetMemoryCudaHandleNV = (PFN_vkGetMemoryCudaHandleNV)vkGetDeviceProcAddr(device, "vkGetMemoryCudaHandleNV");
        // if (!vkGetMemoryCudaHandleNV) { /* Handle error or warning */ }
    }

    const char* VkResultToString(VkResult result) {
        switch (result) {
            case VK_SUCCESS: return "VK_SUCCESS";
            case VK_NOT_READY: return "VK_NOT_READY";
            case VK_TIMEOUT: return "VK_TIMEOUT";
            case VK_EVENT_SET: return "VK_EVENT_SET";
            case VK_EVENT_RESET: return "VK_EVENT_RESET";
            case VK_INCOMPLETE: return "VK_INCOMPLETE";
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
            case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
            default: return "UNKNOWN_ERROR";
        }
    }

    void VulkanContext::initImGui() {
        Log::Info("[VulkanContext::initImGui] ImGui Vulkan backend initialized.");
        // 1. Create Descriptor Pool for ImGui
        // Needs 1 image sampler descriptor type for the font texture.
        // Increase if you plan to use texture IDs in ImGui yourself.
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes); // Allow ample sets
        pool_info.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescriptorPool));
        Log::Info("[VulkanContext::initImGui] ImGui Descriptor Pool created.");

        // 2. Setup ImGui Vulkan Backend
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        init_info.QueueFamily = queueFamilyIndices.graphicsFamily.value();
        init_info.Queue = graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE; // Optional
        init_info.DescriptorPool = imguiDescriptorPool;
        init_info.Subpass = 0; // Render ImGui in the first subpass
        // Number of images in swap chain + number of concurrent frames.
        init_info.MinImageCount = static_cast<uint32_t>(swapChainImages.size()); // Typically 2 or 3
        init_info.ImageCount = static_cast<uint32_t>(swapChainImages.size()); // Should match swapchain image count
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // No MSAA in our main render pass
        init_info.Allocator = nullptr; // Use default allocator
        init_info.CheckVkResultFn = [](VkResult err) {
            // Custom error checking
            if (err == 0) return;
            Log::Error("[VulkanContext::initImGui] ImGui Vulkan Error: VkResult = {}", VkResultToString(err));
            if (err < 0) abort();
        };
        init_info.RenderPass = renderPass;
        // This needs the renderPass to be created already
        ImGui_ImplVulkan_Init(&init_info); //,renderPass);
    }

    void VulkanContext::uploadImGuiFonts() {
        Log::Info("[VulkanContext::uploadImGuiFonts] Uploading ImGui fonts...");
        // --- Use parameter-less version ---
        // This version handles its own command buffer submission and sync.
        // Ensure the device is idle *before* calling this if necessary, although
        // the function itself might handle synchronization. Check ImGui backend docs/code if unsure.
        // VK_CHECK(vkDeviceWaitIdle(device)); // Might be needed before if issues persist

        bool success = ImGui_ImplVulkan_CreateFontsTexture(); // <<< No command buffer argument
        if (!success) {
            Log::Error("[VulkanContext::uploadImGuiFonts] ImGui_ImplVulkan_CreateFontsTexture failed!");
            // Handle error appropriately, maybe throw or log more severely
        }

        // --- Original code that is now incorrect for this ImGui version ---
        // VkCommandBuffer command_buffer = beginSingleTimeCommands(commandPool);
        // ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        // endSingleTimeCommands(command_buffer, graphicsQueue, commandPool);

        // Clear font textures from CPU memory - Function name was wrong.
        // ImGui_ImplVulkan_DestroyFontUploadObjects(); // <<< INCORRECT NAME
        // The parameter-less CreateFontsTexture often doesn't need a separate destroy
        // for upload objects, as it cleans up internally. If textures are retained
        // incorrectly, you might need ImGui_ImplVulkan_DestroyFontsTexture() during cleanup,
        // but typically ImGui_ImplVulkan_Shutdown() handles that. Let's omit this for now.
    }

    void VulkanContext::cleanupImGui() {
        if (imguiDescriptorPool != VK_NULL_HANDLE) {
            Log::Info("Shutting down ImGui Vulkan backend...");
            // Destroy font texture manually if needed (ImGui shutdown might not)
            // fontTexture.destroy(device); // If ImGui_ImplVulkan_Shutdown doesn't do it. Usually it does.

            ImGui_ImplVulkan_Shutdown(); // Shutdown Vulkan backend
            Log::Info("  ImGui_ImplVulkan_Shutdown called.");

            vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr); // Destroy ImGui pool
            imguiDescriptorPool = VK_NULL_HANDLE;
            Log::Info("  ImGui Descriptor Pool destroyed.");
        }
    }

    void VulkanContext::createSwapChain(GLFWwindow *window) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.
            maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // 1 unless doing stereoscopic rendering
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Render directly to swapchain

        // How to handle swap chain images across multiple queue families
        uint32_t indices[] = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};
        if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Images used by multiple queues
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = indices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Image owned by one queue family
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // Usually identity
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // No blending with window system
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE; // Allow clipping obscured pixels

        createInfo.oldSwapchain = VK_NULL_HANDLE; // For resizing, pass the old one here

        VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));

        // Retrieve swap chain images
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        Log::Info("Vulkan Swapchain created ({} images).", imageCount);
    }

    void VulkanContext::createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat,
                                                     VK_IMAGE_ASPECT_COLOR_BIT);
        }
        Log::Info("Swapchain Image Views created.");
    }


    void VulkanContext::createRenderPass() {
        // --- Color Attachment (Swapchain Image) ---
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No MSAA for now
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear framebuffer before drawing
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store result for presentation
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout before pass
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout after pass for presentation

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; // Index in pAttachments array
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during subpass

        // --- Depth Attachment ---
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Don't need depth after pass
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1; // Index in pAttachments array
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // --- Subpass ---
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef; // layout(location = 0) out vec4 outColor;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // --- Subpass Dependency ---
        // Ensure render pass waits for image availability before writing colors
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Implicit subpass before render pass
        dependency.dstSubpass = 0; // Our subpass
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


        // --- Render Pass Create Info ---
        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
        Log::Info("Default Render Pass created.");
    }

    void VulkanContext::createDescriptorSetLayout() {
        // Layout for global uniforms (like camera matrices) bound at set 0
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0; // binding = 0 in shader
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;;
        // Accessible in vertex and fragment shader
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // Add more bindings here for textures, other global resources etc.
        // VkDescriptorSetLayoutBinding samplerLayoutBinding{}; ...

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding}; // Add sampler binding etc.
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &globalSetLayout));
        Log::Info("Global Descriptor Set Layout created.");

        // TODO: Create descriptor set layouts for per-material or per-object data if needed
    }


    void VulkanContext::createGraphicsPipeline() {
        // --- Shader Modules ---
        // Compile shaders using Slang
        // NOTE: Paths are relative to execution directory or need absolute paths
        VkShaderModule vertShaderModule =
                compileSlangShader("shaders/simple.slang", SlangStage::SLANG_STAGE_VERTEX);
        VkShaderModule fragShaderModule = compileSlangShader("shaders/simple.slang",
                                                             SlangStage::SLANG_STAGE_FRAGMENT);

        if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
            // Cleanup already created module if one failed
            if (vertShaderModule) vkDestroyShaderModule(device, vertShaderModule, nullptr);
            if (fragShaderModule) vkDestroyShaderModule(device, fragShaderModule, nullptr);
            throw std::runtime_error("Failed to create shader modules!");
        }

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main"; // Entry point function in Slang code

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main"; // Entry point function in Slang code

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // --- Vertex Input ---
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // --- Input Assembly ---
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Draw triangles
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // --- Viewport and Scissor ---
        // These are often dynamic, but we set initial dummy values here
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport; // Can be dynamic
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor; // Can be dynamic

        // --- Rasterizer ---
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; // Clamp fragments beyond near/far planes
        rasterizer.rasterizerDiscardEnable = VK_FALSE; // Keep geometry passing through rasterizer
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Fill polygons (VK_POLYGON_MODE_LINE for wireframe)
        rasterizer.lineWidth = 1.0f; // Width of lines (requires wideLines feature if > 1.0)
        rasterizer.cullMode = VK_CULL_MODE_NONE; // Cull back faces
        //rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Cull back faces
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        // Define front face by vertex order (Vulkan default matches GLM)
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // --- Multisampling ---
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE; // Enable sample shading or not
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // No MSAA for now
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // --- Depth and Stencil Testing ---
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE; // <<< SET TO FALSE
        depthStencil.depthWriteEnable = VK_FALSE; // <<< SET TO FALSE
        //depthStencil.depthTestEnable = VK_TRUE; // Enable depth testing
        //depthStencil.depthWriteEnable = VK_TRUE; // Write passing fragments to depth buffer
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // Fragments with smaller depth pass
        depthStencil.depthBoundsTestEnable = VK_FALSE; // Optional depth bounds test
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE; // Disable stencil test
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional


        // --- Color Blending ---
        // Config per attached framebuffer
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; // No blending for now
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        // Global blend state
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE; // Use blend factors, not logic op
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // --- Dynamic State ---
        // Allow changing viewport and scissor without recreating pipeline
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            // VK_DYNAMIC_STATE_LINE_WIDTH // If using wide lines dynamically
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // --- Pipeline Layout ---
        // Specifies uniforms and push constants
        // Define the push constant range
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Accessible in vertex shader
        pushConstantRange.offset = 0; // Start at offset 0
        pushConstantRange.size = sizeof(Matrix4f); // Size of the model matrix

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // Bind the global descriptor set layout at set index 0
        std::array<VkDescriptorSetLayout, 1> setLayouts = {globalSetLayout}; // Use array
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data(); // Pointer to array start
        // Add the push constant range <<< FIX
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        // --- Graphics Pipeline ---
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2; // Vertex and Fragment
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState; // Enable dynamic states
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass; // Render pass the pipeline will be used with
        pipelineInfo.subpass = 0; // Index of the subpass
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional: For pipeline derivatives
        pipelineInfo.basePipelineIndex = -1; // Optional

        VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
        Log::Info("Default Graphics Pipeline created.");

        // --- Cleanup Shader Modules ---
        // No longer needed after pipeline creation
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void VulkanContext::createCommandPools() {
        // Pool for graphics commands (drawing, barriers, etc.)
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        // Allow resetting individual command buffers
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
        Log::Info("Graphics Command Pool created.");

        // Optional: Separate pool for transfer operations if using a dedicated transfer queue
        // This can improve performance by allowing concurrent transfer and graphics work.
        // If a dedicated transfer queue exists and is different from graphics:
        // if (queueFamilyIndices.transferFamily.has_value() &&
        //     queueFamilyIndices.transferFamily.value() != queueFamilyIndices.graphicsFamily.value()) {
        //     VkCommandPoolCreateInfo transferPoolInfo = poolInfo;
        //     transferPoolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();
        //     // Transfers are often short-lived, transient buffers are good
        //     transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        //     VK_CHECK(vkCreateCommandPool(device, &transferPoolInfo, nullptr, &transferCommandPool));
        //     Log::Info("Transfer Command Pool created." );
        // }
    }

    void VulkanContext::createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage);

        depthImage.imageView = createImageView(depthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        // Transition layout (optional here, can be done in render pass or single command)
        // transitionImageLayout(depthImage.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        Log::Info("Depth Resources created.");
    }

    void VulkanContext::createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i], // Color attachment (index 0 in render pass)
                depthImage.imageView // Depth attachment (index 1 in render pass)
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass; // Compatible render pass
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
        }
        Log::Info("Swapchain Framebuffers created.");
    }


    void VulkanContext::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(GlobalUBO);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[i]);
            // Persistently map the buffers for easy updates
            VK_CHECK(vkMapMemory(device, uniformBuffers[i].memory, 0, bufferSize, 0, &uniformBuffers[i].mappedData))
            ;
        }
        Log::Info("Uniform Buffers created and mapped.");
    }

    void VulkanContext::createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 1> poolSizes{}; // Adjust size based on descriptor types
        // Pool size for the global UBOs (one per frame in flight)
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        // TODO: Add pool sizes for textures, other buffer types if used globally or by materials

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // Max number of sets to allocate

        VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
        Log::Info("Descriptor Pool created.");
    }

    void VulkanContext::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, globalSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        globalDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, globalDescriptorSets.data()));

        // --- Configure Descriptor Sets ---
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(GlobalUBO); // Size of the UBO data

            // TODO: Add VkDescriptorImageInfo for textures/samplers if they are in the global set

            std::array<VkWriteDescriptorSet, 1> descriptorWrites{}; // Size matches bindings in layout

            // Write for the UBO binding (binding = 0)
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = globalDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0; // Matches the layout binding
            descriptorWrites[0].dstArrayElement = 0; // Start at array element 0
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1; // Update one descriptor
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // Optional
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional

            // TODO: Add writes for other bindings (samplers etc.)
            // descriptorWrites[1].sType = ...
            // descriptorWrites[1].dstBinding = 1; ... etc

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                                   0,
                                   nullptr);
        }
        Log::Info("Global Descriptor Sets allocated and configured.");
    }

    void VulkanContext::createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool; // Use the main graphics command pool
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
        VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start fences signaled (ready for first frame)

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
            VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
            VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]));
        }
        Log::Info("Synchronization objects (Semaphores, Fences) created.");
    }

    // --- VulkanContext Helper Implementations (Continued) ---

    QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice physicalDevice) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
            // Check for Graphics support
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            // Check for Presentation support (needs surface)
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            // Check for dedicated Compute support (optional)
            if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(
                    queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                indices.computeFamily = i; // Found dedicated compute
            }

            // Check for dedicated Transfer support (optional, less common)
            // if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            // indices.transferFamily = i;
            // }


            if (indices.isComplete()) {
                // Found graphics and present
                // Try to find separate compute if not already found
                if (!indices.computeFamily.has_value()) {
                    // Look for any compute queue (even if it overlaps graphics)
                    for (int j = 0; j < queueFamilies.size(); ++j) {
                        if (queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                            indices.computeFamily = j;
                            break;
                        }
                    }
                }
                // If still no compute, it will likely default to graphics queue later
                break;
            }
            i++;
        }

        // Fallback: If no dedicated compute, use graphics queue for compute
        if (!indices.computeFamily.has_value() && indices.graphicsFamily.has_value()) {
            indices.computeFamily = indices.graphicsFamily;
        }


        return indices;
    }

    bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension: availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty(); // True if all required extensions were found
    }


    SwapChainSupportDetails VulkanContext::querySwapChainSupport(VkPhysicalDevice physicalDevice) {
        SwapChainSupportDetails details;
        // Get capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

        // Get formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        // Get present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount,
                                                      details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        // Prefer sRGB if available for better color representation
        for (const auto &availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        // Fallback to the first available format
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanContext::chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes) {
        // Prefer Mailbox for triple buffering (low latency without tearing)
        for (const auto &availablePresentMode: availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        // FIFO is guaranteed to be available (standard vsync)
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window) {
        // If current extent is defined, use it
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            // Otherwise, get size from GLFW and clamp to min/max supported by Vulkan
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    VkFormat VulkanContext::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                                VkFormatFeatureFlags features) {
        for (VkFormat format: candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("Failed to find supported format!");
    }


    VkFormat VulkanContext::findDepthFormat() {
        // Find a suitable depth format (with stencil optional)
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }


    uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // Check if the memory type's bit is set in typeFilter
            // AND if the memory type has all the required property flags
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
                return i; // Found suitable memory type
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                     AllocatedBuffer &allocatedBuffer) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Simple case

        VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &allocatedBuffer.buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, allocatedBuffer.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        // Example: If using VK_KHR_buffer_device_address
        // VkMemoryAllocateFlagsInfo flagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
        // if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        //     flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        //     allocInfo.pNext = &flagsInfo;
        // }


        VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &allocatedBuffer.memory));

        // Bind the memory to the buffer
        VK_CHECK(vkBindBufferMemory(device, allocatedBuffer.buffer, allocatedBuffer.memory, 0)); // Offset 0

        allocatedBuffer.size = size; // Store requested size, not allocation size
        allocatedBuffer.mappedData = nullptr; // Not mapped by default here
    }

    VkCommandBuffer VulkanContext::beginSingleTimeCommands(VkCommandPool pool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = pool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Use command buffer once

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        return commandBuffer;
    }

    void VulkanContext::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool) {
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // Submit to the specified queue and wait for completion
        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE)); // No fence needed, we wait
        VK_CHECK(vkQueueWaitIdle(queue)); // Wait for the command to finish execution

        vkFreeCommandBuffers(device, pool, 1, &commandBuffer); // Free the command buffer
    }

    void VulkanContext::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        // Use graphics queue for simplicity, could use transfer queue if available
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer, graphicsQueue, commandPool);
    }

    void VulkanContext::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                    VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                                    AllocatedImage &allocatedImage) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1; // No mipmapping for now
        imageInfo.arrayLayers = 1; // No image arrays for now
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling for now
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &allocatedImage.image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, allocatedImage.image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &allocatedImage.memory));
        VK_CHECK(vkBindImageMemory(device, allocatedImage.image, allocatedImage.memory, 0));

        allocatedImage.format = format;
        allocatedImage.extent = {width, height, 1};
    }

    VkImageView VulkanContext::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags; // e.g., COLOR or DEPTH
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        // viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Default swizzles
        // viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        // viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        // viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;


        VkImageView imageView;
        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
        return imageView;
    }


    void VulkanContext::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                              VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool); // Use graphics pool for simplicity

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // No queue ownership transfer
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        // Determine aspect mask based on format and layout
        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            // Include stencil if format has it
            if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // Define pipeline stage masks based on layouts
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0; // No need to wait for anything
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Write access for transfer

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // Earliest possible stage
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // Stage where transfer happens
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout ==
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Wait for transfer write to finish
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // Read access in shader

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // Stage where sampling happens
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout ==
                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // Stage for depth/stencil tests
        } else {
            // Add more layout transition cases as needed
            throw std::invalid_argument("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0, // Dependency flags
            0, nullptr, // Memory barriers
            0, nullptr, // Buffer memory barriers
            1, &barrier // Image memory barriers
        );


        endSingleTimeCommands(commandBuffer, graphicsQueue, commandPool);
    }


    void VulkanContext::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0; // Tightly packed data
        region.bufferImageHeight = 0; // Tightly packed data

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Image must be in this layout
            1, // Number of regions
            &region
        );

        endSingleTimeCommands(commandBuffer, graphicsQueue, commandPool);
    }


    VkShaderModule VulkanContext::createShaderModule(const std::vector<uint32_t> &code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size() * sizeof(uint32_t); // Size in bytes
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
        return shaderModule;
    }

    // --- Slang Shader Compilation ---
    VkShaderModule VulkanContext::compileSlangShader(const std::string &shaderPath, SlangStage stage) {
        if (!slangSession) {
            Log::Error("ERROR: Slang session not initialized!");
            return VK_NULL_HANDLE;
        }

        SlangResult result;
        slang::ICompileRequest *request = nullptr;
        result = slangSession->createCompileRequest(&request);
        if (SLANG_FAILED(result) || !request) {
            Log::Error("[]VulkanContext::compileSlangShader] Failed to create Slang compile request for {}",
                       shaderPath);
            return VK_NULL_HANDLE;
        }

        // Add the source file to the request
        int translationUnitIndex = request->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, nullptr);
        request->addTranslationUnitSourceFile(translationUnitIndex, shaderPath.c_str());

        // Specify the entry point
        const char *entryPointName = (stage == SLANG_STAGE_VERTEX) ? "vertexMain" : "fragmentMain";
        // Adjust as needed
        int entryPointIndex = request->addEntryPoint(translationUnitIndex, entryPointName, stage);
        if (entryPointIndex < 0) {
            Log::Error("[VulkanContext::compileSlangShader]: Could not find entry point '({})' in {}", entryPointName,
                       shaderPath);
            Log::Error("[VulkanContext::compileSlangShader]: Diagnostics \n{}", request->getDiagnosticOutput());
            request->release();
            return VK_NULL_HANDLE;
        }

        // Specify the target (already set in session, but can override)
        int targetIndex = request->addCodeGenTarget(SLANG_SPIRV);
        // Optional: Set SPIR-V specific options
        // slang::TargetDesc spirvTarget = {}; ... request->setTargetDesc(targetIndex, spirvTarget);


        // Compile the code
        result = request->compile();

        // Check for errors and print diagnostics
        const char *diagnostics = request->getDiagnosticOutput();
        if (SLANG_FAILED(result)) {
            Log::Error("[Vulkan::compileSlangShader] Compilation failed for {} ({})", shaderPath, entryPointName);
            if (diagnostics && strlen(diagnostics) > 0) {
                Log::Error("[Vulkan::compileSlangShader] Slang Compilation Diagnostics: \n{}", diagnostics);
            }
            request->release();
            return VK_NULL_HANDLE;
        } else {
            if (diagnostics && strlen(diagnostics) > 0) {
                Log::Info("[Vulkan::compileSlangShader] Slang Compilation Diagnostics for {} ({}):\n{}", shaderPath,
                          entryPointName, diagnostics);
            }
        }

        // Get the compiled code (SPIR-V)
        size_t dataSize = 0;
        void const *data = request->getEntryPointCode(entryPointIndex, &dataSize);
        if (!data || dataSize == 0) {
            Log::Error("[VulkanContext::compileSlangShader]: Slang failed to produce SPIR-V code for {},   ({})",
                       shaderPath, entryPointName);
            request->release();
            return VK_NULL_HANDLE;
        }

        // --- Dump SPIR-V to file for inspection ---
        std::string dumpFilename = (stage == SlangStage::SLANG_STAGE_VERTEX)
                                       ? "dump_vertex.spv"
                                       : "dump_fragment.spv";
        std::ofstream dumpFile(dumpFilename, std::ios::binary | std::ios::trunc);
        if (dumpFile.is_open()) {
            dumpFile.write(static_cast<const char *>(data), dataSize);
            dumpFile.close();
            Log::Info("[VulkanContext::compileSlangShader] Dumped SPIR-V ({} bytes) to: {}", dataSize, dumpFilename);
        } else {
            Log::Error("[VulkanContext::compileSlangShader] Failed to open {} for writing SPIR-V dump.", dumpFilename);
        }
        // --- End SPIR-V dump ---


        // Slang returns raw bytes, Vulkan expects uint32_t*, ensure alignment
        if (dataSize % sizeof(uint32_t) != 0) {
            Log::Error(
                "[VulkanContext::compileSlangShader] Slang SPIR-V output size ({}) is not a multiple of 4 bytes!",
                dataSize);
            request->release();
            return VK_NULL_HANDLE;
        }
        // Using vector ensures proper alignment for uint32_t
        std::vector<uint32_t> spirvCode(dataSize / sizeof(uint32_t));
        memcpy(spirvCode.data(), data, dataSize);


        // Create the Vulkan shader module
        VkShaderModule shaderModule = createShaderModule(spirvCode);

        // Clean up the Slang request object
        request->release();

        if (shaderModule != VK_NULL_HANDLE) {
            Log::Info("[VulkanContext::compileSlangShader] Successfully created Vulkan Shader Module for: {} ({})",
                      shaderPath, entryPointName);
        } else {
            // createShaderModule would have thrown via VK_CHECK if vkCreateShaderModule failed,
            // but good practice to check handle anyway, though likely redundant here.
            Log::Error("[VulkanContext::compileSlangShader] Failed to create Vulkan Shader Module for, {}, {}",
                       shaderPath, entryPointName);
        }

        return shaderModule;
    }


    // --- Vulkan Validation Layers and Debug Callback ---

    bool VulkanContext::checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName: validationLayers) {
            bool layerFound = false;
            for (const auto &layerProperties: availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                Log::Error("Validation layer not found: {}", layerName);
                return false;
            }
        }
        return true;
    }

    std::vector<const char *> VulkanContext::getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // Add extensions needed for CUDA interop or other features if required
        // extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
        // extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);


        Log::Info("Required Instance Extensions:");
        for (const auto &ext: extensions) {
            Log::Info("[Vulkan::getRequiredExtensions]\t {}", ext);
        }

        return extensions;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
        // Filter severity
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            Log::Error("Validation Layer: ");
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) { Log::Error("[VERBOSE] "); }
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) { Log::Error("[INFO] "); }
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) { Log::Error("[WARNING] "); }
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) { Log::Error("[ERROR] "); }

            Log::Error("[Type: ");
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) { Log::Error("General "); }
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) { Log::Error("Validation "); }
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) { Log::Error("Performance "); }
            Log::Error("] ");

            Log::Error(pCallbackData->pMessage);

            // Optionally print object names/handles involved
            if (pCallbackData->objectCount > 0) {
                Log::Error("[Vulkan::debugCallback] Objects involved ({})",  pCallbackData->objectCount);
                for (uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
                    std::stringstream ss;
                    ss << "    Object [" << i << "] - Type: " << pCallbackData->pObjects[i].objectType
                               << ", Handle: " << (void *) pCallbackData->pObjects[i].objectHandle;
                    if (pCallbackData->pObjects[i].pObjectName) {
                        Log::Error(", Name: \"{}\"", pCallbackData->pObjects[i].pObjectName);
                    }
                }
            }
        }
        return VK_FALSE; // Application should not be aborted on validation message
    }

    VkResult VulkanContext::CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                         const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                         const VkAllocationCallbacks *pAllocator,
                                                         VkDebugUtilsMessengerEXT *pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void VulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                      const VkAllocationCallbacks *pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        // Check for required features (add more as needed)
        bool featuresSupported = supportedFeatures.samplerAnisotropy &&
                                 supportedFeatures.fillModeNonSolid && // For potential wireframe
                                 supportedFeatures.wideLines; // For lines > 1.0f

        // Check for required Vulkan 1.1/1.2 features used
        VkPhysicalDeviceVulkan11Features features11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        features11.pNext = &features12; // Chain features structs

        VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        features2.pNext = &features11;
        vkGetPhysicalDeviceFeatures2(device, &features2);

        bool extraFeaturesSupported = features12.bufferDeviceAddress; // If using buffer addresses


        return indices.isComplete() && extensionsSupported && swapChainAdequate && featuresSupported &&
               extraFeaturesSupported;
    }
}
