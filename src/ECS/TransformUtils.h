//
// Created by alex on 4/18/25.
//

#ifndef TRANSFORMUTILS_H
#define TRANSFORMUTILS_H

#include "TransformComponent.h"

namespace Bcg::TransformUtils {
    void update(TransformComponent &transform);

    void setFromMatrix(TransformComponent &transform, const Eigen::Affine3f &model_matrix);

    void preApply(TransformComponent &transform, const Eigen::Affine3f &m);

    void postApply(TransformComponent &transform, const Eigen::Affine3f &m);

    void translate(TransformComponent &transform, const Translation &translation);

    void rotate(TransformComponent &transform, const Rotation &rotation,
                const Vector3f &pivot_point = Vector3f::Zero());

    void scale(TransformComponent &transform, const Sscaling &scaling);
}

#endif //TRANSFORMUTILS_H
