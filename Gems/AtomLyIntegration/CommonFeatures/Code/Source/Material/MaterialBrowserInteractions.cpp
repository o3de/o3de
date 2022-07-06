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
            if (HandlesSource(fullSourceFileName))
            {
                openers.push_back({ "Material_Editor", "Open in Material Editor...", QIcon(),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor,
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
                  [&]([[maybe_unused]] const char* fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                  {
                      const AZ::Data::AssetId assetId = AtomToolsFramework::GetSettingsObject<AZ::Data::AssetId>(
                          "/O3DE/Atom/MaterialEditor/DefaultMaterialTypeAsset",
                          AZ::RPI::AssetUtils::GetAssetIdForProductPath("materials/types/standardpbr.azmaterialtype"));
                      QWidget* mainWindow = nullptr;
                      AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                          mainWindow, &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

                      CreateDocumentDialog dialog(
                          QObject::tr("Create Material"), QObject::tr("Select Type"), QObject::tr("Select Material Path"),
                          fullSourceFolderNameInCallback, { "material" }, assetId,
                          []([[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
                          {
                              AZStd::unordered_set<AZ::Uuid> supportedAssetTypesToCreate;
                              supportedAssetTypesToCreate.insert(azrtti_typeid<AZ::RPI::MaterialTypeAsset>());

                              // If any asset types are specified, do early rejection tests to avoid expensive string comparisons
                              const auto& assetTypes = supportedAssetTypesToCreate;
                              if (assetTypes.empty() || assetTypes.find(assetInfo.m_assetType) != assetTypes.end())
                              {
                                  // Additional filtering will be done against the path to the source file for this asset
                                  const auto& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);

                                  // Only add source files with extensions supported by the document types creation rules
                                  // Ignore any files that are marked as non editable in the registry
                                  return AZ::StringFunc::EndsWith(sourcePath.c_str(), AZ::RPI::MaterialTypeSourceData::Extension) &&
                                      IsDocumentPathEditable(sourcePath);
                              }
                              return false;
                          }, mainWindow);
                      dialog.adjustSize();

                      if (dialog.exec() == QDialog::Accepted && !dialog.m_sourcePath.isEmpty() && !dialog.m_targetPath.isEmpty())
                      {
                          AZ::RPI::MaterialSourceData materialData;
                          materialData.m_materialTypeVersion = 5;
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
            return AZStd::wildcard_match("*.material", fileName.data());
        }
    } // namespace Render
} // namespace AZ
