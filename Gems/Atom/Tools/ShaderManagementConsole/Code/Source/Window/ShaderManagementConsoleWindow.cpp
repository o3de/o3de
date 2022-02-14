/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/Utils/Utils.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <Window/ShaderManagementConsoleTableView.h>
#include <Window/ShaderManagementConsoleWindow.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QUrl>
#include <QWindow>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindow::ShaderManagementConsoleWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, parent)
    {
        // Among other things, we need the window wrapper to save the main window size, position, and state
        auto mainWindowWrapper =
            new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        mainWindowWrapper->setGuest(this);
        mainWindowWrapper->enableSaveRestoreGeometry("O3DE", "ShaderManagementConsole", "mainWindowGeometry");

        setWindowTitle("Shader Management Console");

        setObjectName("ShaderManagementConsoleWindow");

        m_assetBrowser->SetFilterState("", AZ::RPI::ShaderAsset::Group, true);
        m_assetBrowser->SetOpenHandler([this](const AZStd::string& absolutePath)
            {
                if (AzFramework::StringFunc::Path::IsExtension(absolutePath.c_str(), AZ::RPI::ShaderSourceData::Extension) ||
                    AzFramework::StringFunc::Path::IsExtension(absolutePath.c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
                {
                    AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                        m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, absolutePath);
                    return;
                }

                QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath.c_str()));
            });

        // Restore geometry and show the window
        mainWindowWrapper->showFromSettings();

        // Disable unused actions
        m_actionNew->setVisible(false);
        m_actionNew->setEnabled(false);
        m_actionSaveAsChild->setVisible(false);
        m_actionSaveAsChild->setEnabled(false);

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    bool ShaderManagementConsoleWindow::GetOpenDocumentParams(AZStd::string& openPath)
    {
        openPath = QFileDialog::getOpenFileName(
            this, tr("Open Document"), AZ::Utils::GetProjectPath().c_str(),
            tr("Shader Files (*.%1);;Shader Variant List Files (*.%2)")
                .arg(AZ::RPI::ShaderSourceData::Extension)
                .arg(AZ::RPI::ShaderVariantListSourceData::Extension))
            .toUtf8()
            .constData();
        return !openPath.empty();
    }

    QWidget* ShaderManagementConsoleWindow::CreateDocumentTabView(const AZ::Uuid& documentId)
    {
        return new ShaderManagementConsoleTableView(m_toolId, documentId, centralWidget());
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleWindow.cpp>
