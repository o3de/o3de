/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>

#include <EMStudio/AnimViewportWidget.h>
#include <EMStudio/AnimViewportRenderer.h>

namespace EMStudio
{
    AnimViewportWidget::AnimViewportWidget(QWidget* parent)
        : AtomToolsFramework::RenderViewportWidget(parent)
    {
        setObjectName(QString::fromUtf8("AtomViewportWidget"));
        QSizePolicy qSize(QSizePolicy::Preferred, QSizePolicy::Preferred);
        qSize.setHorizontalStretch(0);
        qSize.setVerticalStretch(0);
        qSize.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(qSize);
        setAutoFillBackground(false);
        setStyleSheet(QString::fromUtf8(""));

        m_renderer = AZStd::make_unique<AnimViewportRenderer>(GetViewportContext());

        SetupCameras();
        SetupCameraController();
    }

    template<typename T>
    AZStd::remove_cvref_t<T> GetRegistry(const AZStd::string_view setting, T&& defaultValue)
    {
        AZStd::remove_cvref_t<T> value = AZStd::forward<T>(defaultValue);
        if (const auto* registry = AZ::SettingsRegistry::Get())
        {
            T potentialValue;
            if (registry->Get(potentialValue, setting))
            {
                value = AZStd::move(potentialValue);
            }
        }

        return value;
    }

    // TODO: TranslateCameraInput should have a way to initiate with default channel id setup.
    static AzFramework::TranslateCameraInputChannelIds BuildTranslateCameraInputChannelIds()
    {
        constexpr AZStd::string_view CameraTranslateForwardIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateForwardId";
        constexpr AZStd::string_view CameraTranslateBackwardIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateBackwardId";
        constexpr AZStd::string_view CameraTranslateLeftIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateLeftId";
        constexpr AZStd::string_view CameraTranslateRightIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateRightId";
        constexpr AZStd::string_view CameraTranslateUpIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateUpId";
        constexpr AZStd::string_view CameraTranslateDownIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateUpDownId";
        constexpr AZStd::string_view CameraTranslateBoostIdSetting = "/Amazon/Preferences/Editor/Camera/TranslateBoostId";

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

    void AnimViewportWidget::SetupCameras()
    {
        constexpr AZStd::string_view CameraOrbitIdSetting = "/Amazon/Preferences/Editor/Camera/OrbitId";
        constexpr AZStd::string_view CameraOrbitLookIdSetting = "/Amazon/Preferences/Editor/Camera/OrbitLookId";

        m_orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(
            AzFramework::InputChannelId(GetRegistry(CameraOrbitLookIdSetting, AZStd::string("mouse_button_left")).c_str()));

        const auto translateCameraInputChannelIds = BuildTranslateCameraInputChannelIds();
        m_orbitTranslateCamera =
            AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::OrbitTranslation, translateCameraInputChannelIds);
        m_orbitDollyScrollCamera = AZStd::make_shared<AzFramework::OrbitDollyScrollCameraInput>();
    }

    void AnimViewportWidget::SetupCameraController()
    {
        auto controller = AZStd::make_shared<AtomToolsFramework::ModularViewportCameraController>();
        controller->SetCameraViewportContextBuilderCallback(
            [viewportId =
                 GetViewportContext()->GetId()](AZStd::unique_ptr<AtomToolsFramework::ModularCameraViewportContext>& cameraViewportContext)
            {
                cameraViewportContext = AZStd::make_unique<AtomToolsFramework::ModularCameraViewportContextImpl>(viewportId);
            });

        controller->SetCameraPriorityBuilderCallback(
            [](AtomToolsFramework::CameraControllerPriorityFn& cameraControllerPriorityFn)
            {
                cameraControllerPriorityFn = AtomToolsFramework::DefaultCameraControllerPriority;
            });

        controller->SetCameraPropsBuilderCallback(
            [](AzFramework::CameraProps& cameraProps)
            {
                cameraProps.m_rotateSmoothnessFn = []
                {
                    return 5.0f;
                };

                cameraProps.m_translateSmoothnessFn = []
                {
                    return 5.0f;
                };

                cameraProps.m_rotateSmoothingEnabledFn = []
                {
                    return true;
                };

                cameraProps.m_translateSmoothingEnabledFn = []
                {
                    return true;
                };
            });

        controller->SetCameraListBuilderCallback(
            [this](AzFramework::Cameras& cameras)
            {
                cameras.AddCamera(m_orbitRotateCamera);
                cameras.AddCamera(m_orbitTranslateCamera);
                cameras.AddCamera(m_orbitDollyScrollCamera);
            });
        GetControllerList()->Add(controller);
    }
} // namespace EMStudio
