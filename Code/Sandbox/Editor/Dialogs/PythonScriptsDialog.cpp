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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "PythonScriptsDialog.h"

// AzCore
#include <AzCore/Module/Module.h>                               // for AZ::ModuleData
#include <AzCore/Module/ModuleManagerBus.h>                     // for AZ::ModuleManagerRequestBus
#include <AzCore/Module/DynamicModuleHandle.h>                  // for AZ::DynamicModuleHandle

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
    const QString s_kPythonFileNameSpec = "*.py";

    // Tree root element name
    const QString s_kRootElementName = "Python Scripts";
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

    const auto editorEnvStr = gSettings.strEditorEnv.toLocal8Bit();
    AZStd::string editorScriptsPath = AZStd::string::format("@engroot@/%s", editorEnvStr.constData());
    XmlNodeRef envNode = XmlHelpers::LoadXmlFromFile(editorScriptsPath.c_str());
    if (envNode)
    {
        QString scriptPath;
        int childrenCount = envNode->getChildCount();
        for (int idx = 0; idx < childrenCount; ++idx)
        {
            XmlNodeRef child = envNode->getChild(idx);
            if (child->haveAttr("scriptPath"))
            {
                scriptPath = child->getAttr("scriptPath");
                scriptFolders.push_back(scriptPath);
            }
        }
    }

    ScanFolderForScripts(QString("@devroot@/%1/Editor/Scripts").arg(GetIEditor()->GetProjectName()), scriptFolders);

    auto moduleCallback = [this, &scriptFolders](const AZ::ModuleData& moduleData) -> bool
    {
        if (moduleData.GetDynamicModuleHandle())
        {
            const AZ::OSString& modulePath = moduleData.GetDynamicModuleHandle()->GetFilename();
            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(modulePath.c_str(), fileName);

            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(fileName.c_str(), tokens, '.');
            if (tokens.size() > 2 && tokens[0] == "Gem")
            {
                ScanFolderForScripts(QString("@engroot@/Gems/%1/Editor/Scripts").arg(tokens[1].c_str()), scriptFolders);
            }
        }
        return true;
    };
    AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::EnumerateModules, moduleCallback);

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

    if (selectedItem == NULL)
    {
        return;
    }

    if (ui->treeView->IsFile(selectedItem))
    {
        QString workingDirectory = QDir::currentPath();
        const QString scriptPath = QStringLiteral("%1/%2").arg(workingDirectory).arg(ui->treeView->GetPath(selectedItem));
        using namespace AzToolsFramework;
        EditorPythonRunnerRequestBus::Broadcast(&EditorPythonRunnerRequestBus::Events::ExecuteByFilename, scriptPath.toUtf8().constData());
    }
}

#include <Dialogs/moc_PythonScriptsDialog.cpp>
