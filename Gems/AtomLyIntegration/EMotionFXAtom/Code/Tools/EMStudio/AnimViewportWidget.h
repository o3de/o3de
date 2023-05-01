/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QSettings>
#include <QMouseEvent>

#include <Atom/RPI.Public/SceneBus.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>
#include <EMStudio/AnimViewportRequestBus.h>
#include <Integration/Rendering/RenderFlag.h>


namespace EMStudio
{
    class AtomRenderPlugin;
    class AnimViewportRenderer;

    class AnimViewportWidget
        : public AtomToolsFramework::RenderViewportWidget
        , private AnimViewportRequestBus::Handler
        , private ViewportPluginRequestBus::Handler
        , private AZ::RPI::SceneNotificationBus::Handler
    {
    public:
        AnimViewportWidget(AtomRenderPlugin* parentPlugin);
        ~AnimViewportWidget() override;
        AnimViewportRenderer* GetAnimViewportRenderer() { return m_renderer.get(); }

        void Reinit(bool resetCamera = true);

    private:
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void CalculateCameraProjection();
        void RenderCustomPluginData();
        void FollowCharacter();

        void SetupCameras();
        void SetupCameraController();

        void OnContextMenuEvent(QMouseEvent* event);

        // AnimViewportRequestBus::Handler overrides
        void UpdateCameraViewMode(RenderOptions::CameraViewMode mode) override;
        void UpdateCameraFollowUp(bool follow) override;
        void UpdateRenderFlags(EMotionFX::ActorRenderFlags renderFlags) override;

        // ViewportPluginRequestBus::Handler overrides
        AZ::s32 GetViewportId() const;

        // SceneNotificationBus overrides ...
        void OnBeginPrepareRender() override;

        // MouseEvent
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        void resizeEvent(QResizeEvent* event) override;

        static constexpr float CameraDistance = 2.0f;

        AtomRenderPlugin* m_plugin;
        AZStd::unique_ptr<AnimViewportRenderer> m_renderer;

        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_lookRotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_lookTranslateCamera;
        AZStd::shared_ptr<AzFramework::LookScrollTranslationCameraInput> m_lookScrollTranslationCamera;
        AZStd::shared_ptr<AzFramework::PanCameraInput> m_lookPanCamera;

        AZStd::shared_ptr<AzFramework::OrbitCameraInput> m_orbitCamera;
        AZStd::shared_ptr<AzFramework::OrbitScrollDollyCameraInput> m_orbitScrollDollyCamera;
        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_orbitRotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_orbitTranslateCamera;
        AZStd::shared_ptr<AzFramework::OrbitMotionDollyCameraInput> m_orbitMotionDollyCamera;
        AZStd::shared_ptr<AzFramework::PanCameraInput> m_orbitPanCamera;

        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_followRotateCamera;
        AZStd::shared_ptr<AzFramework::OrbitScrollDollyCameraInput> m_followScrollDollyCamera;
        AZStd::shared_ptr<AzFramework::OrbitMotionDollyCameraInput> m_followScrollMotionCamera;

        AZ::Vector3 m_defaultOrbitPoint = AZ::Vector3::CreateZero();

        // Properties related to the mouse event.
        // Used to prevent right click option showing up when mouse moved between press and release.
        QPoint m_prevMousePoint;
        int m_pixelsSinceClick = 0;
        const int MinMouseMovePixes = 5;

        AzToolsFramework::ViewportUi::ViewportUiManager m_viewportUiManager;
        QWidget m_renderOverlay;

        AzFramework::DebugDisplayRequests* m_debugDisplay = nullptr;
    };
}
