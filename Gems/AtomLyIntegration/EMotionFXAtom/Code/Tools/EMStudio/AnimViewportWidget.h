/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <EMStudio/AnimViewportRequestBus.h>

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

    private:
        void SetupCameras();
        void SetupCameraController();

        // AnimViewportRequestBus::Handler overrides
        void ResetCamera();
        void SetCameraViewMode(CameraViewMode mode);

        static constexpr float CameraDistance = 2.0f;

        AZStd::unique_ptr<AnimViewportRenderer> m_renderer;
        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_rotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_translateCamera;
        AZStd::shared_ptr<AzFramework::OrbitDollyScrollCameraInput> m_orbitDollyScrollCamera;
    };
}
