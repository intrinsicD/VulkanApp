//
// Created by alex on 4/25/25.
//

#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include "Manager.h"
#include "entt/resource/resource.hpp"

namespace Bcg {
    struct Shader {

    };
    class ShaderManager : public Manager {
    public:
        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        entt::resource<Shader> loadShader(const std::string &path);

    private:
    };
}

#endif //SHADERMANAGER_H
