/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Name/Name.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>
#include <Window/ShaderManagementConsoleWindow.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTableView>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindow::ShaderManagementConsoleWindow(QWidget* parent /* = 0 */)
        : AtomToolsFramework::AtomToolsDocumentMainWindow(parent)
    {
        resize(1280, 1024);

        // Among other things, we need the window wrapper to save the main window size, position, and state
        auto mainWindowWrapper =
            new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        mainWindowWrapper->setGuest(this);
        mainWindowWrapper->enableSaveRestoreGeometry("O3DE", "ShaderManagementConsole", "mainWindowGeometry");

        setWindowTitle("Shader Management Console");

        setObjectName("ShaderManagementConsoleWindow");

        m_toolBar = new ShaderManagementConsoleToolBar(this);
        m_toolBar->setObjectName("ToolBar");
        addToolBar(m_toolBar);

        AddDockWidget("Asset Browser", new ShaderManagementConsoleBrowserWidget, Qt::BottomDockWidgetArea, Qt::Vertical);
        AddDockWidget("Python Terminal", new AzToolsFramework::CScriptTermDialog, Qt::BottomDockWidgetArea, Qt::Horizontal);

        SetDockWidgetVisible("Python Terminal", false);

        // Restore geometry and show the window
        mainWindowWrapper->showFromSettings();

        // Disable unused actions
        m_actionNew->setVisible(false);
        m_actionNew->setEnabled(false);
        m_actionSaveAsChild->setVisible(false);
        m_actionSaveAsChild->setEnabled(false);

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    QWidget* ShaderManagementConsoleWindow::CreateDocumentTabView(const AZ::Uuid& documentId)
    {
        AZStd::unordered_set<AZStd::string> optionNames;

        size_t shaderOptionCount = 0;
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderOptionCount, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionCount);

        for (size_t optionIndex = 0; optionIndex < shaderOptionCount; ++optionIndex)
        {
            AZ::RPI::ShaderOptionDescriptor shaderOptionDesc;
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                shaderOptionDesc, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor, optionIndex);

            const char* optionName = shaderOptionDesc.GetName().GetCStr();
            optionNames.insert(optionName);
        }

        size_t shaderVariantCount = 0;
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderVariantCount, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantCount);

        auto model = new QStandardItemModel();
        model->setRowCount(static_cast<int>(shaderVariantCount));
        model->setColumnCount(static_cast<int>(optionNames.size()));

        int nameIndex = 0;
        for (const auto& optionName : optionNames)
        {
            model->setHeaderData(nameIndex++, Qt::Horizontal, optionName.c_str());
        }

        for (int variantIndex = 0; variantIndex < shaderVariantCount; ++variantIndex)
        {
            AZ::RPI::ShaderVariantListSourceData::VariantInfo shaderVariantInfo;
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                shaderVariantInfo, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantInfo, variantIndex);

            model->setHeaderData(variantIndex, Qt::Vertical, QString::number(variantIndex));

            for (const auto& shaderOption : shaderVariantInfo.m_options)
            {
                AZ::Name optionName{ shaderOption.first };
                AZ::Name optionValue{ shaderOption.second };

                auto optionIt = optionNames.find(optionName.GetCStr());
                int optionIndex = static_cast<int>(AZStd::distance(optionNames.begin(), optionIt));

                QStandardItem* item = new QStandardItem(optionValue.GetCStr());
                model->setItem(variantIndex, optionIndex, item);
            }
        }

        // The document tab contains a table view.
        auto contentWidget = new QTableView(centralWidget());
        contentWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        contentWidget->setModel(model);
        return contentWidget;
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleWindow.cpp>
