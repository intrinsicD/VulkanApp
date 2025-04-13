//
// Created by alex on 4/9/25.
//

#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "Manager.h"
#include <vulkan/vulkan_core.h>

namespace Bcg {
    class Application;
    class UIManager : public Manager {
    public:
        ~UIManager() override;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        void initImGui();

        void initGLFWBackend();

        void beginFrame();

        void endFrame();

        void recordDrawCommands(VkCommandBuffer commandBuffer);

        void buildUI();
    };
}

#endif //UIMANAGER_H
