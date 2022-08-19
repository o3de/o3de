/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/wildcard.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <Material/MaterialBrowserInteractions.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>

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
            if (AZStd::wildcard_match("*.material", fullSourceFileName))
            {
                openers.push_back({ "Material_Editor", "Open in Material Editor...", QIcon(),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor,
                            fullSourceFileNameInCallback);
                    } });
            }
            if (AZStd::wildcard_match("*.materialcanvas.azasset", fullSourceFileName))
            {
                openers.push_back({ "Material_Canvas", "Open in Material Canvas (Experimental)...", QIcon(),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialCanvas,
                            fullSourceFileNameInCallback);
                    } });
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
                      const AZ::Data::AssetId assetId = AtomToolsFramework::GetSettingsObject<AZ::Data::AssetId>(
                          "/O3DE/Atom/MaterialEditor/DefaultMaterialTypeAsset",
                          AZ::RPI::AssetUtils::GetAssetIdForProductPath("materials/types/standardpbr.azmaterialtype"));
                      QWidget* mainWindow = nullptr;
                      AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                          mainWindow, &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

                      CreateDocumentDialog dialog( QObject::tr("Create Material"), QObject::tr("Select Type"), QObject::tr("Select Material Path"),
                          fullSourceFolderNameInCallback.c_str(), { "material" }, assetId,
                          [](const AZ::Data::AssetInfo& assetInfo)
                          {
                              return assetInfo.m_assetType == azrtti_typeid<AZ::RPI::MaterialTypeAsset>() &&
                                  IsDocumentPathEditable(AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId));
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
                      }
                  } });
        }

        bool MaterialBrowserInteractions::HandlesSource(AZStd::string_view fileName) const
        {
            return AZStd::wildcard_match("*.material", fileName.data()) ||
                AZStd::wildcard_match("*.materialcanvas.azasset", fileName.data());
        }
    } // namespace Render
} // namespace AZ
