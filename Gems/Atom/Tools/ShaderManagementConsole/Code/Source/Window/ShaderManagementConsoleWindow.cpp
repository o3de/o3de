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

    void ShaderManagementConsoleWindow::OnDocumentModified(const AZ::Uuid& documentId)
    {
        Base::OnDocumentModified(documentId);
        m_documentInspector->SetDocumentId(documentId);
    }

    AZStd::string ShaderManagementConsoleWindow::GetSaveDocumentParams(const AZStd::string& initialPath) const
    {
        AZStd::string fullPath;
        if (initialPath.compare(""))
        {
            fullPath = initialPath;
        }
        else
        {
            // This is a generated shader variant list without a file path
            AZ::RPI::ShaderVariantListSourceData shaderVariantList = {};
            ShaderManagementConsoleDocumentRequestBus::EventResult(
                shaderVariantList,
                m_documentInspector->GetDocumentId(),
                &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantListSourceData);
            fullPath = AZ::RPI::AssetUtils::ResolvePathReference(initialPath, shaderVariantList.m_shaderFilePath);
            AzFramework::StringFunc::Path::ReplaceExtension(fullPath, "shadervariantlist");
        }

        QMessageBox msgBox;
        msgBox.setText("Where do you want to save the list?");
        QPushButton* projectBtn = msgBox.addButton(QObject::tr("Save to project"), QMessageBox::YesRole);
        QPushButton* engineBtn = msgBox.addButton(QObject::tr("Save to engine"), QMessageBox::NoRole);
        msgBox.exec();

        AZStd::string result;
        if (msgBox.clickedButton() == projectBtn)
        {
            char projectRoot[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", projectRoot, AZ_MAX_PATH_LEN);

            AZStd::string shaderName;
            AZStd::string shaderPath;
            AzFramework::StringFunc::Path::Split(fullPath.c_str(), nullptr, &shaderPath, &shaderName);
            AZStd::string relativePath = shaderPath.substr(shaderPath.find("Assets"), shaderPath.length());
            AzFramework::StringFunc::Path::StripComponent(relativePath);
            result = AZStd::string::format("%s\\ShaderVariants\\%s\\%s.shadervariantlist", projectRoot, relativePath.c_str(), shaderName.c_str());
            AzFramework::StringFunc::Path::Normalize(result);
        }
        else if(msgBox.clickedButton() == engineBtn)
        {
            result = fullPath;
        }

        return result;
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleWindow.cpp>
