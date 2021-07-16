/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Component/EntityUtils.h>
#include <cinttypes>

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    namespace AssetHelpers
    {
        // Simplified function to trace messages
        void PrintInfo(const char* format, ...);

        // Simplifies the conversion of an AssetId to a string to avoid overly verbose trace calls
        inline AZStd::string AssetIdToString(AZ::Data::AssetId assetId) { return assetId.ToString<AZStd::string>(); }

        // Given the full path to the asset, attempt to get the AssetInfo
        bool GetAssetInfo(AZStd::string_view fullPath, AZ::Data::AssetInfo& outAssetInfo);
        
        // Given the AssetId to the asset, attempt to get the AssetInfo
        AZ::Data::AssetInfo GetAssetInfo(AZ::Data::AssetId assetId);

        // Find the AssetType for a given asset
        inline AZ::Data::AssetType GetAssetType(AZ::Data::AssetId assetId)
        {
            AZ::Data::AssetInfo assetInfo = GetAssetInfo(assetId);
            return assetInfo.m_assetType;
        }

        // Find the AssetType for a given asset by path
        AZ::Data::AssetType GetAssetType(const char* assetPath);

        // Get AssetInfo from the AssetSystem as opposed to the catalog
        AZ::Data::AssetInfo GetAssetInfo(AZ::Data::AssetId assetId, AZStd::string& rootFilePath);

        AZ::Data::AssetInfo GetSourceInfo(const AZStd::string& sourceFilePath, AZStd::string& watchFolder);

        AZ::Data::AssetInfo GetSourceInfoByProductId(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType);

        void DumpAssetInfo(AZ::Data::AssetId assetId, const char* extra);

        bool IsValidSourceFile(const AZStd::string& filePath, ScriptCanvas::ScriptCanvasId scriptCanvasId);
    }
}
