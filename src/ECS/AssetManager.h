//
// Created by alex on 4/24/25.
//

#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <Eigen/src/Core/Matrix.h>

#include "Manager.h"
#include "MatVec.h"
#include <entt/resource/cache.hpp>
#include <entt/resource/resource.hpp>

namespace Bcg {
    struct Mesh {
        std::vector<Vector3f> vertices;
        std::vector<Vector3f> normals;
        std::vector<Vector3f> uvs;
        std::vector<Vector3i> faces;
    };

    struct Material {

    };

    struct Texture {

    };

    struct Shader {

    };

    class AssetManager : public Manager {
    public:
        entt::resource<Mesh> loadMesh(const std::string &path);
        entt::resource<Material> loadMaterial(const std::string &path);
        entt::resource<Texture> loadTexture(const std::string &path);

    private:
    };
}

#endif //ASSETMANAGER_H
