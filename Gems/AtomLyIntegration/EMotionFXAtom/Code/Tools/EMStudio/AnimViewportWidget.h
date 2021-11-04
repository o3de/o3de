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
#include <EMStudio/AnimViewportRequestBus.h>
#include <Integration/Rendering/RenderFlag.h>

namespace EMStudio
{
    class AnimViewportRenderer;

    class AnimViewportWidget
        : public AtomToolsFramework::RenderViewportWidget
        , private AnimViewportRequestBus::Handler
    {
    public:
        AnimViewportWidget(QWidget* parent = nullptr);
        ~AnimViewportWidget() override;
        AnimViewportRenderer* GetAnimViewportRenderer() { return m_renderer.get(); }

        void Reinit(bool resetCamera = true);
        EMotionFX::ActorRenderFlagBitset GetRenderFlags() const;

    private:
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void CalculateCameraProjection();
        void SetupCameras();
        void SetupCameraController();

        void LoadRenderFlags();
        void SaveRenderFlags();

        // AnimViewportRequestBus::Handler overrides
        void ResetCamera();
        void SetCameraViewMode(CameraViewMode mode);
        void ToggleRenderFlag(EMotionFX::ActorRenderFlag flag);

        static constexpr float CameraDistance = 2.0f;
        static constexpr float DepthNear = 0.01f;
        static constexpr float DepthFar = 100.0f;

        AZStd::unique_ptr<AnimViewportRenderer> m_renderer;
        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_rotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_translateCamera;
        AZStd::shared_ptr<AzFramework::OrbitDollyScrollCameraInput> m_orbitDollyScrollCamera;
        EMotionFX::ActorRenderFlagBitset m_renderFlags;
    };
}
