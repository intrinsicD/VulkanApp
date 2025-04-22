//
// Created by alex on 22.04.25.
//

#include "VulkanWrappers.h"
#include "VulkanUtils.h"
#include "Logger.h"

namespace Bcg{
    VKAPI_ATTR VkBool32 VKAPI_CALL Bcg::debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT              messageType,
            const VkDebugUtilsMessengerCallbackDataEXT  *pCallbackData,
            void                                        *pUserData)
    {
        // Severity prefix

        std::string severity_prefix = "[Vulkan::ValidationLayer";

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            severity_prefix += "::Verbose";
        }
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            severity_prefix += "::Info";
        }
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            severity_prefix += "::Warning";
        }
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            severity_prefix += "::Error";
        }

        // Type tags (optional)
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
            severity_prefix += "::General";
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            severity_prefix += "::Validation";
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
            severity_prefix += "::Performance";
        }
        severity_prefix += "] ";

        // The actual callback message
        Log::Info("{} {}", severity_prefix, pCallbackData->pMessage);

        // Returning VK_FALSE tells Vulkan to **not** abort the call that triggered the validation message.
        return VK_FALSE;
    }

    Instance::Instance(
            const char*                     app_name,
            const std::vector<const char*>& extensions,
            bool                            enable_layers,
            const std::vector<const char*>& layers
    ) {
        // --- App Info ---
        VkApplicationInfo app_info{};
        app_info.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName    = app_name;
        app_info.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName         = "Custom Engine";
        app_info.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion          = VK_API_VERSION_1_2;

        // --- Instance Create Info ---
        VkInstanceCreateInfo create_info{};
        create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo        = &app_info;
        create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        if (enable_layers) {
            create_info.enabledLayerCount   = static_cast<uint32_t>(layers.size());
            create_info.ppEnabledLayerNames = layers.data();

            // Chain in debug‑messenger create info so we catch errors during vkCreateInstance
            VkDebugUtilsMessengerCreateInfoEXT debug_ci{};
            debug_ci.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_ci.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                       | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debug_ci.pfnUserCallback = debugCallback;
            create_info.pNext        = &debug_ci;
        } else {
            create_info.enabledLayerCount = 0;
            create_info.pNext             = nullptr;
        }

        // --- Create Vulkan Instance ---
        VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance_));
    }

    Instance::~Instance() noexcept {
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
        }
    }

    Instance::Instance(Instance&& other) noexcept
            : instance_(other.instance_) {
        other.instance_ = VK_NULL_HANDLE;
    }

    Instance& Instance::operator=(Instance&& other) noexcept {
        if (this != &other) {
            if (instance_ != VK_NULL_HANDLE) {
                vkDestroyInstance(instance_, nullptr);
            }
            instance_       = other.instance_;
            other.instance_ = VK_NULL_HANDLE;
        }
        return *this;
    }
}