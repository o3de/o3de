/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "PythonScriptsDialog.h"

// AzCore
#include <AzCore/Module/Module.h>                               // for AZ::ModuleData
#include <AzCore/Module/ModuleManagerBus.h>                     // for AZ::ModuleManagerRequestBus
#include <AzCore/Module/DynamicModuleHandle.h>                  // for AZ::DynamicModuleHandle
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

// AzToolsFramework
#include <AzToolsFramework/API/ViewPaneOptions.h>               // for AzToolsFramework::ViewPaneOptions
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h> // for AzToolsFramework::EditorPythonRunnerRequestBus
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/LineEdit.h>

// Editor
#include "Settings.h"
#include "LyViewPaneNames.h"



AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <Dialogs/ui_PythonScriptsDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

//////////////////////////////////////////////////////////////////////////
namespace
{
    // File name extension for python files
    const QString s_kPythonFileNameSpec("*.py");

    // Tree root element name
    const QString s_kRootElementName("Python Scripts");
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::RegisterViewClass()
{
    if (AzToolsFramework::EditorPythonRunnerRequestBus::HasHandlers())
    {
        AzToolsFramework::ViewPaneOptions options;
        options.canHaveMultipleInstances = true;
        AzToolsFramework::RegisterViewPane<CPythonScriptsDialog>("Python Scripts", LyViewPane::CategoryOther, options);
    }
}

//////////////////////////////////////////////////////////////////////////
CPythonScriptsDialog::CPythonScriptsDialog(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CPythonScriptsDialog)
{
    ui->setupUi(this);

    AzQtComponents::LineEdit::applySearchStyle(ui->searchField);

    AZStd::vector<QString> scriptFolders;
    auto engineScriptPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / "Assets" / "Editor" / "Scripts";
    scriptFolders.push_back(engineScriptPath.c_str());

    AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
    ScanFolderForScripts(QString("%1/Editor/Scripts").arg(projectPath.c_str()), scriptFolders);

    AZStd::vector<AZ::IO::Path> gemSourcePaths;

    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        auto AppendGemPaths = [&gemSourcePaths](AZStd::string_view, AZStd::string_view gemPath)
        {
            gemSourcePaths.emplace_back(gemPath);
        };

        AZ::SettingsRegistryMergeUtils::VisitActiveGems(*settingsRegistry, AppendGemPaths);

        for (const AZ::IO::Path& gemSourcePath : gemSourcePaths)
        {
            ScanFolderForScripts(QString("%1/Editor/Scripts").arg(gemSourcePath.c_str()), scriptFolders);
        }
    }

    ui->treeView->Configure(scriptFolders, s_kPythonFileNameSpec, s_kRootElementName, false, false);
    ui->treeView->expandAll();
    QObject::connect(ui->treeView, &CFolderTreeCtrl::ItemDoubleClicked, this, &CPythonScriptsDialog::OnExecute);
    QObject::connect(ui->executeButton, &QPushButton::clicked, this, &CPythonScriptsDialog::OnExecute);
    QObject::connect(ui->searchField, &QLineEdit::textChanged, this, [&](QString searchText){
        ui->treeView->SetSearchFilter(searchText);
        if(searchText.trimmed().isEmpty()) 
        {
            ui->treeView->expandAll();
        }
    });
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::ScanFolderForScripts(QString path, AZStd::vector<QString>& scriptFolders) const
{
    char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
    if (AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(path.toLocal8Bit().constData(), resolvedPath, AZ_MAX_PATH_LEN))
    {
        if (AZ::IO::SystemFile::Exists(resolvedPath))
        {
            scriptFolders.push_back(path);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CPythonScriptsDialog::~CPythonScriptsDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::OnExecute()
{
    QList<QStandardItem*> selectedItems = ui->treeView->GetSelectedItems();
    QStandardItem* selectedItem = selectedItems.empty() ? nullptr : selectedItems.first();

    if (selectedItem == nullptr)
    {
        return;
    }

    if (ui->treeView->IsFile(selectedItem))
    {
        auto scriptPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / ui->treeView->GetPath(selectedItem).toUtf8().constData();
        using namespace AzToolsFramework;
        EditorPythonRunnerRequestBus::Broadcast(&EditorPythonRunnerRequestBus::Events::ExecuteByFilename, scriptPath.Native());
    }
}

#include <Dialogs/moc_PythonScriptsDialog.cpp>
