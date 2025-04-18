//
// Created by alex on 4/15/25.
//

#include <imgui.h>
#include <sstream>

#include "UICameraComponent.h"
#include "CameraUtils.h"

namespace Bcg {
    inline void UICameraMatrices(const CameraParametersComponent &camera) {
        ImGui::Text("View Matrix:");
        std::stringstream ss_view;
        ss_view << camera.viewMatrix.matrix();
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 400.0f); // Optional: Wrap long matrix text
        ImGui::TextUnformatted(ss_view.str().c_str()); // Use TextUnformatted for potentially long strings
        ImGui::PopTextWrapPos();
        ImGui::Separator();

        ImGui::Text("Projection Matrix:");
        std::stringstream ss_proj;
        ss_proj << camera.projectionMatrix;
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 400.0f);
        ImGui::TextUnformatted(ss_proj.str().c_str());
        ImGui::PopTextWrapPos();
    }

    void UICameraComponent(const CameraParametersComponent &camera) {
        ImGui::Text("Camera Parameters");
        ImGui::Separator();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", camera.position.x(), camera.position.y(), camera.position.z());
        ImGui::Text("Target: (%.2f, %.2f, %.2f)", camera.target.x(), camera.target.y(), camera.target.z());
        ImGui::Text("Up: (%.2f, %.2f, %.2f)", camera.up.x(), camera.up.y(), camera.up.z());
        ImGui::Separator();
        ImGui::Text("FOV: %.2f degrees", camera.fovYDegrees);
        ImGui::Text("Aspect Ratio: %.2f", camera.aspectRatio);
        ImGui::Text("Near Plane: %.2f", camera.nearPlane);
        ImGui::Text("Far Plane: %.2f", camera.farPlane);
        ImGui::Separator();
        ImGui::Text("Movement Speed: %.2f", camera.movementSpeed);
        ImGui::Text("Zoom Sensitivity: %.2f", camera.zoomSensitivity);
        ImGui::Text("Distance to Target: %.2f", camera.distance);
        if (ImGui::CollapsingHeader("Matrices")) {
            UICameraMatrices(camera);
        }
    }

    bool UICameraComponent(CameraParametersComponent &camera) {
        ImGui::Text("Camera Parameters");
        ImGui::Separator();
        bool modifiedView = false;
        modifiedView |= ImGui::InputFloat3("Position", camera.position.data());
        modifiedView |= ImGui::InputFloat3("Target", camera.target.data());
        modifiedView |= ImGui::InputFloat3("Up", camera.up.data());
        camera.dirtyView |= modifiedView;

        bool modifiedProj = false;
        modifiedProj |= ImGui::SliderFloat("FOV (Degrees)", &camera.fovYDegrees, 10.0f, 120.0f);
        ImGui::Text("Aspect Ratio: %.2f", camera.aspectRatio);
        modifiedProj |= ImGui::DragFloat("Near Plane", &camera.nearPlane, 0.01f, 0.001f, camera.farPlane - 0.01f,
                                         "%.3f");
        modifiedProj |= ImGui::DragFloat("Far Plane", &camera.farPlane, 0.1f, camera.nearPlane + 0.01f, 10000.0f,
                                         "%.1f");
        if (modifiedProj) {
            camera.nearPlane = std::min(camera.nearPlane, camera.farPlane - 0.001f);
            camera.farPlane = std::max(camera.farPlane, camera.nearPlane + 0.001f);
            camera.dirtyProjection = modifiedProj;
        }
        ImGui::SliderFloat("Movement Speed: %.2f", &camera.movementSpeed, 0.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("Zoom Sensitivity: %.2f", &camera.zoomSensitivity, 0.0f, 10.0f, "%.2f");
        if (ImGui::SliderFloat("Distance to Target: %.2f", &camera.distance, 0.1f, 1000.0f, "%.2f")) {
            CameraUtils::setDistanceToTarget(camera, camera.distance);
        }
        if (ImGui::CollapsingHeader("Matrices")) {
            UICameraMatrices(camera);
            if (ImGui::Button("Reset")) {
                camera.position = Vector3f(0.0f, 0.0f, 5.0f);
                camera.target = Vector3f::Zero();
                camera.up = Vector3f(0.0f, 1.0f, 0.0f);
                camera.dirtyView = true;

                camera.fovYDegrees = 45.0f;
                camera.aspectRatio = 1.0f;
                camera.nearPlane = 0.1f;
                camera.farPlane = 100.0f;

                camera.movementSpeed = 1.0f;
                camera.zoomSensitivity = 0.5f;
                camera.distance = 1.0f;
                camera.dirtyProjection = true;
            }
        }
        return modifiedView || modifiedProj;
    }
}
