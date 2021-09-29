/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>


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
        AZStd::unique_ptr<AnimViewportRenderer> m_renderer;
    };
}
