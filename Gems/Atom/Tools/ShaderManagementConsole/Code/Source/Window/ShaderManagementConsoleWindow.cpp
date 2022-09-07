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
#include <Window/ShaderManagementConsoleWindow.h>

#include <QFileDialog>
#include <QUrl>
#include <QWindow>
#include <QMessageBox>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindow::ShaderManagementConsoleWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, "ShaderManagementConsoleWindow",  parent)
    {
        m_assetBrowser->SetFilterState("", AZ::RPI::ShaderAsset::Group, true);

        m_documentInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_documentInspector->SetDocumentSettingsPrefix("/O3DE/Atom/ShaderManagementConsole/DocumentInspector");
        AddDockWidget("Inspector", m_documentInspector, Qt::RightDockWidgetArea);
        SetDockWidgetVisible("Inspector", false);

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    void ShaderManagementConsoleWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        Base::OnDocumentOpened(documentId);
        m_documentInspector->SetDocumentId(documentId);
    }

    AZStd::string ShaderManagementConsoleWindow::GetSaveDocumentParams(const AZStd::string& initialPath) const
    {
        // Get shader file path
        AZStd::string shaderFullPath;
        AZ::RPI::ShaderVariantListSourceData shaderVariantList = {};
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderVariantList,
            GetCurrentDocumentId(),
            &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);
        shaderFullPath = AZ::RPI::AssetUtils::ResolvePathReference(initialPath, shaderVariantList.m_shaderFilePath);
        
        QMessageBox msgBox;
        msgBox.setText("Where do you want to save the list?");
        QPushButton* projectBtn = msgBox.addButton(QObject::tr("Save to project"), QMessageBox::YesRole);
        QPushButton* engineBtn = msgBox.addButton(QObject::tr("Save to engine"), QMessageBox::NoRole);
        msgBox.exec();

        AZStd::string result;
        if (msgBox.clickedButton() == projectBtn)
        {
            AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            AZStd::string relativePath, rootFolder;
            bool pathFound;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                pathFound,
                &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
                shaderFullPath,
                relativePath,
                rootFolder);

            if (pathFound)
            {
                AzFramework::StringFunc::Path::ReplaceExtension(relativePath, "shadervariantlist");
                result = AZStd::string::format("%s\\ShaderVariants\\%s", projectPath.c_str(), relativePath.c_str());
                AzFramework::StringFunc::Path::Normalize(result);
            }
            else
            {
                AZ_Error("ShaderManagementConsoleWindow", false, "Can not find a relative path from the shader: '%s'.", shaderFullPath.c_str());
            }
        }
        else if(msgBox.clickedButton() == engineBtn)
        {
            AzFramework::StringFunc::Path::ReplaceExtension(shaderFullPath, "shadervariantlist");
            result = shaderFullPath;
        }

        return result;
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleWindow.cpp>
