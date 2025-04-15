//
// Created by alex on 4/9/25.
//

#include "TransformSystem.h"

namespace Bcg {
    void TransformSystem::initialize(ApplicationContext *context) {
        this->context = context;
    }

    void TransformSystem::shutdown() {

    }

    void TransformSystem::update(TransformComponent &transform) {
        if (!transform.dirty) {
            return;
        }

        Eigen::Affine3f t(Eigen::Translation3f(transform.position));
        Eigen::Affine3f r(transform.rotation);
        Eigen::Affine3f s(Eigen::Scaling(transform.scale));

        transform.cachedModelMatrix = (t * r * s).matrix();

        transform.dirty = false;
    }

    void TransformSystem::setFromMatrix(TransformComponent &transform, const Matrix4f &model_matrix) {
        // 1. Extract the translation directly from the last column.
        transform.position = model_matrix.block<3, 1>(0, 3);

        // 2. Extract the 3x3 transformation matrix that encodes rotation and scaling.
        Eigen::Matrix3f RS = model_matrix.block<3, 3>(0, 0);

        // 3. Compute the SVD of the 3x3 matrix.
        Eigen::JacobiSVD<Eigen::Matrix3f> svd(RS, Eigen::ComputeFullU | Eigen::ComputeFullV);
        Eigen::Matrix3f U = svd.matrixU();
        Eigen::Matrix3f V = svd.matrixV();
        Eigen::Vector3f sigma = svd.singularValues();

        // 4. Compute the rotation matrix from U and V.
        //    For an ideal transformation, RS = R * S, where S is a diagonal scale matrix.
        //    The best rotation in a least-squares sense is R = U * V^T.
        Eigen::Matrix3f R = U * V.transpose();

        // 5. Correct for any reflection: proper rotation matrices have det(R) == +1.
        if (R.determinant() < 0) {
            // Flip the sign of the last column of U and the corresponding singular value.
            U.col(2) *= -1;
            sigma[2] *= -1;
            R = U * V.transpose();
        }

        // 6. Convert the rotation matrix to an angle-axis representation.
        transform.rotation = Eigen::AngleAxisf(R);

        // 7. The singular values represent the scale factors along the principal axes.
        transform.scale = sigma;
        transform.dirty = true;
    }

    void TransformSystem::preApply(TransformComponent &transform, const Matrix4f &m) {
        update(transform);
        setFromMatrix(transform, transform.cachedModelMatrix * m);
    }

    void TransformSystem::postApply(TransformComponent &transform, const Matrix4f &m) {
        update(transform);
        setFromMatrix(transform, m * transform.cachedModelMatrix);
    }

    void TransformSystem::translate(TransformComponent &transform, const Translation &delta_translation) {
        transform.position += delta_translation;
        transform.dirty = true;
    }

    void TransformSystem::rotate(TransformComponent &transform, const Rotation &delta_rotation,
                                 const Vector3f &pivot_point) {
        transform.position = pivot_point + delta_rotation.toRotationMatrix() * (transform.position - pivot_point);
        transform.rotation = delta_rotation * transform.rotation;
        transform.dirty = true;
    }

    void TransformSystem::scale(TransformComponent &transform, const Sscaling &delta_scaling) {
        transform.scale = transform.scale.cwiseProduct(delta_scaling);
        transform.dirty = true;
    }
}