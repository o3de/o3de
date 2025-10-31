/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/string/wildcard.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <Material/EditorMaterialComponentUtil.h>
#include <Material/MaterialBrowserInteractions.h>

namespace AZ
{
    namespace Render
    {
        MaterialBrowserInteractions::MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        }

        MaterialBrowserInteractions::~MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        }

        void MaterialBrowserInteractions::AddSourceFileOpeners(const char* fullSourceFileName, [[maybe_unused]] const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
        {
            const AZStd::string_view path(fullSourceFileName);
            if (path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialExtensionWithDot) ||
                path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialTypeExtensionWithDot))
            {
                openers.push_back({ "Material_Editor", "Open in Material Editor...", QIcon(":/Menu/material_editor.svg"),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor,
                            fullSourceFileNameInCallback);
                    } });
                return;
            }

            if (path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphExtensionWithDot) ||
                path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphNodeExtensionWithDot) ||
                path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphTemplateExtensionWithDot) ||
                path.ends_with(AZ::Render::EditorMaterialComponentUtil::ShaderExtensionWithDot))
            {
                openers.push_back({ "Material_Canvas", "Open in Material Canvas...", QIcon(":/Menu/material_canvas.svg"),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialCanvas,
                            fullSourceFileNameInCallback);
                    } });
                return;
            }
        }

        void MaterialBrowserInteractions::AddSourceFileCreators(
            [[maybe_unused]] const char* fullSourceFolderName,
            [[maybe_unused]] const AZ::Uuid& sourceUUID,
            AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
        {
            using namespace AtomToolsFramework;

            creators.push_back(
                { "Material_Creator", "Material", QIcon(),
                  [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                  {
                      const AZStd::string defaultMaterialType =
                          AtomToolsFramework::GetPathWithoutAlias(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                              "/O3DE/Atom/MaterialEditor/DefaultMaterialType",
                              "@gemroot:Atom_Feature_Common@/Assets/Materials/Types/StandardPBR.materialtype"));

                      QWidget* mainWindow = nullptr;
                      AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                          mainWindow, &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

                      CreateDocumentDialog dialog(
                          QObject::tr("Create Material"),
                          QObject::tr("Select Material Type"),
                          fullSourceFolderNameInCallback.empty() ? QObject::tr("Select Material Path") : QString(),
                          fullSourceFolderNameInCallback.c_str(),
                          { "material" },
                          defaultMaterialType.c_str(),
                          [](const AZStd::string& path)
                          {
                              return IsDocumentPathEditable(path) &&
                                  path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialTypeExtensionWithDot);
                          },
                          mainWindow);

                      dialog.adjustSize();

                      if (dialog.exec() == QDialog::Accepted && !dialog.m_sourcePath.isEmpty() && !dialog.m_targetPath.isEmpty())
                      {
                          uint32_t defaultMaterialVersion = 5;
                          const char* fullSourcePath = dialog.m_sourcePath.toUtf8().constData();
                          auto materialTypeSourceData = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(fullSourcePath);

                          if (materialTypeSourceData.IsSuccess())
                          {
                              defaultMaterialVersion = materialTypeSourceData.GetValue().m_version;
                          }
                          AZ::RPI::MaterialSourceData materialData;
                          materialData.m_materialTypeVersion = defaultMaterialVersion;
                          materialData.m_materialType = AtomToolsFramework::GetPathToExteralReference(
                              dialog.m_targetPath.toUtf8().constData(), dialog.m_sourcePath.toUtf8().constData());
                          dialog.m_sourcePath.toUtf8().constData();
                          materialData.m_parentMaterial = "";
                          AZ::RPI::JsonUtils::SaveObjectToFile(dialog.m_targetPath.toUtf8().constData(), materialData);
                          AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                              AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                              &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                              dialog.m_targetPath.toUtf8().constData(),
                              AZ::Crc32(),
                              false);
                      }
                  } });

            creators.push_back(
                { "Material_Graph_Creator", "Material Graph", QIcon(),
                  [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                  {
                      const AZStd::string defaultMaterialGraphTemplate =
                          AtomToolsFramework::GetPathWithoutAlias(AtomToolsFramework::GetSettingsValue<AZStd::string>(
                              "/O3DE/Atom/MaterialCanvas/DefaultMaterialGraphTemplate",
                              "@gemroot:MaterialCanvas@/Assets/MaterialCanvas/GraphData/blank_graph.materialgraphtemplate"));

                      QWidget* mainWindow = nullptr;
                      AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                          mainWindow, &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

                      CreateDocumentDialog dialog(
                          QObject::tr("Create Material Graph"),
                          QObject::tr("Select Material Graph Template"),
                          fullSourceFolderNameInCallback.empty() ? QObject::tr("Select Material Path") : QString(),
                          fullSourceFolderNameInCallback.c_str(),
                          { "materialgraph" },
                          defaultMaterialGraphTemplate.c_str(),
                          [](const AZStd::string& path)
                          {
                              return IsDocumentPathEditable(path) &&
                                  path.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphTemplateExtensionWithDot);
                          },
                          mainWindow);
                      dialog.adjustSize();

                      if (dialog.exec() == QDialog::Accepted && !dialog.m_sourcePath.isEmpty() && !dialog.m_targetPath.isEmpty())
                      {
                          AZ::IO::FileIOBase::GetInstance()->Copy(
                              dialog.m_sourcePath.toUtf8().constData(), dialog.m_targetPath.toUtf8().constData());
                          AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                              AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                              &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                              dialog.m_targetPath.toUtf8().constData(),
                              AZ::Crc32(),
                              false);
                      }
                  } });
        }

        bool MaterialBrowserInteractions::HandlesSource(AZStd::string_view fileName) const
        {
            return fileName.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialExtensionWithDot) ||
                fileName.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialTypeExtensionWithDot) ||
                fileName.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphExtensionWithDot) ||
                fileName.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphNodeExtensionWithDot) ||
                fileName.ends_with(AZ::Render::EditorMaterialComponentUtil::MaterialGraphTemplateExtensionWithDot) ||
                fileName.ends_with(AZ::Render::EditorMaterialComponentUtil::ShaderExtensionWithDot);
        }
    } // namespace Render
} // namespace AZ
