//
// Created by alex on 4/10/25.
//

#ifndef MANAGER_H
#define MANAGER_H

#include "ApplicationContext.h"

namespace Bcg {
    class Manager {
    public:
        explicit Manager() = default;

        virtual ~Manager() = default;

        virtual void initialize(ApplicationContext *context) = 0;

        virtual void shutdown() = 0;

    protected:
        ApplicationContext *context; // Context for accessing application resources
    };
}


#endif //MANAGER_H
