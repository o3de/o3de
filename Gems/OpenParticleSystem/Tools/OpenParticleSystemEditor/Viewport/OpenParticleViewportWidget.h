/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Windowing/WindowBus.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <OpenParticleViewportWidgetRequestsBus.h>
#endif
 // disable warnings spawned by QT
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QWidget>
#include <QShowEvent>
#include <QHideEvent>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace RPI
    {
        class WindowContext;
    }
}

namespace OpenParticleSystemEditor
{
    class OpenParticleViewportRenderer;

    class OpenParticleViewportWidget
        : public AtomToolsFramework::RenderViewportWidget
        , public OpenParticleViewportWidgetRequestsBus::Handler
    {
    public:
        explicit OpenParticleViewportWidget(QWidget* parent = nullptr);
        ~OpenParticleViewportWidget() override;

        AZStd::unique_ptr<OpenParticleViewportRenderer> m_renderer;

        virtual AzFramework::CameraState CameraState() override;
        virtual AzFramework::ViewportId ViewportId() override;

    protected:
        virtual void showEvent(QShowEvent* event) override;
        virtual void hideEvent(QHideEvent* event) override;
        virtual void keyReleaseEvent(QKeyEvent* event) override;
    };
} // namespace OpenParticleSystemEditor
