/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Device.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/WindowContext.h>

#include <Viewport/MaterialViewportRenderer.h>
#include <Viewport/MaterialViewportWidget.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QWindow>
#include "Viewport/ui_MaterialViewportWidget.h"
AZ_POP_DISABLE_WARNING

#include <AzFramework/Viewport/ViewportControllerList.h>

namespace MaterialEditor
{

    MaterialViewportWidget::MaterialViewportWidget(QWidget* parent)
        : AtomToolsFramework::RenderViewportWidget(parent)
        , m_ui(new Ui::MaterialViewportWidget)
    {
        m_ui->setupUi(this);

        // The viewport context created by AtomToolsFramework::RenderViewportWidget has no name.
        // Systems like frame capturing and post FX expect there to be a context with DefaultViewportContextName
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        const AZ::Name defaultContextName = viewportContextManager->GetDefaultViewportContextName();
        viewportContextManager->RenameViewportContext(GetViewportContext(), defaultContextName);

        m_renderer = AZStd::make_unique<MaterialViewportRenderer>(GetViewportContext()->GetWindowContext());
        GetControllerList()->Add(m_renderer->GetController());
    }
} // namespace MaterialEditor
