//
// Created by alex on 4/18/25.
//

#ifndef CAMERAUTILS_H
#define CAMERAUTILS_H

#include "CameraComponent.h"
#include "TransformComponent.h"
#include "Mouse.h"

namespace Bcg::CameraUtils{
     void update(CameraParametersComponent &camera);

     void setDistanceToTarget(CameraParametersComponent &camera, float distance);

     void setFromMatrix(CameraParametersComponent &camera, const Eigen::Affine3f &model_matrix);

     void setFromTransform(CameraParametersComponent &camera, const TransformComponent &model_matrix);

     void zoom(CameraParametersComponent &camera, float delta);

     void arcball(CameraParametersComponent &camera, const Mouse &mouse);
}

#endif //CAMERAUTILS_H
