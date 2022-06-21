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
        // Simplifies the conversion of an AssetId to a string to avoid overly verbose trace calls
        inline AZStd::string AssetIdToString(AZ::Data::AssetId assetId) { return assetId.ToString<AZStd::string>(); }

        // Given the full path to the source file, attempt to get the AssetInfo
        bool GetSourceInfo(AZStd::string_view fullPath, AZ::Data::AssetInfo& outAssetInfo);
        
        // Given the full path to the source file, attempt to get the AssetInfo and watch folder
        AZ::Data::AssetInfo GetSourceInfo(const AZStd::string& sourceFilePath, AZStd::string& watchFolder);

        // Given the full path to the source file, attempt to get the AssetInfo and assetType
        AZ::Data::AssetInfo GetSourceInfoByProductId(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType);

        void DumpAssetInfo(AZ::Data::AssetId assetId, const char* extra);

        bool IsValidSourceFile(const AZStd::string& filePath, ScriptCanvas::ScriptCanvasId scriptCanvasId);
    }
}
