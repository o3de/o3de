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

#pragma once

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Component/EntityUtils.h>
#include <cinttypes>

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
    }
}
