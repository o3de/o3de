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

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>

namespace Ui
{
    class MaterialViewportWidget;
}

namespace AZ
{
    namespace RPI
    {
        class WindowContext;
    }
}

namespace MaterialEditor
{
    class MaterialViewportRenderer;

    class MaterialViewportWidget
        : public AtomToolsFramework::RenderViewportWidget
    {
    public:
        MaterialViewportWidget(QWidget* parent = nullptr);

        QScopedPointer<Ui::MaterialViewportWidget> m_ui;
        AZStd::unique_ptr<MaterialViewportRenderer> m_renderer;
    };
} // namespace MaterialEditor
