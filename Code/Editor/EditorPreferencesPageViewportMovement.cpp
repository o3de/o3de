/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorPreferencesPageViewportMovement.h"

#include <AzQtComponents/Components/StyleManager.h>

// Editor
#include "Settings.h"
#include "EditorViewportSettings.h"

void CEditorPreferencesPage_ViewportMovement::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<CameraMovementSettings>()
        ->Version(1)
        ->Field("MoveSpeed", &CameraMovementSettings::m_moveSpeed)
        ->Field("RotateSpeed", &CameraMovementSettings::m_rotateSpeed)
        ->Field("FastMoveSpeed", &CameraMovementSettings::m_fastMoveSpeed)
        ->Field("WheelZoomSpeed", &CameraMovementSettings::m_wheelZoomSpeed)
        ->Field("InvertYAxis", &CameraMovementSettings::m_invertYRotation)
        ->Field("InvertPan", &CameraMovementSettings::m_invertPan);

    serialize.Class<CEditorPreferencesPage_ViewportMovement>()
        ->Version(1)
        ->Field("CameraMovementSettings", &CEditorPreferencesPage_ViewportMovement::m_cameraMovementSettings);


    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        editContext->Class<CameraMovementSettings>("Camera Movement Settings", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_moveSpeed, "Camera Movement Speed", "Camera Movement Speed")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_rotateSpeed, "Camera Rotation Speed", "Camera Rotation Speed")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_fastMoveSpeed, "Fast Movement Scale", "Fast Movement Scale (holding shift")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_wheelZoomSpeed, "Wheel Zoom Speed", "Wheel Zoom Speed")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &CameraMovementSettings::m_invertYRotation, "Invert Y Axis", "Invert Y Rotation (holding RMB)")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &CameraMovementSettings::m_invertPan, "Invert Pan", "Invert Pan (holding MMB)");

        editContext->Class<CEditorPreferencesPage_ViewportMovement>("Gizmo Movement Preferences", "Gizmo Movement Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_ViewportMovement::m_cameraMovementSettings, "Camera Movement Settings", "Camera Movement Settings");
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
    if (SandboxEditor::UsingNewCameraSystem())
    {
        SandboxEditor::SetCameraTranslateSpeed(m_cameraMovementSettings.m_moveSpeed);
        SandboxEditor::SetCameraRotateSpeed(m_cameraMovementSettings.m_rotateSpeed);
        SandboxEditor::SetCameraBoostMultiplier(m_cameraMovementSettings.m_fastMoveSpeed);
        SandboxEditor::SetCameraScrollSpeed(m_cameraMovementSettings.m_wheelZoomSpeed);
        SandboxEditor::SetCameraOrbitYawRotationInverted(m_cameraMovementSettings.m_invertYRotation);
        SandboxEditor::SetCameraPanInvertedX(m_cameraMovementSettings.m_invertPan);
        SandboxEditor::SetCameraPanInvertedY(m_cameraMovementSettings.m_invertPan);
    }
    else
    {
        gSettings.cameraMoveSpeed = m_cameraMovementSettings.m_moveSpeed;
        gSettings.cameraRotateSpeed = m_cameraMovementSettings.m_rotateSpeed;
        gSettings.cameraFastMoveSpeed = m_cameraMovementSettings.m_fastMoveSpeed;
        gSettings.wheelZoomSpeed = m_cameraMovementSettings.m_wheelZoomSpeed;
        gSettings.invertYRotation = m_cameraMovementSettings.m_invertYRotation;
        gSettings.invertPan = m_cameraMovementSettings.m_invertPan;
    }
}

void CEditorPreferencesPage_ViewportMovement::InitializeSettings()
{
    if (SandboxEditor::UsingNewCameraSystem())
    {
        m_cameraMovementSettings.m_moveSpeed = SandboxEditor::CameraTranslateSpeed();
        m_cameraMovementSettings.m_rotateSpeed = SandboxEditor::CameraRotateSpeed();
        m_cameraMovementSettings.m_fastMoveSpeed = SandboxEditor::CameraBoostMultiplier();
        m_cameraMovementSettings.m_wheelZoomSpeed = SandboxEditor::CameraScrollSpeed();
        m_cameraMovementSettings.m_invertYRotation = SandboxEditor::CameraOrbitYawRotationInverted();
        m_cameraMovementSettings.m_invertPan = SandboxEditor::CameraPanInvertedX() && SandboxEditor::CameraPanInvertedY();
    }
    else
    {
        m_cameraMovementSettings.m_moveSpeed = gSettings.cameraMoveSpeed;
        m_cameraMovementSettings.m_rotateSpeed = gSettings.cameraRotateSpeed;
        m_cameraMovementSettings.m_fastMoveSpeed = gSettings.cameraFastMoveSpeed;
        m_cameraMovementSettings.m_wheelZoomSpeed = gSettings.wheelZoomSpeed;
        m_cameraMovementSettings.m_invertYRotation = gSettings.invertYRotation;
        m_cameraMovementSettings.m_invertPan = gSettings.invertPan;
    }
}
