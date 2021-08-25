/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "EditorPreferencesPageViewportMovement.h"

#include <AzQtComponents/Components/StyleManager.h>

// Editor
#include "EditorViewportSettings.h"
#include "Settings.h"

void CEditorPreferencesPage_ViewportMovement::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<CameraMovementSettings>()
        ->Version(2)
        ->Field("TranslateSpeed", &CameraMovementSettings::m_translateSpeed)
        ->Field("RotateSpeed", &CameraMovementSettings::m_rotateSpeed)
        ->Field("BoostMultiplier", &CameraMovementSettings::m_boostMultiplier)
        ->Field("ScrollSpeed", &CameraMovementSettings::m_scrollSpeed)
        ->Field("DollySpeed", &CameraMovementSettings::m_dollySpeed)
        ->Field("PanSpeed", &CameraMovementSettings::m_panSpeed)
        ->Field("RotateSmoothing", &CameraMovementSettings::m_rotateSmoothing)
        ->Field("RotateSmoothness", &CameraMovementSettings::m_rotateSmoothness)
        ->Field("TranslateSmoothing", &CameraMovementSettings::m_translateSmoothing)
        ->Field("TranslateSmoothness", &CameraMovementSettings::m_translateSmoothness)
        ->Field("CaptureCursorLook", &CameraMovementSettings::m_captureCursorLook)
        ->Field("OrbitYawRotationInverted", &CameraMovementSettings::m_orbitYawRotationInverted)
        ->Field("PanInvertedX", &CameraMovementSettings::m_panInvertedX)
        ->Field("PanInvertedY", &CameraMovementSettings::m_panInvertedY);

    serialize.Class<CEditorPreferencesPage_ViewportMovement>()->Version(1)->Field(
        "CameraMovementSettings", &CEditorPreferencesPage_ViewportMovement::m_cameraMovementSettings);

    if (AZ::EditContext* editContext = serialize.GetEditContext())
    {
        editContext->Class<CameraMovementSettings>("Camera Settings", "")
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_translateSpeed, "Camera Movement Speed", "Camera movement speed")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_rotateSpeed, "Camera Rotation Speed", "Camera rotation speed")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_boostMultiplier, "Camera Boost Multiplier",
                "Camera boost multiplier to apply to movement speed")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_scrollSpeed, "Camera Scroll Speed",
                "Camera movement speed while using scroll/wheel input")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_dollySpeed, "Camera Dolly Speed",
                "Camera movement speed while using mouse motion to move in and out")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_panSpeed, "Camera Pan Speed",
                "Camera movement speed while panning using the mouse")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &CameraMovementSettings::m_rotateSmoothing, "Camera Rotate Smoothing",
                "Is camera rotation smoothing enabled or disabled")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_rotateSmoothness, "Camera Rotate Smoothness",
                "Amount of camera smoothing to apply while rotating the camera")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->Attribute(AZ::Edit::Attributes::Visibility, &CameraMovementSettings::RotateSmoothingVisibility)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &CameraMovementSettings::m_translateSmoothing, "Camera Translate Smoothing",
                "Is camera translation smoothing enabled or disabled")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_translateSmoothness, "Camera Translate Smoothness",
                "Amount of camera smoothing to apply while translating the camera")
            ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
            ->Attribute(AZ::Edit::Attributes::Visibility, &CameraMovementSettings::TranslateSmoothingVisibility)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &CameraMovementSettings::m_orbitYawRotationInverted, "Camera Orbit Yaw Inverted",
                "Inverted yaw rotation while orbiting")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &CameraMovementSettings::m_panInvertedX, "Invert Pan X",
                "Invert direction of pan in local X axis")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &CameraMovementSettings::m_panInvertedY, "Invert Pan Y",
                "Invert direction of pan in local Y axis")
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox, &CameraMovementSettings::m_captureCursorLook, "Camera Capture Look Cursor",
                "Should the cursor be captured (hidden) while performing free look");

        editContext->Class<CEditorPreferencesPage_ViewportMovement>("Viewport Preferences", "Viewport Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(
                AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportMovement::m_cameraMovementSettings,
                "Camera Movement Settings", "Camera Movement Settings");
    }
}

CEditorPreferencesPage_ViewportMovement::CEditorPreferencesPage_ViewportMovement()
{
    InitializeSettings();
    m_icon = QIcon(":/res/Camera.svg");
}

const char* CEditorPreferencesPage_ViewportMovement::GetTitle()
{
    return "Camera";
}

QIcon& CEditorPreferencesPage_ViewportMovement::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_ViewportMovement::OnApply()
{
    SandboxEditor::SetCameraTranslateSpeed(m_cameraMovementSettings.m_translateSpeed);
    SandboxEditor::SetCameraRotateSpeed(m_cameraMovementSettings.m_rotateSpeed);
    SandboxEditor::SetCameraBoostMultiplier(m_cameraMovementSettings.m_boostMultiplier);
    SandboxEditor::SetCameraScrollSpeed(m_cameraMovementSettings.m_scrollSpeed);
    SandboxEditor::SetCameraDollyMotionSpeed(m_cameraMovementSettings.m_dollySpeed);
    SandboxEditor::SetCameraPanSpeed(m_cameraMovementSettings.m_panSpeed);
    SandboxEditor::SetCameraRotateSmoothness(m_cameraMovementSettings.m_rotateSmoothness);
    SandboxEditor::SetCameraRotateSmoothingEnabled(m_cameraMovementSettings.m_rotateSmoothing);
    SandboxEditor::SetCameraTranslateSmoothness(m_cameraMovementSettings.m_translateSmoothness);
    SandboxEditor::SetCameraTranslateSmoothingEnabled(m_cameraMovementSettings.m_translateSmoothing);
    SandboxEditor::SetCameraCaptureCursorForLook(m_cameraMovementSettings.m_captureCursorLook);
    SandboxEditor::SetCameraOrbitYawRotationInverted(m_cameraMovementSettings.m_orbitYawRotationInverted);
    SandboxEditor::SetCameraPanInvertedX(m_cameraMovementSettings.m_panInvertedX);
    SandboxEditor::SetCameraPanInvertedY(m_cameraMovementSettings.m_panInvertedY);
}

void CEditorPreferencesPage_ViewportMovement::InitializeSettings()
{
    m_cameraMovementSettings.m_translateSpeed = SandboxEditor::CameraTranslateSpeed();
    m_cameraMovementSettings.m_rotateSpeed = SandboxEditor::CameraRotateSpeed();
    m_cameraMovementSettings.m_boostMultiplier = SandboxEditor::CameraBoostMultiplier();
    m_cameraMovementSettings.m_scrollSpeed = SandboxEditor::CameraScrollSpeed();
    m_cameraMovementSettings.m_dollySpeed = SandboxEditor::CameraDollyMotionSpeed();
    m_cameraMovementSettings.m_panSpeed = SandboxEditor::CameraPanSpeed();
    m_cameraMovementSettings.m_rotateSmoothness = SandboxEditor::CameraRotateSmoothness();
    m_cameraMovementSettings.m_rotateSmoothing = SandboxEditor::CameraRotateSmoothingEnabled();
    m_cameraMovementSettings.m_translateSmoothness = SandboxEditor::CameraTranslateSmoothness();
    m_cameraMovementSettings.m_translateSmoothing = SandboxEditor::CameraTranslateSmoothingEnabled();
    m_cameraMovementSettings.m_captureCursorLook = SandboxEditor::CameraCaptureCursorForLook();
    m_cameraMovementSettings.m_orbitYawRotationInverted = SandboxEditor::CameraOrbitYawRotationInverted();
    m_cameraMovementSettings.m_panInvertedX = SandboxEditor::CameraPanInvertedX();
    m_cameraMovementSettings.m_panInvertedY = SandboxEditor::CameraPanInvertedY();
}
