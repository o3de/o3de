/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <Document/MaterialCanvasDocumentRequestBus.h>
#include <Viewport/MaterialCanvasViewportWidget.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/MaterialCanvasMainWindowSettings.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QUrl>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace MaterialCanvas
{
    MaterialCanvasMainWindow::MaterialCanvasMainWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, parent)
    {
        // Among other things, we need the window wrapper to save the main window size, position, and state
        auto mainWindowWrapper =
            new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        mainWindowWrapper->setGuest(this);
        mainWindowWrapper->enableSaveRestoreGeometry("O3DE", "MaterialCanvas", "mainWindowGeometry");

        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        setObjectName("MaterialCanvasMainWindow");

        m_toolBar = new MaterialCanvasToolBar(this);
        m_toolBar->setObjectName("ToolBar");
        addToolBar(m_toolBar);

        m_materialViewport = new MaterialCanvasViewportWidget(m_toolId, centralWidget());
        m_materialViewport->setObjectName("Viewport");
        m_materialViewport->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        centralWidget()->layout()->addWidget(m_materialViewport);

        m_assetBrowser->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);
        m_assetBrowser->SetOpenHandler([this](const AZStd::string& absolutePath) {
            if (AzFramework::StringFunc::Path::IsExtension(absolutePath.c_str(), AZ::RPI::MaterialSourceData::Extension))
            {
                AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                    m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, absolutePath);
                return;
            }

            if (AzFramework::StringFunc::Path::IsExtension(absolutePath.c_str(), AZ::RPI::MaterialTypeSourceData::Extension))
            {
                return;
            }

            QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath.c_str()));
        });

        m_materialInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_materialInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/MaterialInspector");
        m_materialInspector->SetIndicatorFunction(
            [](const AzToolsFramework::InstanceDataNode* /*node*/)
            {
                return ":/Icons/blank.png";
            });

        AddDockWidget("Inspector", m_materialInspector, Qt::RightDockWidgetArea, Qt::Vertical);

        // Restore geometry and show the window
        mainWindowWrapper->showFromSettings();

        // Restore additional state for docked windows
        auto windowSettings = AZ::UserSettings::CreateFind<MaterialCanvasMainWindowSettings>(
            AZ::Crc32("MaterialCanvasMainWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        if (!windowSettings->m_mainWindowState.empty())
        {
            QByteArray windowState(windowSettings->m_mainWindowState.data(), static_cast<int>(windowSettings->m_mainWindowState.size()));
            m_advancedDockManager->restoreState(windowState);
        }

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

    bool MaterialCanvasMainWindow::GetCreateDocumentParams(AZStd::string& /*openPath*/, AZStd::string& /*savePath*/)
    {
        return false;
    }

    bool MaterialCanvasMainWindow::GetOpenDocumentParams(AZStd::string& openPath)
    {
        const AZStd::vector<AZ::Data::AssetType> assetTypes = { azrtti_typeid<AZ::RPI::MaterialAsset>() };
        openPath = AtomToolsFramework::GetOpenFileInfo(assetTypes).absoluteFilePath().toUtf8().constData();
        return !openPath.empty();
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

    void MaterialCanvasMainWindow::OpenAbout()
    {
        QMessageBox::about(this, windowTitle(), QApplication::applicationName());
    }

    void MaterialCanvasMainWindow::closeEvent(QCloseEvent* closeEvent)
    {
        // Capture docking state before shutdown
        auto windowSettings = AZ::UserSettings::CreateFind<MaterialCanvasMainWindowSettings>(
            AZ::Crc32("MaterialCanvasMainWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        QByteArray windowState = m_advancedDockManager->saveState();
        windowSettings->m_mainWindowState.assign(windowState.begin(), windowState.end());

        Base::closeEvent(closeEvent);
    }
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasMainWindow.cpp>
