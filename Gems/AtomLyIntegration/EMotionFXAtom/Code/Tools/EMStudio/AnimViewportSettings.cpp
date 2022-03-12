/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <EMStudio/AnimViewportSettings.h>

namespace EMStudio::ViewportUtil
{
    constexpr AZStd::string_view CameraRotateSmoothnessSetting = "/Amazon/Preferences/Editor/Camera/RotateSmoothness";
    constexpr AZStd::string_view CameraTranslateSmoothnessSetting = "/Amazon/Preferences/Editor/Camera/TranslateSmoothness";
    constexpr AZStd::string_view CameraTranslateSmoothingSetting = "/Amazon/Preferences/Editor/Camera/TranslateSmoothing";
    constexpr AZStd::string_view CameraRotateSmoothingSetting = "/Amazon/Preferences/Editor/Camera/RotateSmoothing";

    constexpr AZStd::string_view CameraOrbitLookIdSetting = "/Amazon/Preferences/Editor/Camera/OrbitLookId";
    constexpr AZStd::string_view CameraTranslateForwardIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateForwardId";
    constexpr AZStd::string_view CameraTranslateBackwardIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateBackwardId";
    constexpr AZStd::string_view CameraTranslateLeftIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateLeftId";
    constexpr AZStd::string_view CameraTranslateRightIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateRightId";
    constexpr AZStd::string_view CameraTranslateUpIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateUpId";
    constexpr AZStd::string_view CameraTranslateDownIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateUpDownId";
    constexpr AZStd::string_view CameraTranslateBoostIdSetting = "/Amazon/Preferences/Editor/Camera/TranslateBoostId";

    AzFramework::TranslateCameraInputChannelIds BuildTranslateCameraInputChannelIds()
    {
        AzFramework::TranslateCameraInputChannelIds translateCameraInputChannelIds;
        translateCameraInputChannelIds.m_leftChannelId =
            AzFramework::InputChannelId(GetRegistry(CameraTranslateLeftIdSetting, AZStd::string("keyboard_key_alphanumeric_A")).c_str());
        translateCameraInputChannelIds.m_rightChannelId =
            AzFramework::InputChannelId(GetRegistry(CameraTranslateRightIdSetting, AZStd::string("keyboard_key_alphanumeric_D")).c_str());
        translateCameraInputChannelIds.m_forwardChannelId =
            AzFramework::InputChannelId(GetRegistry(CameraTranslateForwardIdSetting, AZStd::string("keyboard_key_alphanumeric_W")).c_str());
        translateCameraInputChannelIds.m_backwardChannelId = AzFramework::InputChannelId(
            GetRegistry(CameraTranslateBackwardIdSetting, AZStd::string("keyboard_key_alphanumeric_S")).c_str());
        translateCameraInputChannelIds.m_upChannelId =
            AzFramework::InputChannelId(GetRegistry(CameraTranslateUpIdSetting, AZStd::string("keyboard_key_alphanumeric_E")).c_str());
        translateCameraInputChannelIds.m_downChannelId =
            AzFramework::InputChannelId(GetRegistry(CameraTranslateDownIdSetting, AZStd::string("keyboard_key_alphanumeric_Q")).c_str());
        translateCameraInputChannelIds.m_boostChannelId =
            AzFramework::InputChannelId(GetRegistry(CameraTranslateBoostIdSetting, AZStd::string("keyboard_key_modifier_shift_l")).c_str());

        return translateCameraInputChannelIds;
    }

    float CameraRotateSmoothness()
    {
        return aznumeric_cast<float>(GetRegistry(CameraRotateSmoothnessSetting, 5.0));
    }

    float CameraTranslateSmoothness()
    {
        return aznumeric_cast<float>(GetRegistry(CameraTranslateSmoothnessSetting, 5.0));
    }

    bool CameraRotateSmoothingEnabled()
    {
        return GetRegistry(CameraRotateSmoothingSetting, true);
    }

    bool CameraTranslateSmoothingEnabled()
    {
        return GetRegistry(CameraTranslateSmoothingSetting, true);
    }

    AzFramework::InputChannelId BuildRotateCameraInputId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraOrbitLookIdSetting, AZStd::string("mouse_button_left")).c_str());
    }
}
