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
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzCore/Utils/Utils.h>

#include <Data/ShaderVariantStatisticData.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <Window/ShaderManagementConsoleWindow.h>
#include <Window/ShaderManagementConsoleStatisticView.h>
#include <ShaderManagementConsoleRequestBus.h>

#include <QFileDialog>
#include <QMenu>
#include <QProgressDialog>
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

    AZStd::string ShaderManagementConsoleWindow::GetSaveDocumentParams(const AZStd::string& initialPath, const AZ::Uuid& documentId) const
    {
        // Get shader file path
        AZ::IO::Path shaderFullPath;
        AZ::RPI::ShaderVariantListSourceData shaderVariantList = {};
        ShaderManagementConsoleDocumentRequestBus::EventResult(
            shaderVariantList,
            documentId,
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

    void ShaderManagementConsoleWindow::CreateMenus(QMenuBar* menuBar)
    {
        Base::CreateMenus(menuBar);

        // Add statistic button
        QAction* action = new QAction(tr("Generate Shader Variant Statistic..."), m_menuFile);
        QObject::connect(action, &QAction::triggered, this, &ShaderManagementConsoleWindow::GenerateStatisticView);
        m_menuFile->insertAction(m_menuFile->actions().back(), action);
    }

    void ShaderManagementConsoleWindow::GenerateStatisticView()
    {
        AZStd::vector<AZ::Data::AssetId> materialAssetIdList;
        ShaderManagementConsoleRequestBus::BroadcastResult(
            materialAssetIdList, &ShaderManagementConsoleRequestBus::Events::GetAllMaterialAssetIds);

        QProgressDialog progressDialog(AzToolsFramework::GetActiveWindow());
        progressDialog.setWindowModality(Qt::WindowModal);
        progressDialog.setMaximum(static_cast<int>(materialAssetIdList.size()));
        progressDialog.setMaximumWidth(400);
        progressDialog.setMaximumHeight(100);
        progressDialog.setWindowTitle(tr("Gather information from material assets"));
        progressDialog.setLabelText(tr("Gather shader variant information..."));

        ShaderVariantStatisticData statisticData;
        for (int i = 0; i < materialAssetIdList.size(); ++i)
        {
            AZStd::vector<AZ::RPI::ShaderCollection::Item> shaderItemList;
            ShaderManagementConsoleRequestBus::BroadcastResult(
                shaderItemList, &ShaderManagementConsoleRequestBus::Events::GetMaterialInstanceShaderItems, materialAssetIdList[i]);

            for (auto& shaderItem : shaderItemList)
            {
                AZ::RPI::ShaderVariantId shaderVariantId = shaderItem.GetShaderVariantId();

                if (!shaderVariantId.IsEmpty())
                {
                    if (statisticData.m_shaderVariantUsage.find(shaderVariantId) == statisticData.m_shaderVariantUsage.end())
                    {
                        // Varient not found
                        statisticData.m_shaderVariantUsage[shaderVariantId].m_count = 1;
                    }
                    else
                    {
                        statisticData.m_shaderVariantUsage[shaderVariantId].m_count += 1;
                    }

                    AZ::RPI::ShaderOptionGroup shaderOptionGroup = shaderItem.GetShaderOptionGroup();
                    statisticData.m_shaderVariantUsage[shaderVariantId].m_shaderOptionGroup = shaderOptionGroup;

                    for (auto& shaderOptionDescriptor : shaderOptionGroup.GetShaderOptionDescriptors())
                    {
                        AZ::Name optionName = shaderOptionDescriptor.GetName();
                        AZ::RPI::ShaderOptionValue optionValue = shaderOptionGroup.GetValue(optionName);

                        if (!optionValue.IsValid())
                        {
                            continue;
                        }
                        AZ::Name valueName = shaderOptionDescriptor.GetValueName(optionValue);
                        if (statisticData.m_shaderOptionUsage.find(optionName) == statisticData.m_shaderOptionUsage.end())
                        {
                            // Option Not found
                            statisticData.m_shaderOptionUsage[optionName][valueName] = 1;
                        }
                        else
                        {
                            if (statisticData.m_shaderOptionUsage[optionName].find(valueName) ==
                                statisticData.m_shaderOptionUsage[optionName].end())
                            {
                                // Value not found
                                statisticData.m_shaderOptionUsage[optionName][valueName] = 1;
                            }
                            else
                            {
                                statisticData.m_shaderOptionUsage[optionName][valueName] += 1;
                            }
                        }
                    }
                }
            }

            progressDialog.setValue(i);

            if (progressDialog.wasCanceled())
            {
                return;
            }
        }
        progressDialog.close();

        if (m_statisticView)
        {
            delete m_statisticView;
            m_statisticView = nullptr;
        }

        m_statisticView = new ShaderManagementConsoleStatisticView(statisticData, nullptr);
        m_statisticView->setWindowTitle(tr("Shader Variant Statistic View"));
        m_statisticView->show();
    }

    void ShaderManagementConsoleWindow::closeEvent(QCloseEvent* closeEvent)
    {
        if (m_statisticView)
        {
            m_statisticView->close();
            delete m_statisticView;
            m_statisticView = nullptr;
        }

        Base::closeEvent(closeEvent);
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleWindow.cpp>
