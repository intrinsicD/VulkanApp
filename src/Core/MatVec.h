//
// Created by alex on 4/10/25.
//

#ifndef MATVEC_H
#define MATVEC_H

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace Bcg {
    using Vector2f = Eigen::Vector2f;
    using Vector3f = Eigen::Vector3f;
    using Vector4f = Eigen::Vector4f;

    using Vector2d = Eigen::Vector2d;
    using Vector3d = Eigen::Vector3d;
    using Vector4d = Eigen::Vector4d;

    using Vector2i = Eigen::Vector2i;
    using Vector3i = Eigen::Vector3i;
    using Vector4i = Eigen::Vector4i;

    using Matrix2f = Eigen::Matrix2f;
    using Matrix3f = Eigen::Matrix3f;
    using Matrix4f = Eigen::Matrix4f;

    using Matrix2d = Eigen::Matrix2d;
    using Matrix3d = Eigen::Matrix3d;
    using Matrix4d = Eigen::Matrix4d;

    using Quaternionf = Eigen::Quaternion<float>;
    using Quaterniond = Eigen::Quaternion<double>;

    using Rotation = Eigen::AngleAxisf;
    using Translation = Vector3f;
    using Sscaling = Vector3f;

    // radians
    template<typename T>
    constexpr T radians(T degrees) {
        return degrees * static_cast<T>(0.01745329251994329576923690768489);
    }

    // degrees
    template<typename T>
    constexpr T degrees(T radians) {
        return radians * static_cast<T>(57.295779513082320876798154814105);
    }

}

#endif //MATVEC_H
