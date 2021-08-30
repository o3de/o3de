/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Document/MaterialDocumentRequestBus.h>
#include <Atom/RHI/Factory.h>
#include <Atom/Window/MaterialEditorWindowSettings.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>
#include <Viewport/MaterialViewportWidget.h>
#include <Window/CreateMaterialDialog/CreateMaterialDialog.h>
#include <Window/HelpDialog/HelpDialog.h>
#include <Window/MaterialBrowserWidget.h>
#include <Window/MaterialEditorWindow.h>
#include <Window/MaterialInspector/MaterialInspector.h>
#include <Window/PerformanceMonitor/PerformanceMonitorWidget.h>
#include <Window/SettingsDialog/SettingsDialog.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QFileDialog>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    MaterialEditorWindow::MaterialEditorWindow(QWidget* parent /* = 0 */)
        : AtomToolsFramework::AtomToolsDocumentMainWindow(parent)
    {
        resize(1280, 1024);

        // Among other things, we need the window wrapper to save the main window size, position, and state
        auto mainWindowWrapper =
            new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        mainWindowWrapper->setGuest(this);
        mainWindowWrapper->enableSaveRestoreGeometry("O3DE", "MaterialEditor", "mainWindowGeometry");

        // set the style sheet for RPE highlighting and other styling
        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral(":/MaterialEditor.qss"));

        QApplication::setWindowIcon(QIcon(":/Icons/materialeditor.svg"));

        AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
        if (!apiName.IsEmpty())
        {
            QString title = QString{ "%1 (%2)" }.arg(QApplication::applicationName()).arg(apiName.GetCStr());
            setWindowTitle(title);
        }
        else
        {
            AZ_Assert(false, "Render API name not found");
            setWindowTitle(QApplication::applicationName());
        }

        setObjectName("MaterialEditorWindow");

        m_toolBar = new MaterialEditorToolBar(this);
        m_toolBar->setObjectName("ToolBar");
        addToolBar(m_toolBar);

        m_materialViewport = new MaterialViewportWidget(centralWidget());
        m_materialViewport->setObjectName("Viewport");
        m_materialViewport->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        centralWidget()->layout()->addWidget(m_materialViewport);

        AddDockWidget("Asset Browser", new MaterialBrowserWidget, Qt::BottomDockWidgetArea, Qt::Vertical);
        AddDockWidget("Inspector", new MaterialInspector, Qt::RightDockWidgetArea, Qt::Horizontal);
        AddDockWidget("Viewport Settings", new ViewportSettingsInspector, Qt::LeftDockWidgetArea, Qt::Horizontal);
        AddDockWidget("Performance Monitor", new PerformanceMonitorWidget, Qt::RightDockWidgetArea, Qt::Horizontal);
        AddDockWidget("Python Terminal", new AzToolsFramework::CScriptTermDialog, Qt::BottomDockWidgetArea, Qt::Horizontal);

        SetDockWidgetVisible("Viewport Settings", false);
        SetDockWidgetVisible("Performance Monitor", false);
        SetDockWidgetVisible("Python Terminal", false);

        // Restore geometry and show the window
        mainWindowWrapper->showFromSettings();

        // Restore additional state for docked windows
        auto windowSettings = AZ::UserSettings::CreateFind<MaterialEditorWindowSettings>(
            AZ::Crc32("MaterialEditorWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        if (!windowSettings->m_mainWindowState.empty())
        {
            QByteArray windowState(windowSettings->m_mainWindowState.data(), static_cast<int>(windowSettings->m_mainWindowState.size()));
            m_advancedDockManager->restoreState(windowState);
        }

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    void MaterialEditorWindow::ResizeViewportRenderTarget(uint32_t width, uint32_t height)
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
            "Material Editor", static_cast<uint32_t>(newDeviceSize.width()) == width && static_cast<uint32_t>(newDeviceSize.height()) == height,
            "Resizing the window did not give the expected frame size. Requested %d x %d but got %d x %d.", width, height,
            newDeviceSize.width(), newDeviceSize.height());
    }

    void MaterialEditorWindow::LockViewportRenderTargetSize(uint32_t width, uint32_t height)
    {
        m_materialViewport->LockRenderTargetSize(width, height);
    }

    void MaterialEditorWindow::UnlockViewportRenderTargetSize()
    {
        m_materialViewport->UnlockRenderTargetSize();
    }

    bool MaterialEditorWindow::GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath)
    {
        CreateMaterialDialog createDialog(this);
        createDialog.adjustSize();

        if (createDialog.exec() == QDialog::Accepted &&
            !createDialog.m_materialFileInfo.absoluteFilePath().isEmpty() &&
            !createDialog.m_materialTypeFileInfo.absoluteFilePath().isEmpty())
        {
            savePath = createDialog.m_materialFileInfo.absoluteFilePath().toUtf8().constData();
            openPath = createDialog.m_materialTypeFileInfo.absoluteFilePath().toUtf8().constData();
            return true;
        }
        return false;
    }

    bool MaterialEditorWindow::GetOpenDocumentParams(AZStd::string& openPath)
    {
        const AZStd::vector<AZ::Data::AssetType> assetTypes = { azrtti_typeid<AZ::RPI::MaterialAsset>() };
        openPath = AtomToolsFramework::GetOpenFileInfo(assetTypes).absoluteFilePath().toUtf8().constData();
        return !openPath.empty();
    }

    void MaterialEditorWindow::OpenSettings()
    {
        SettingsDialog dialog(this);
        dialog.exec();
    }

    void MaterialEditorWindow::OpenHelp()
    {
        HelpDialog dialog(this);
        dialog.exec();
    }

    void MaterialEditorWindow::closeEvent(QCloseEvent* closeEvent)
    {
        // Capture docking state before shutdown
        auto windowSettings = AZ::UserSettings::CreateFind<MaterialEditorWindowSettings>(
            AZ::Crc32("MaterialEditorWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        QByteArray windowState = m_advancedDockManager->saveState();
        windowSettings->m_mainWindowState.assign(windowState.begin(), windowState.end());

        Base::closeEvent(closeEvent);
    }
} // namespace MaterialEditor

#include <Window/moc_MaterialEditorWindow.cpp>
