//
// Created by alex on 4/18/25.
//

#include "TransformUtils.h"

namespace Bcg::TransformUtils{

    void update(TransformComponent &transform) {
        if (!transform.dirty) {
            return;
        }

        Eigen::Affine3f t(Eigen::Translation3f(transform.position));
        Eigen::Affine3f r(transform.rotation);
        Eigen::Affine3f s(Eigen::Scaling(transform.scale));

        transform.cachedModelMatrix = t * r * s;

        transform.dirty = false;
    }

    void setFromMatrix(TransformComponent &transform, const Eigen::Affine3f &model_matrix) {
        // 1. Extract the translation directly from the last column.
        transform.position = model_matrix.translation();

        // 2. Extract the 3x3 transformation matrix that encodes rotation and scaling.
        Eigen::Matrix3f RS = model_matrix.linear();

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

    void preApply(TransformComponent &transform, const Eigen::Affine3f &m) {
        update(transform);
        setFromMatrix(transform, transform.cachedModelMatrix * m);
    }

    void postApply(TransformComponent &transform, const Eigen::Affine3f &m) {
        update(transform);
        setFromMatrix(transform, m * transform.cachedModelMatrix);
    }

    void translate(TransformComponent &transform, const Translation &delta_translation) {
        transform.position += delta_translation;
        transform.dirty = true;
    }

    void rotate(TransformComponent &transform, const Rotation &delta_rotation,
                                 const Vector3f &pivot_point) {
        transform.position = pivot_point + delta_rotation.toRotationMatrix() * (transform.position - pivot_point);
        transform.rotation = delta_rotation * transform.rotation;
        transform.dirty = true;
    }

    void scale(TransformComponent &transform, const Sscaling &delta_scaling) {
        transform.scale = transform.scale.cwiseProduct(delta_scaling);
        transform.dirty = true;
    }
}