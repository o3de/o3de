/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QSettings>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AzFramework/Viewport/CameraInput.h>

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

        // AnimViewportRequestBus::Handler overrides
        void UpdateCameraViewMode(RenderOptions::CameraViewMode mode);
        void UpdateCameraFollowUp(bool follow);
        void UpdateRenderFlags(EMotionFX::ActorRenderFlagBitset renderFlags);

        // ViewportPluginRequestBus::Handler overrides
        AZ::s32 GetViewportId() const;

        static constexpr float CameraDistance = 2.0f;

        AtomRenderPlugin* m_plugin;
        AZStd::unique_ptr<AnimViewportRenderer> m_renderer;
        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_rotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_translateCamera;
        AZStd::shared_ptr<AzFramework::OrbitDollyScrollCameraInput> m_orbitDollyScrollCamera;
    };
}
