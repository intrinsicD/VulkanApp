//
// Created by alex on 4/10/25.
//

#ifndef SYSTEM_H
#define SYSTEM_H

#include "ApplicationContext.h"

namespace Bcg{
    class System{
      public:
        System() = default;

        virtual ~System() = default;

        virtual void initialize(ApplicationContext *context) = 0;

        virtual void shutdown() = 0;

      protected:
        ApplicationContext *context; // Context for accessing application resources
    };
}

#endif //SYSTEM_H
