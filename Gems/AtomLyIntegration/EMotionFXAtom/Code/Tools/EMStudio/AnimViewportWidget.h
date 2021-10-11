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

namespace EMStudio
{
    class AnimViewportRenderer;

    class AnimViewportWidget
        : public AtomToolsFramework::RenderViewportWidget
    {
    public:
        AnimViewportWidget(QWidget* parent = nullptr);
        AnimViewportRenderer* GetAnimViewportRenderer() { return m_renderer.get(); }

    private:
        void SetupCameras();
        void SetupCameraController();

        AZStd::unique_ptr<AnimViewportRenderer> m_renderer;
        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_rotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_translateCamera;
        AZStd::shared_ptr<AzFramework::OrbitDollyScrollCameraInput> m_orbitDollyScrollCamera;
    };
}
