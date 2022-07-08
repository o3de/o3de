/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Assets/ScriptCanvasAssetHelpers.h>


#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    namespace AssetHelpers
    {
        // Simplified function to trace messages
        void PrintInfo(const char* format, ...)
        {
            // TODO: Turn into something nice with enable_if
            static bool s_enabled = false;
            if (s_enabled)
            {
                char sBuffer[1024];
                va_list ArgList;
                va_start(ArgList, format);
                azvsnprintf(sBuffer, sizeof(sBuffer), format, ArgList);
                sBuffer[sizeof(sBuffer) - 1] = '\0';
                va_end(ArgList);

                AZ_TracePrintf("Script Canvas", "%s\n", sBuffer);
            }
        }

        // Given the full path to the asset, attempt to get the AssetInfo
        bool GetSourceInfo(AZStd::string_view fullPath, AZ::Data::AssetInfo& outAssetInfo)
        {
            AZStd::string watchFolder;
            AZ::Data::AssetInfo catalogAssetInfo;
            bool sourceInfoFound{};

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, fullPath.data(), catalogAssetInfo, watchFolder);
            if (sourceInfoFound)
            {
                outAssetInfo = catalogAssetInfo;
                return true;
            }

            return false;
        }

        AZ::Data::AssetInfo GetSourceInfo(const AZStd::string& sourceFilePath, AZStd::string& watchFolder)
        {
            AZ::Data::AssetInfo assetInfo;
            AzToolsFramework::AssetSystemRequestBus::Broadcast(&AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, sourceFilePath.c_str(), assetInfo, watchFolder);

            return assetInfo;
        }

        AZ::Data::AssetInfo GetSourceInfoByProductId(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType)
        {
            AzToolsFramework::AssetSystemRequestBus::Events* assetSystem = AzToolsFramework::AssetSystemRequestBus::FindFirstHandler();

            if (assetSystem == nullptr)
            {
                return AZ::Data::AssetInfo();
            }

            AZStd::string watchFolder;
            AZ::Data::AssetInfo assetInfo;
            const AZStd::string platformName = ""; // Empty for default

            if (!assetSystem->GetAssetInfoById(assetId, assetType, platformName, assetInfo, watchFolder))
            {
                return AZ::Data::AssetInfo();
            }

            return assetInfo;
        }

        void DumpAssetInfo(AZ::Data::AssetId assetId, [[maybe_unused]] const char* extra)
        {
            static bool s_enabled = false;
            if (!s_enabled)
            {
                return;
            }

            AZ::Data::AssetInfo assetInfo = GetSourceInfoByProductId(assetId, {});
            if (assetInfo.m_assetId.IsValid())
            {
                AZ_TracePrintf("Script Canvas", "-------------------------------------\n");
                AZ_TracePrintf("Script Canvas", "AssetId: %s", AssetIdToString(assetId).c_str());
                AZ_TracePrintf("Script Canvas", AZStd::string::format("AssetType: %s", assetInfo.m_assetType.ToString<AZStd::string>().c_str()).c_str());
                AZ_TracePrintf("Script Canvas", AZStd::string::format("RelativePath: %s", assetInfo.m_relativePath.c_str()).c_str());
                AZ_TracePrintf("Script Canvas", AZStd::string::format("Size in Byes: %llu", assetInfo.m_sizeBytes).c_str());
                AZ_TracePrintf("Script Canvas", AZStd::string::format("%s\n", extra).c_str());
                AZ_TracePrintf("Script Canvas", "-------------------------------------\n");
            }
            else
            {
                AZ_TracePrintf("Script Canvas", "Cannot DumpAssetInfo for Asset with ID: %s\n", AssetIdToString(assetId).c_str());
            }

        }

        bool IsValidSourceFile(const AZStd::string& filePath, [[maybe_unused]] ScriptCanvas::ScriptCanvasId scriptCanvasId)
        {
            return AZ::StringFunc::EndsWith(filePath, ScriptCanvasEditor::SourceDescription::GetFileExtension(), false);
        }
    }
}
