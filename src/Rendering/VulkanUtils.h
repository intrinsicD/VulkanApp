//
// Created by alex on 4/9/25.
//

#ifndef VULKANUTILS_H
#define VULKANUTILS_H

#include <vulkan/vulkan.h>

namespace Bcg{
    const char *vkResultToString(VkResult result);

    struct AllocatedBuffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        void *mappedData = nullptr; // If persistently mapped

        // Add cleanup logic (or use VMA)
        void destroy(VkDevice device);
    };

    struct AllocatedImage {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkFormat format;
        VkExtent3D extent;

        // Add cleanup logic
        void destroy(VkDevice device);
    };
}

#endif //VULKANUTILS_H
