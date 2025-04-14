//
// Created by alex on 4/9/25.
//

#ifndef TRANSFORMSYSTEM_H
#define TRANSFORMSYSTEM_H

#include "System.h"
#include "TransformComponent.h"

namespace Bcg {
    class TransformSystem : public System {
    public:
        ~TransformSystem() override = default;

        void initialize(ApplicationContext *context) override;

        void shutdown() override;

        void update(TransformComponent &transform);

        void setFromMatrix(TransformComponent &transform, const Matrix4f &model_matrix);

        void preApply(TransformComponent &transform, const Matrix4f &m);

        void postApply(TransformComponent &transform, const Matrix4f &m);

        void translate(TransformComponent &transform, const Translation &translation);

        void rotate(TransformComponent &transform, const Rotation &rotation,
                    const Vector3f &pivot_point = Vector3f::Zero());

        void scale(TransformComponent &transform, const Sscaling &scaling);
    };
}

#endif //TRANSFORMSYSTEM_H
