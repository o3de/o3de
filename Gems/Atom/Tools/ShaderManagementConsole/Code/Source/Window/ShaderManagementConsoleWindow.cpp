/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Utils/Utils.h>
#include <Window/ShaderManagementConsoleWindow.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>

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

    void ShaderManagementConsoleWindow::UpdateMenus(QMenuBar* menuBar)
    {
        Base::UpdateMenus(menuBar);
        m_actionSaveAsChild->setVisible(false);
    }

    AZStd::string ShaderManagementConsoleWindow::GetSaveDocumentParams(const AZStd::string& initialPath) const
    {
        // Get shader file path
        AZ::IO::Path shaderFullPath;
        AZ::RPI::ShaderVariantListSourceData shaderVariantList = {};
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderVariantList,
            GetCurrentDocumentId(),
            &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);
        shaderFullPath = AZ::RPI::AssetUtils::ResolvePathReference(initialPath, shaderVariantList.m_shaderFilePath);
        
        QMessageBox msgBox;
        msgBox.setText("Where do you want to save the list?");
        QPushButton* projectBtn = msgBox.addButton(QObject::tr("Save to project"), QMessageBox::ActionRole);
        QPushButton* engineBtn = msgBox.addButton(QObject::tr("Save to engine"), QMessageBox::ActionRole);
        msgBox.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
        msgBox.exec();

        AZ::IO::Path result;
        if (msgBox.clickedButton() == projectBtn)
        {
            AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            AZ::IO::Path relativePath, rootFolder;
            bool pathFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                pathFound,
                &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
                shaderFullPath.Native(),
                relativePath.Native(),
                rootFolder.Native());

            if (pathFound)
            {
                relativePath.ReplaceExtension("shadervariantlist");
                projectPath /= AZ::IO::Path ("ShaderVariants") / relativePath;
                result = projectPath.LexicallyNormal();
            }
            else
            {
                AZ_Error("ShaderManagementConsoleWindow", false, "Can not find a relative path from the shader: '%s'.", shaderFullPath.c_str());
            }
        }
        else if(msgBox.clickedButton() == engineBtn)
        {
            shaderFullPath.ReplaceExtension("shadervariantlist");
            result = shaderFullPath;
        }

        return result.Native();
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleWindow.cpp>
