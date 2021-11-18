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

    QStringList scriptFolders;
    auto engineScriptPath = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / "Assets" / "Editor" / "Scripts";
    scriptFolders.push_back(engineScriptPath.c_str());

    AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
    ScanFolderForScripts(QString("%1/Editor/Scripts").arg(projectPath.c_str()), scriptFolders);

    struct GetGemSourcePathsVisitor
        : AZ::SettingsRegistryInterface::Visitor
    {
        GetGemSourcePathsVisitor(AZ::SettingsRegistryInterface& settingsRegistry)
            : m_settingsRegistry(settingsRegistry)
        {}

        using AZ::SettingsRegistryInterface::Visitor::Visit;
        void Visit(AZStd::string_view path, AZStd::string_view, AZ::SettingsRegistryInterface::Type,
            AZStd::string_view value) override
        {
            AZStd::string_view jsonSourcePathPointer{ path };
            // Remove the array index from the path and check if the JSON path ends with "/SourcePaths"
            AZ::StringFunc::TokenizeLast(jsonSourcePathPointer, "/");
            if (jsonSourcePathPointer.ends_with("/SourcePaths"))
            {
                AZ::IO::Path newSourcePath = jsonSourcePathPointer;
                // Resolve any file aliases first - Do not use ResolvePath() as that assumes
                // any relative path is underneath the @products@ alias
                if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
                {
                    AZ::IO::FixedMaxPath replacedAliasPath;
                    if (fileIoBase->ReplaceAlias(replacedAliasPath, value))
                    {
                        newSourcePath = AZ::IO::PathView(replacedAliasPath);
                    }
                }

                // The current assumption is that the gem source path is the relative to the engine root
                AZ::IO::Path engineRootPath;
                m_settingsRegistry.Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
                newSourcePath = (engineRootPath / newSourcePath).LexicallyNormal();

                if (auto gemSourcePathIter = AZStd::find(m_gemSourcePaths.begin(), m_gemSourcePaths.end(), newSourcePath);
                    gemSourcePathIter == m_gemSourcePaths.end())
                {
                    m_gemSourcePaths.emplace_back(AZStd::move(newSourcePath));
                }
            }
        }

        AZStd::vector<AZ::IO::Path> m_gemSourcePaths;
    private:
        AZ::SettingsRegistryInterface& m_settingsRegistry;
    };

    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        GetGemSourcePathsVisitor visitor{ *settingsRegistry };
        constexpr auto gemListKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::OrganizationRootKey)
            + "/Gems";
        settingsRegistry->Visit(visitor, gemListKey);
        for (const AZ::IO::Path& gemSourcePath : visitor.m_gemSourcePaths)
        {
            ScanFolderForScripts(QString("%1/Editor/Scripts").arg(gemSourcePath.c_str()), scriptFolders);
        }
    }

    ui->treeView->init(scriptFolders, s_kPythonFileNameSpec, s_kRootElementName, false, false);
    QObject::connect(ui->treeView, &CFolderTreeCtrl::ItemDoubleClicked, this, &CPythonScriptsDialog::OnExecute);
    QObject::connect(ui->executeButton, &QPushButton::clicked, this, &CPythonScriptsDialog::OnExecute);
    QObject::connect(ui->searchField, &QLineEdit::textChanged, ui->treeView, &CFolderTreeCtrl::SetSearchFilter);
}

//////////////////////////////////////////////////////////////////////////
void CPythonScriptsDialog::ScanFolderForScripts(QString path, QStringList& scriptFolders) const
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
