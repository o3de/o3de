/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MatrixUtils.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/View.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>

#include <EMStudio/AnimViewportWidget.h>
#include <EMStudio/AnimViewportRenderer.h>
#include <EMStudio/AnimViewportSettings.h>
#include <EMStudio/AtomRenderPlugin.h>

namespace EMStudio
{
    AnimViewportWidget::AnimViewportWidget(AtomRenderPlugin* parentPlugin)
        : AtomToolsFramework::RenderViewportWidget(parentPlugin->GetInnerWidget())
        , m_plugin(parentPlugin)
    {
        setObjectName(QString::fromUtf8("AtomViewportWidget"));
        QSizePolicy qSize(QSizePolicy::Preferred, QSizePolicy::Preferred);
        qSize.setHorizontalStretch(0);
        qSize.setVerticalStretch(0);
        qSize.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(qSize);
        setAutoFillBackground(false);
        setStyleSheet(QString::fromUtf8(""));

        m_renderer = AZStd::make_unique<AnimViewportRenderer>(GetViewportContext(), m_plugin->GetRenderOptions());
        SetScene(m_renderer->GetFrameworkScene(), false);

        LoadRenderFlags();
        SetupCameras();
        SetupCameraController();
        Reinit();

        AnimViewportRequestBus::Handler::BusConnect();
        ViewportPluginRequestBus::Handler::BusConnect();
    }

    AnimViewportWidget::~AnimViewportWidget()
    {
        SaveRenderFlags();
        ViewportPluginRequestBus::Handler::BusDisconnect();
        AnimViewportRequestBus::Handler::BusDisconnect();
    }

    void AnimViewportWidget::Reinit(bool resetCamera)
    {
        m_renderer->Reinit();
        m_renderer->UpdateActorRenderFlag(m_renderFlags);

        if (resetCamera)
        {
            ResetCamera();
        }
    }

    EMotionFX::ActorRenderFlagBitset AnimViewportWidget::GetRenderFlags() const
    {
        return m_renderFlags;
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

    void AnimViewportWidget::SetCameraViewMode(CameraViewMode mode)
    {
        // Set the camera view mode.
        const AZ::Vector3 targetPosition = m_renderer->GetCharacterCenter();
        AZ::Vector3 cameraPosition;
        switch (mode)
        {
        case CameraViewMode::FRONT:
            cameraPosition.Set(targetPosition.GetX(), targetPosition.GetY() + CameraDistance, targetPosition.GetZ());
            break;
        case CameraViewMode::BACK:
            cameraPosition.Set(targetPosition.GetX(), targetPosition.GetY() - CameraDistance, targetPosition.GetZ());
            break;
        case CameraViewMode::TOP:
            cameraPosition.Set(targetPosition.GetX(), targetPosition.GetY(), CameraDistance + targetPosition.GetZ());
            break;
        case CameraViewMode::BOTTOM:
            cameraPosition.Set(targetPosition.GetX(), targetPosition.GetY(), -CameraDistance + targetPosition.GetZ());
            break;
        case CameraViewMode::LEFT:
            cameraPosition.Set(targetPosition.GetX() - CameraDistance, targetPosition.GetY(), targetPosition.GetZ());
            break;
        case CameraViewMode::RIGHT:
            cameraPosition.Set(targetPosition.GetX() + CameraDistance, targetPosition.GetY(), targetPosition.GetZ());
            break;
        case CameraViewMode::DEFAULT:
            // The default view mode is looking from the top left of the character.
            cameraPosition.Set(
                targetPosition.GetX() - CameraDistance, targetPosition.GetY() + CameraDistance, targetPosition.GetZ() + CameraDistance);
            break;
        }
        GetViewportContext()->SetCameraTransform(AZ::Transform::CreateLookAt(cameraPosition, targetPosition));
    }

    void AnimViewportWidget::SetFollowCharacter(bool follow)
    {
        m_followCharacter = follow;
    }

    void AnimViewportWidget::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        RenderViewportWidget::OnTick(deltaTime, time);
        CalculateCameraProjection();
        RenderCustomPluginData();
        FollowCharacter();
    }

    void AnimViewportWidget::CalculateCameraProjection()
    {
        auto viewportContext = GetViewportContext();
        auto windowSize = viewportContext->GetViewportSize();
        // Prevent devided by zero
        const float height = AZStd::max<float>(aznumeric_cast<float>(windowSize.m_height), 1.0f);
        const float aspectRatio = aznumeric_cast<float>(windowSize.m_width) / height;

        const RenderOptions* renderOptions = m_plugin->GetRenderOptions();
        AZ::Matrix4x4 viewToClipMatrix;
        AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, AZ::DegToRad(renderOptions->GetFOV()), aspectRatio,
            renderOptions->GetNearClipPlaneDistance(), renderOptions->GetFarClipPlaneDistance(), true);

        viewportContext->GetDefaultView()->SetViewToClipMatrix(viewToClipMatrix);
    }

    void AnimViewportWidget::RenderCustomPluginData()
    {
        const size_t numPlugins = GetPluginManager()->GetNumActivePlugins();
        for (size_t i = 0; i < numPlugins; ++i)
        {
            EMStudioPlugin* plugin = GetPluginManager()->GetActivePlugin(i);
            plugin->Render(m_renderFlags);
        }
    }

    void AnimViewportWidget::FollowCharacter()
    {
        if (m_followCharacter)
        {
            // When follow charater move is enabled, we are adding the delta of the character movement to the camera per frame.
            AZ::Vector3 camPos = GetViewportContext()->GetCameraTransform().GetTranslation();
            camPos += m_renderer->GetCharacterCenter() - m_prevCharacterPos;
            AZ::Transform newCamTransform = GetViewportContext()->GetCameraTransform();
            newCamTransform.SetTranslation(camPos);
            GetViewportContext()->SetCameraTransform(newCamTransform);
        }
        m_prevCharacterPos = m_renderer->GetCharacterCenter();
    }

    void AnimViewportWidget::ToggleRenderFlag(EMotionFX::ActorRenderFlag flag)
    {
        m_renderFlags[flag] = !m_renderFlags[flag];
        m_renderer->UpdateActorRenderFlag(m_renderFlags);
    }

    void AnimViewportWidget::LoadRenderFlags()
    {
        AZStd::string renderFlagsFilename(EMStudioManager::GetInstance()->GetAppDataFolder());
        renderFlagsFilename += "AnimViewportRenderFlags.cfg";
        QSettings settings(renderFlagsFilename.c_str(), QSettings::IniFormat, this);

        for (uint32 i = 0; i < EMotionFX::ActorRenderFlag::NUM_RENDERFLAGS; ++i)
        {
            QString name = QString(i);
            const bool isEnabled = settings.value(name).toBool();
            m_renderFlags[i] = isEnabled;
        }
        m_renderer->UpdateActorRenderFlag(m_renderFlags);
    }

    void AnimViewportWidget::SaveRenderFlags()
    {
        AZStd::string renderFlagsFilename(EMStudioManager::GetInstance()->GetAppDataFolder());
        renderFlagsFilename += "AnimViewportRenderFlags.cfg";
        QSettings settings(renderFlagsFilename.c_str(), QSettings::IniFormat, this);

        for (uint32 i = 0; i < EMotionFX::ActorRenderFlag::NUM_RENDERFLAGS; ++i)
        {
            QString name = QString(i);
            settings.setValue(name, (bool)m_renderFlags[i]);
        }
    }

    AZ::s32 AnimViewportWidget::GetViewportId() const
    {
        return GetViewportContext()->GetId();
    }
} // namespace EMStudio
