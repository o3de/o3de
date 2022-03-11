/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AtomToolsFramework/Util/Util.h>
#include <Viewport/MaterialCanvasViewportWidget.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

#include <QApplication>
#include <QMessageBox>
#include <QWindow>

namespace MaterialCanvas
{
    MaterialCanvasMainWindow::MaterialCanvasMainWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, parent)
    {
        m_toolBar = new MaterialCanvasToolBar(m_toolId, this);
        m_toolBar->setObjectName("ToolBar");
        addToolBar(m_toolBar);

        m_materialViewport = new MaterialCanvasViewportWidget(m_toolId, centralWidget());
        m_materialViewport->setObjectName("Viewport");
        m_materialViewport->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        centralWidget()->layout()->addWidget(m_materialViewport);

        m_assetBrowser->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);

        m_materialInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_materialInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/MaterialInspector");

        AddDockWidget("Inspector", m_materialInspector, Qt::RightDockWidgetArea, Qt::Vertical);
        AddDockWidget("Viewport Settings", new ViewportSettingsInspector(m_toolId, this), Qt::LeftDockWidgetArea, Qt::Vertical);
        SetDockWidgetVisible("Viewport Settings", false);

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    void MaterialCanvasMainWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        Base::OnDocumentOpened(documentId);
        m_materialInspector->SetDocumentId(documentId);
    }

    void MaterialCanvasMainWindow::ResizeViewportRenderTarget(uint32_t width, uint32_t height)
    {
        QSize requestedViewportSize = QSize(width, height) / devicePixelRatioF();
        QSize currentViewportSize = m_materialViewport->size();
        QSize offset = requestedViewportSize - currentViewportSize;
        QSize requestedWindowSize = size() + offset;
        resize(requestedWindowSize);

        AZ_Assert(
            m_materialViewport->size() == requestedViewportSize,
            "Resizing the window did not give the expected viewport size. Requested %d x %d but got %d x %d.",
            requestedViewportSize.width(), requestedViewportSize.height(), m_materialViewport->size().width(),
            m_materialViewport->size().height());

        [[maybe_unused]] QSize newDeviceSize = m_materialViewport->size();
        AZ_Warning(
            "Material Canvas", static_cast<uint32_t>(newDeviceSize.width()) == width && static_cast<uint32_t>(newDeviceSize.height()) == height,
            "Resizing the window did not give the expected frame size. Requested %d x %d but got %d x %d.", width, height,
            newDeviceSize.width(), newDeviceSize.height());
    }

    void MaterialCanvasMainWindow::LockViewportRenderTargetSize(uint32_t width, uint32_t height)
    {
        m_materialViewport->LockRenderTargetSize(width, height);
    }

    void MaterialCanvasMainWindow::UnlockViewportRenderTargetSize()
    {
        m_materialViewport->UnlockRenderTargetSize();
    }

    void MaterialCanvasMainWindow::OpenSettings()
    {
    }

    void MaterialCanvasMainWindow::OpenHelp()
    {
        QMessageBox::information(
            this, windowTitle(),
            R"(<html><head/><body>
            <p><h3><u>Material Canvas Controls</u></h3></p>
            <p><b>LMB</b> - pan camera</p>
            <p><b>RMB</b> or <b>Alt+LMB</b> - orbit camera around target</p>
            <p><b>MMB</b> or <b>Alt+MMB</b> - move camera on its xy plane</p>
            <p><b>Alt+RMB</b> or <b>LMB+RMB</b> - dolly camera on its z axis</p>
            <p><b>Ctrl+LMB</b> - rotate model</p>
            <p><b>Shift+LMB</b> - rotate environment</p>
            </body></html>)");
    }
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasMainWindow.cpp>
