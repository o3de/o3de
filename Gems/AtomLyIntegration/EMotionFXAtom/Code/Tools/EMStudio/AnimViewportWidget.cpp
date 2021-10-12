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
#include <EMStudio/AnimViewportSettings.h>

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

    void AnimViewportWidget::SetupCameras()
    {
        m_rotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(EMStudio::ViewportUtil::BuildRotateCameraInputId());

        const auto translateCameraInputChannelIds = EMStudio::ViewportUtil::BuildTranslateCameraInputChannelIds();
        m_translateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslatePivotLook);
        m_translateCamera.get()->m_translateSpeedFn = []
        {
            return 3.0f;
        };

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
                    return EMStudio::ViewportUtil::CameraRotateSmoothness();
                };

                cameraProps.m_translateSmoothnessFn = []
                {
                    return EMStudio::ViewportUtil::CameraTranslateSmoothness();
                };

                cameraProps.m_rotateSmoothingEnabledFn = []
                {
                    return EMStudio::ViewportUtil::CameraRotateSmoothingEnabled();
                };

                cameraProps.m_translateSmoothingEnabledFn = []
                {
                    return EMStudio::ViewportUtil::CameraTranslateSmoothingEnabled();
                };
            });

        controller->SetCameraListBuilderCallback(
            [this](AzFramework::Cameras& cameras)
            {
                cameras.AddCamera(m_rotateCamera);
                cameras.AddCamera(m_translateCamera);
                cameras.AddCamera(m_orbitDollyScrollCamera);
            });
        GetControllerList()->Add(controller);
    }
} // namespace EMStudio
