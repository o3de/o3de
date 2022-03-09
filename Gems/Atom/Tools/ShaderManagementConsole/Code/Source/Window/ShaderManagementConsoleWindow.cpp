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
#include <Window/ShaderManagementConsoleTableView.h>
#include <Window/ShaderManagementConsoleWindow.h>

#include <QFileDialog>
#include <QUrl>
#include <QWindow>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindow::ShaderManagementConsoleWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, parent)
    {
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        m_assetBrowser->SetFilterState("", AZ::RPI::ShaderAsset::Group, true);

        // Disable unused actions
        m_actionNew->setVisible(false);
        m_actionNew->setEnabled(false);
        m_actionSaveAsChild->setVisible(false);
        m_actionSaveAsChild->setEnabled(false);

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    QWidget* ShaderManagementConsoleWindow::CreateDocumentTabView(const AZ::Uuid& documentId)
    {
        return new ShaderManagementConsoleTableView(m_toolId, documentId, centralWidget());
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleWindow.cpp>
