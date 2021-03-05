/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RHI/Device.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/WindowContext.h>

#include <Source/Viewport/MaterialViewportWidget.h>
#include <Source/Viewport/MaterialViewportRenderer.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QAbstractEventDispatcher>
#include <QWindow>
#include "Source/Viewport/ui_MaterialViewportWidget.h"
AZ_POP_DISABLE_WARNING

#include <AzCore/PlatformIncl.h>
#include <AzFramework/Viewport/ViewportControllerList.h>

namespace Platform
{
    void ProcessInput(void* message);
}


namespace MaterialEditor
{

    MaterialViewportWidget::MaterialViewportWidget(QWidget* parent)
        : AtomToolsFramework::RenderViewportWidget(AzFramework::InvalidViewportId, parent)
        , m_ui(new Ui::MaterialViewportWidget)
    {
        m_ui->setupUi(this);

        if (auto dispatcher = QAbstractEventDispatcher::instance())
        {
            dispatcher->installNativeEventFilter(this);
        }

        m_renderer = AZStd::make_unique<MaterialViewportRenderer>(GetViewportContext()->GetWindowContext());
        GetControllerList()->Add(m_renderer->GetController());
    }

    // This is a temporary fix to get input working in Qt window, otherwise it wont receive input events
    // This will later be handled on the QApplication subclass level
    bool MaterialViewportWidget::nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* /*result*/)
    {
        Platform::ProcessInput(message);

        return false;
    }
} // namespace MaterialEditor

#include <Source/Viewport/moc_MaterialViewportWidget.cpp>
