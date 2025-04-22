//
// Created by alex on 4/9/25.
//

#ifndef VULKANUTILS_H
#define VULKANUTILS_H

#include <vulkan/vulkan.h>

#ifdef NDEBUG
#define VK_CHECK(call) (call)
#else

#include <cstdio>
#include <cstdlib>

#define VK_CHECK(call)                                                    \
        do {                                                                  \
            VkResult result_ = (call);                                        \
            if (result_ != VK_SUCCESS) {                                      \
                fprintf(stderr, "Vulkan call failed: %s (%d) in %s:%d\n",     \
                VkResultToString(result_), result_, __FILE__, __LINE__); \
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

namespace Bcg {
    const char *VkResultToString(VkResult result);

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
