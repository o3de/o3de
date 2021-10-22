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
        Reinit();

        AnimViewportRequestBus::Handler::BusConnect();
    }

    AnimViewportWidget::~AnimViewportWidget()
    {
        AnimViewportRequestBus::Handler::BusDisconnect();
    }

    void AnimViewportWidget::Reinit(bool resetCamera)
    {
        if (resetCamera)
        {
            ResetCamera();
        }
        m_renderer->Reinit();
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

    void AnimViewportWidget::ResetCamera()
    {
        SetCameraViewMode(CameraViewMode::DEFAULT);
    }

    void AnimViewportWidget::SetCameraViewMode([[maybe_unused]]CameraViewMode mode)
    {
        // Set the camera view mode.
        const AZ::Vector3 targetPosition = m_renderer->GetCharacterCenter();
        AZ::Vector3 cameraPosition;
        switch (mode)
        {
        case CameraViewMode::FRONT:
            cameraPosition.Set(0.0f, CameraDistance, targetPosition.GetZ());
            break;
        case CameraViewMode::BACK:
            cameraPosition.Set(0.0f, -CameraDistance, targetPosition.GetZ());
            break;
        case CameraViewMode::TOP:
            cameraPosition.Set(0.0f, 0.0f, CameraDistance + targetPosition.GetZ());
            break;
        case CameraViewMode::BOTTOM:
            cameraPosition.Set(0.0f, 0.0f, -CameraDistance + targetPosition.GetZ());
            break;
        case CameraViewMode::LEFT:
            cameraPosition.Set(-CameraDistance, 0.0f, targetPosition.GetZ());
            break;
        case CameraViewMode::RIGHT:
            cameraPosition.Set(CameraDistance, 0.0f, targetPosition.GetZ());
            break;
        case CameraViewMode::DEFAULT:
            // The default view mode is looking from the top left of the character.
            cameraPosition.Set(-CameraDistance, CameraDistance, CameraDistance + targetPosition.GetZ());
            break;
        }
        GetViewportContext()->SetCameraTransform(AZ::Transform::CreateLookAt(cameraPosition, targetPosition));
    }
} // namespace EMStudio
