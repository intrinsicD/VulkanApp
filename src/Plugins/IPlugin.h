//
// Created by alex on 4/9/25.
//

#ifndef IPLUGIN_H
#define IPLUGIN_H

#include "Application.h"

namespace Bcg{
    // --- Plugin Interface ---

    class IPlugin {
    public:
        virtual ~IPlugin() = default;

        virtual const char *getName() const = 0;

        // Called once after basic application setup
        virtual void initialize(ApplicationContext *context) = 0;

        // Called once before application teardown
        virtual void shutdown() = 0;

        // Called every frame
        virtual void update(float deltaTime) = 0;

        // Optional: Allow plugins to register render passes or interact with rendering
        virtual void registerRenderPasses() {
        };
        // Optional: Allow plugins to register components/systems
        virtual void registerComponents() {
        };
        // Optional: Allow plugins to connect to events
        virtual void connectEvents() {
        };

        // Provide access for plugins if needed (use with caution)
        ApplicationContext *getCtx() { return context; }

    protected:
        ApplicationContext *context = nullptr; // Set during initialize
    };
}

#endif //IPLUGIN_H
