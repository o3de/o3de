/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "EditorPreferencesPageViewportCamera.h"

#include <AzCore/std/sort.h>
#include <AzFramework/Input/Buses/Requests/InputDeviceRequestBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <EditorModularViewportCameraComposerBus.h>

// Editor
#include "EditorViewportSettings.h"
#include "Settings.h"

namespace EditorPreferencesViewportCameraStrings
{
    static const char* CameraMovementSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Movement Settings");
    static const char* CameraSpeedScaleName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Speed Scale");
    static const char* CameraSpeedScaleDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Overall scale applied to all camera movements");
    static const char* CameraMovementSpeedName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Movement Speed");
    static const char* CameraMovementSpeedDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera movement speed");
    static const char* CameraRotationSpeedName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Rotation Speed");
    static const char* CameraRotationSpeedDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera rotation speed");
    static const char* CameraBoostMultiplierName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Boost Multiplier");
    static const char* CameraBoostMultiplierDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera boost multiplier to apply to movement speed");
    static const char* CameraScrollSpeedName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Scroll Speed");
    static const char* CameraScrollSpeedDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera movement speed while using scroll/wheel input");
    static const char* CameraDollySpeedName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Dolly Speed");
    static const char* CameraDollySpeedDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera movement speed while using mouse motion to move in and out");
    static const char* CameraPanSpeedName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Pan Speed");
    static const char* CameraPanSpeedDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera movement speed while panning using the mouse");
    static const char* CameraRotateSmoothingName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Rotate Smoothing");
    static const char* CameraRotateSmoothingDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Is camera rotation smoothing enabled or disabled");
    static const char* CameraRotateSmoothnessName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Rotate Smoothness");
    static const char* CameraRotateSmoothnessDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Amount of camera smoothing to apply while rotating the camera");
    static const char* CameraTranslateSmoothingName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Translate Smoothing");
    static const char* CameraTranslateSmoothingDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Is camera translation smoothing enabled or disabled");
    static const char* CameraTranslateSmoothnessName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Translate Smoothness");
    static const char* CameraTranslateSmoothnessDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Amount of camera smoothing to apply while translating the camera");
    static const char* CameraOrbitYawInvertedName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Orbit Yaw Inverted");
    static const char* CameraOrbitYawInvertedDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Inverted yaw rotation while orbiting");
    static const char* InvertPanXName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Invert Pan X");
    static const char* InvertPanXDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Invert direction of pan in local X axis");
    static const char* InvertPanYName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Invert Pan Y");
    static const char* InvertPanYDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Invert direction of pan in local Y axis");
    static const char* InvertZoomName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Invert Zoom Direction");
    static const char* InvertZoomDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Invert Mouse Wheel Zoom direction");
    static const char* CameraCaptureLookCursorName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Capture Look Cursor");
    static const char* CameraCaptureLookCursorDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Should the cursor be captured (hidden) while performing free look");
    static const char* DefaultCameraPositionName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Default Camera Position");
    static const char* DefaultCameraPositionDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Default Camera Position when a level is first opened");
    static const char* DefaultCameraOrientationName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Default Camera Orientation");
    static const char* DefaultCameraOrientationDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Default Camera Orientation when a level is first opened (X - Pitch value (degrees), Y - Yaw value (degrees)");
    static const char* DefaultOrbitDistanceName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Default Orbit Distance");
    static const char* DefaultOrbitDistanceDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "The default distance to orbit about when there is no entity selected");
    static const char* CameraGoToPositionDurationName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Go To Position Duration");
    static const char* CameraGoToPositionDurationDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Time it takes for the camera to interpolate to a given position");
    static const char* CameraGoToPositionInstantlyName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Go To Position Instantly");
    static const char* CameraGoToPositionInstantlyDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera will instantly go to the set position and won't interpolate there");
    static const char* RestoreDefaultsMovementDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Restore camera movement settings to defaults");
    static const char* RestoreDefaultsButtonText = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Restore defaults");

    static const char* CameraInputSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Camera Input Settings");
    static const char* TranslateForwardName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Translate Forward");
    static const char* TranslateForwardDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to move the camera forward");
    static const char* TranslateBackwardName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Translate Backward");
    static const char* TranslateBackwardDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to move the camera backward");
    static const char* TranslateLeftName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Translate Left");
    static const char* TranslateLeftDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to move the camera left");
    static const char* TranslateRightName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Translate Right");
    static const char* TranslateRightDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to move the camera right");
    static const char* TranslateUpName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Translate Up");
    static const char* TranslateUpDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to move the camera up");
    static const char* TranslateDownName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Translate Down");
    static const char* TranslateDownDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to move the camera down");
    static const char* BoostName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Boost");
    static const char* BoostDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to move the camera more quickly");
    static const char* OrbitName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Orbit");
    static const char* OrbitDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to begin the camera orbit behavior");
    static const char* FreeLookName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Free Look");
    static const char* FreeLookDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to begin camera free look");
    static const char* FreePanName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Free Pan");
    static const char* FreePanDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to begin camera free pan");
    static const char* OrbitLookName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Orbit Look");
    static const char* OrbitLookDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to begin camera orbit look");
    static const char* OrbitDollyName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Orbit Dolly");
    static const char* OrbitDollyDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to begin camera orbit dolly");
    static const char* OrbitPanName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Orbit Pan");
    static const char* OrbitPanDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to begin camera orbit pan");
    static const char* FocusName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Focus");
    static const char* FocusDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Key/button to focus camera orbit");
    static const char* RestoreDefaultsInputDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Restore camera input settings to defaults");

    static const char* ViewportPreferencesClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageViewportCamera", "Viewport Preferences");
}

static AZStd::vector<AZStd::string> GetInputNamesByDevice(const AzFramework::InputDeviceId inputDeviceId)
{
    AzFramework::InputDeviceRequests::InputChannelIdSet availableInputChannelIds;
    AzFramework::InputDeviceRequestBus::Event(
        inputDeviceId, &AzFramework::InputDeviceRequests::GetInputChannelIds, availableInputChannelIds);

    AZStd::vector<AZStd::string> inputChannelNames;
    for (const AzFramework::InputChannelId& inputChannelId : availableInputChannelIds)
    {
        inputChannelNames.push_back(inputChannelId.GetName());
    }

    AZStd::sort(inputChannelNames.begin(), inputChannelNames.end());

    return inputChannelNames;
}

static AZStd::vector<AZStd::string> GetEditorInputNames()
{
    // function static to defer having to call GetInputNamesByDevice for every CameraInputSettings member
    static bool inputNamesGenerated = false;
    static AZStd::vector<AZStd::string> inputNames;

    if (!inputNamesGenerated)
    {
        AZStd::vector<AZStd::string> keyboardInputNames = GetInputNamesByDevice(AzFramework::InputDeviceKeyboard::Id);
        AZStd::vector<AZStd::string> mouseInputNames = GetInputNamesByDevice(AzFramework::InputDeviceMouse::Id);

        inputNames.insert(inputNames.end(), mouseInputNames.begin(), mouseInputNames.end());
        inputNames.insert(inputNames.end(), keyboardInputNames.begin(), keyboardInputNames.end());

        inputNamesGenerated = true;
    }

    return inputNames;
}

void CEditorPreferencesPage_ViewportCamera::CameraMovementSettings::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<CameraMovementSettings>()
        ->Version(6)
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
        ->Field("PanInvertedY", &CameraMovementSettings::m_panInvertedY)
        ->Field("ZoomInverted", &CameraMovementSettings::m_zoomInverted)
        ->Field("DefaultPosition", &CameraMovementSettings::m_defaultPosition)
        ->Field("DefaultOrientation", &CameraMovementSettings::m_defaultPitchYaw)
        ->Field("DefaultOrbitDistance", &CameraMovementSettings::m_defaultOrbitDistance)
        ->Field("SpeedScale", &CameraMovementSettings::m_speedScale)
        ->Field("GoToPositionInstantly", &CameraMovementSettings::m_goToPositionInstantly)
        ->Field("GoToPositionDuration", &CameraMovementSettings::m_goToPositionDuration)
        ->Field("Reset", &CameraMovementSettings::m_resetButton);

    if (AZ::EditContext* editContext = serialize.GetEditContext())
    {
        using namespace EditorPreferencesViewportCameraStrings;
        const float minValue = 0.0001f;
        editContext->Class<CameraMovementSettings>(CameraMovementSettingsClassName, "")
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_speedScale,
                CameraSpeedScaleName,
                CameraSpeedScaleDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_translateSpeed, CameraMovementSpeedName, CameraMovementSpeedDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &CameraMovementSettings::m_rotateSpeed, CameraRotationSpeedName, CameraRotationSpeedDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_boostMultiplier,
                CameraBoostMultiplierName,
                CameraBoostMultiplierDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_scrollSpeed,
                CameraScrollSpeedName,
                CameraScrollSpeedDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_dollySpeed,
                CameraDollySpeedName,
                CameraDollySpeedDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_panSpeed,
                CameraPanSpeedName,
                CameraPanSpeedDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &CameraMovementSettings::m_rotateSmoothing,
                CameraRotateSmoothingName,
                CameraRotateSmoothingDesc)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_rotateSmoothness,
                CameraRotateSmoothnessName,
                CameraRotateSmoothnessDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->Attribute(AZ::Edit::Attributes::ReadOnly, &CameraMovementSettings::RotateSmoothingReadOnly)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &CameraMovementSettings::m_translateSmoothing,
                CameraTranslateSmoothingName,
                CameraTranslateSmoothingDesc)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_translateSmoothness,
                CameraTranslateSmoothnessName,
                CameraTranslateSmoothnessDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->Attribute(AZ::Edit::Attributes::ReadOnly, &CameraMovementSettings::TranslateSmoothingReadOnly)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &CameraMovementSettings::m_orbitYawRotationInverted,
                CameraOrbitYawInvertedName,
                CameraOrbitYawInvertedDesc)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &CameraMovementSettings::m_panInvertedX,
                InvertPanXName,
                InvertPanXDesc)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &CameraMovementSettings::m_panInvertedY,
                InvertPanYName,
                InvertPanYDesc)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &CameraMovementSettings::m_zoomInverted,
                InvertZoomName,
                InvertZoomDesc)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &CameraMovementSettings::m_captureCursorLook,
                CameraCaptureLookCursorName,
                CameraCaptureLookCursorDesc)
            ->DataElement(
                AZ::Edit::UIHandlers::Vector3,
                &CameraMovementSettings::m_defaultPosition,
                DefaultCameraPositionName,
                DefaultCameraPositionDesc)
            ->DataElement(
                AZ::Edit::UIHandlers::Vector2,
                &CameraMovementSettings::m_defaultPitchYaw,
                DefaultCameraOrientationName,
                DefaultCameraOrientationDesc)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_defaultOrbitDistance,
                DefaultOrbitDistanceName,
                DefaultOrbitDistanceDesc)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox,
                &CameraMovementSettings::m_goToPositionDuration,
                CameraGoToPositionDurationName,
                CameraGoToPositionDurationDesc)
            ->Attribute(AZ::Edit::Attributes::ReadOnly, &CameraMovementSettings::GoToPositionDurationReadOnly)
            ->Attribute(AZ::Edit::Attributes::Min, minValue)
            ->DataElement(
                AZ::Edit::UIHandlers::CheckBox,
                &CameraMovementSettings::m_goToPositionInstantly,
                CameraGoToPositionInstantlyName,
                CameraGoToPositionInstantlyDesc)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
            ->DataElement(
                AZ::Edit::UIHandlers::Button, &CameraMovementSettings::m_resetButton, "", RestoreDefaultsMovementDesc)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CameraMovementSettings::Reset)
            ->Attribute(AZ::Edit::Attributes::ButtonText, RestoreDefaultsButtonText)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues);
    }
}

void CEditorPreferencesPage_ViewportCamera::CameraInputSettings::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<CameraInputSettings>()
        ->Version(2)
        ->Field("TranslateForward", &CameraInputSettings::m_translateForwardChannelId)
        ->Field("TranslateBackward", &CameraInputSettings::m_translateBackwardChannelId)
        ->Field("TranslateLeft", &CameraInputSettings::m_translateLeftChannelId)
        ->Field("TranslateRight", &CameraInputSettings::m_translateRightChannelId)
        ->Field("TranslateUp", &CameraInputSettings::m_translateUpChannelId)
        ->Field("TranslateDown", &CameraInputSettings::m_translateDownChannelId)
        ->Field("Boost", &CameraInputSettings::m_boostChannelId)
        ->Field("Orbit", &CameraInputSettings::m_orbitChannelId)
        ->Field("FreeLook", &CameraInputSettings::m_freeLookChannelId)
        ->Field("FreePan", &CameraInputSettings::m_freePanChannelId)
        ->Field("OrbitLook", &CameraInputSettings::m_orbitLookChannelId)
        ->Field("OrbitDolly", &CameraInputSettings::m_orbitDollyChannelId)
        ->Field("OrbitPan", &CameraInputSettings::m_orbitPanChannelId)
        ->Field("Focus", &CameraInputSettings::m_focusChannelId)
        ->Field("Reset", &CameraInputSettings::m_resetButton);

    if (AZ::EditContext* editContext = serialize.GetEditContext())
    {
        using namespace EditorPreferencesViewportCameraStrings;
        editContext->Class<CameraInputSettings>(CameraInputSettingsClassName, "")
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_translateForwardChannelId,
                TranslateForwardName,
                TranslateForwardDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_translateBackwardChannelId,
                TranslateBackwardName,
                TranslateBackwardDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_translateLeftChannelId,
                TranslateLeftName,
                TranslateLeftDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_translateRightChannelId,
                TranslateRightName,
                TranslateRightDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_translateUpChannelId,
                TranslateUpName,
                TranslateUpDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_translateDownChannelId,
                TranslateDownName,
                TranslateDownDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_boostChannelId,
                BoostName,
                BoostDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_orbitChannelId,
                OrbitName,
                OrbitDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_freeLookChannelId,
                FreeLookName,
                FreeLookDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox, &CameraInputSettings::m_freePanChannelId, FreePanName, FreePanDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_orbitLookChannelId,
                OrbitLookName,
                OrbitLookDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_orbitDollyChannelId,
                OrbitDollyName,
                OrbitDollyDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox,
                &CameraInputSettings::m_orbitPanChannelId,
                OrbitPanName,
                OrbitPanDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::ComboBox, &CameraInputSettings::m_focusChannelId, FocusName, FocusDesc)
            ->Attribute(AZ::Edit::Attributes::StringList, &GetEditorInputNames)
            ->DataElement(
                AZ::Edit::UIHandlers::Button, &CameraInputSettings::m_resetButton, "", RestoreDefaultsInputDesc)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CameraInputSettings::Reset)
            ->Attribute(AZ::Edit::Attributes::ButtonText, RestoreDefaultsButtonText)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues);
    }
}

void CEditorPreferencesPage_ViewportCamera::Reflect(AZ::SerializeContext& serialize)
{
    CameraMovementSettings::Reflect(serialize);
    CameraInputSettings::Reflect(serialize);

    serialize.Class<CEditorPreferencesPage_ViewportCamera>()
        ->Version(1)
        ->Field("CameraMovementSettings", &CEditorPreferencesPage_ViewportCamera::m_cameraMovementSettings)
        ->Field("CameraInputSettings", &CEditorPreferencesPage_ViewportCamera::m_cameraInputSettings);

    if (AZ::EditContext* editContext = serialize.GetEditContext())
    {
        using namespace EditorPreferencesViewportCameraStrings;
        editContext->Class<CEditorPreferencesPage_ViewportCamera>(ViewportPreferencesClassName, ViewportPreferencesClassName)
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(
                AZ::Edit::UIHandlers::Default,
                &CEditorPreferencesPage_ViewportCamera::m_cameraMovementSettings,
                CameraMovementSettingsClassName,
                CameraMovementSettingsClassName)
            ->DataElement(
                AZ::Edit::UIHandlers::Default,
                &CEditorPreferencesPage_ViewportCamera::m_cameraInputSettings,
                CameraInputSettingsClassName,
                CameraInputSettingsClassName);
    }
}

CEditorPreferencesPage_ViewportCamera::CEditorPreferencesPage_ViewportCamera()
{
    InitializeSettings();
    m_icon = QIcon(":/res/Camera.svg");
}

const char* CEditorPreferencesPage_ViewportCamera::GetCategory()
{
    return QT_TRANSLATE_NOOP("EditorPreferencesDialog", "Viewports");
}

const char* CEditorPreferencesPage_ViewportCamera::GetTitle()
{
    return QT_TRANSLATE_NOOP("EditorPreferencesDialog", "Camera");
}

QIcon& CEditorPreferencesPage_ViewportCamera::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_ViewportCamera::OnCancel()
{
    // noop
}

bool CEditorPreferencesPage_ViewportCamera::OnQueryCancel()
{
    return true;
}

void CEditorPreferencesPage_ViewportCamera::OnApply()
{
    SandboxEditor::SetCameraSpeedScale(m_cameraMovementSettings.m_speedScale);
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
    SandboxEditor::SetCameraZoomInverted(m_cameraMovementSettings.m_zoomInverted);
    SandboxEditor::SetCameraDefaultEditorPosition(m_cameraMovementSettings.m_defaultPosition);
    SandboxEditor::SetCameraDefaultOrbitDistance(m_cameraMovementSettings.m_defaultOrbitDistance);
    SandboxEditor::SetCameraDefaultEditorOrientation(m_cameraMovementSettings.m_defaultPitchYaw);
    SandboxEditor::SetCameraGoToPositionInstantlyEnabled(m_cameraMovementSettings.m_goToPositionInstantly);
    SandboxEditor::SetCameraGoToPositionDuration(m_cameraMovementSettings.m_goToPositionDuration);

    SandboxEditor::SetCameraTranslateForwardChannelId(m_cameraInputSettings.m_translateForwardChannelId);
    SandboxEditor::SetCameraTranslateBackwardChannelId(m_cameraInputSettings.m_translateBackwardChannelId);
    SandboxEditor::SetCameraTranslateLeftChannelId(m_cameraInputSettings.m_translateLeftChannelId);
    SandboxEditor::SetCameraTranslateRightChannelId(m_cameraInputSettings.m_translateRightChannelId);
    SandboxEditor::SetCameraTranslateUpChannelId(m_cameraInputSettings.m_translateUpChannelId);
    SandboxEditor::SetCameraTranslateDownChannelId(m_cameraInputSettings.m_translateDownChannelId);
    SandboxEditor::SetCameraTranslateBoostChannelId(m_cameraInputSettings.m_boostChannelId);
    SandboxEditor::SetCameraOrbitChannelId(m_cameraInputSettings.m_orbitChannelId);
    SandboxEditor::SetCameraFreeLookChannelId(m_cameraInputSettings.m_freeLookChannelId);
    SandboxEditor::SetCameraFreePanChannelId(m_cameraInputSettings.m_freePanChannelId);
    SandboxEditor::SetCameraOrbitLookChannelId(m_cameraInputSettings.m_orbitLookChannelId);
    SandboxEditor::SetCameraOrbitDollyChannelId(m_cameraInputSettings.m_orbitDollyChannelId);
    SandboxEditor::SetCameraOrbitPanChannelId(m_cameraInputSettings.m_orbitPanChannelId);
    SandboxEditor::SetCameraFocusChannelId(m_cameraInputSettings.m_focusChannelId);

    SandboxEditor::EditorModularViewportCameraComposerNotificationBus::Broadcast(
        &SandboxEditor::EditorModularViewportCameraComposerNotificationBus::Events::OnEditorModularViewportCameraComposerSettingsChanged);
}

void CEditorPreferencesPage_ViewportCamera::InitializeSettings()
{
    m_cameraMovementSettings.Initialize();
    m_cameraInputSettings.Initialize();
}

void CEditorPreferencesPage_ViewportCamera::CameraMovementSettings::Reset()
{
    SandboxEditor::ResetCameraSpeedScale();
    SandboxEditor::ResetCameraTranslateSpeed();
    SandboxEditor::ResetCameraRotateSpeed();
    SandboxEditor::ResetCameraBoostMultiplier();
    SandboxEditor::ResetCameraScrollSpeed();
    SandboxEditor::ResetCameraDollyMotionSpeed();
    SandboxEditor::ResetCameraPanSpeed();
    SandboxEditor::ResetCameraRotateSmoothness();
    SandboxEditor::ResetCameraRotateSmoothingEnabled();
    SandboxEditor::ResetCameraTranslateSmoothness();
    SandboxEditor::ResetCameraTranslateSmoothingEnabled();
    SandboxEditor::ResetCameraCaptureCursorForLook();
    SandboxEditor::ResetCameraOrbitYawRotationInverted();
    SandboxEditor::ResetCameraPanInvertedX();
    SandboxEditor::ResetCameraPanInvertedY();
    SandboxEditor::ResetCameraZoomInverted();
    SandboxEditor::ResetCameraDefaultEditorPosition();
    SandboxEditor::ResetCameraDefaultOrbitDistance();
    SandboxEditor::ResetCameraDefaultEditorOrientation();
    SandboxEditor::ResetCameraGoToPositionInstantlyEnabled();
    SandboxEditor::ResetCameraGoToPositionDuration();

    Initialize();
}

void CEditorPreferencesPage_ViewportCamera::CameraMovementSettings::Initialize()
{
    m_speedScale = SandboxEditor::CameraSpeedScale();
    m_translateSpeed = SandboxEditor::CameraTranslateSpeed();
    m_rotateSpeed = SandboxEditor::CameraRotateSpeed();
    m_boostMultiplier = SandboxEditor::CameraBoostMultiplier();
    m_scrollSpeed = SandboxEditor::CameraScrollSpeed();
    m_dollySpeed = SandboxEditor::CameraDollyMotionSpeed();
    m_panSpeed = SandboxEditor::CameraPanSpeed();
    m_rotateSmoothness = SandboxEditor::CameraRotateSmoothness();
    m_rotateSmoothing = SandboxEditor::CameraRotateSmoothingEnabled();
    m_translateSmoothness = SandboxEditor::CameraTranslateSmoothness();
    m_translateSmoothing = SandboxEditor::CameraTranslateSmoothingEnabled();
    m_captureCursorLook = SandboxEditor::CameraCaptureCursorForLook();
    m_orbitYawRotationInverted = SandboxEditor::CameraOrbitYawRotationInverted();
    m_panInvertedX = SandboxEditor::CameraPanInvertedX();
    m_panInvertedY = SandboxEditor::CameraPanInvertedY();
    m_zoomInverted = SandboxEditor::CameraZoomInverted();
    m_defaultPosition = SandboxEditor::CameraDefaultEditorPosition();
    m_defaultOrbitDistance = SandboxEditor::CameraDefaultOrbitDistance();
    m_defaultPitchYaw = SandboxEditor::CameraDefaultEditorOrientation();
    m_goToPositionInstantly = SandboxEditor::CameraGoToPositionInstantlyEnabled();
    m_goToPositionDuration = SandboxEditor::CameraGoToPositionDuration();
}

void CEditorPreferencesPage_ViewportCamera::CameraInputSettings::Reset()
{
    SandboxEditor::ResetCameraTranslateForwardChannelId();
    SandboxEditor::ResetCameraTranslateBackwardChannelId();
    SandboxEditor::ResetCameraTranslateLeftChannelId();
    SandboxEditor::ResetCameraTranslateRightChannelId();
    SandboxEditor::ResetCameraTranslateUpChannelId();
    SandboxEditor::ResetCameraTranslateDownChannelId();
    SandboxEditor::ResetCameraTranslateBoostChannelId();
    SandboxEditor::ResetCameraOrbitChannelId();
    SandboxEditor::ResetCameraFreeLookChannelId();
    SandboxEditor::ResetCameraFreePanChannelId();
    SandboxEditor::ResetCameraOrbitLookChannelId();
    SandboxEditor::ResetCameraOrbitDollyChannelId();
    SandboxEditor::ResetCameraOrbitPanChannelId();
    SandboxEditor::ResetCameraFocusChannelId();

    Initialize();
}

void CEditorPreferencesPage_ViewportCamera::CameraInputSettings::Initialize()
{
    m_translateForwardChannelId = SandboxEditor::CameraTranslateForwardChannelId().GetName();
    m_translateBackwardChannelId = SandboxEditor::CameraTranslateBackwardChannelId().GetName();
    m_translateLeftChannelId = SandboxEditor::CameraTranslateLeftChannelId().GetName();
    m_translateRightChannelId = SandboxEditor::CameraTranslateRightChannelId().GetName();
    m_translateUpChannelId = SandboxEditor::CameraTranslateUpChannelId().GetName();
    m_translateDownChannelId = SandboxEditor::CameraTranslateDownChannelId().GetName();
    m_boostChannelId = SandboxEditor::CameraTranslateBoostChannelId().GetName();
    m_orbitChannelId = SandboxEditor::CameraOrbitChannelId().GetName();
    m_freeLookChannelId = SandboxEditor::CameraFreeLookChannelId().GetName();
    m_freePanChannelId = SandboxEditor::CameraFreePanChannelId().GetName();
    m_orbitLookChannelId = SandboxEditor::CameraOrbitLookChannelId().GetName();
    m_orbitDollyChannelId = SandboxEditor::CameraOrbitDollyChannelId().GetName();
    m_orbitPanChannelId = SandboxEditor::CameraOrbitPanChannelId().GetName();
    m_focusChannelId = SandboxEditor::CameraFocusChannelId().GetName();
}
