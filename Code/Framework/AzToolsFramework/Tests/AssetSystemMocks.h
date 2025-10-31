/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace UnitTests
{
    class MockAssetSystemRequest : public AzToolsFramework::AssetSystemRequestBus::Handler
    {
    public:
        MOCK_METHOD1(GetAbsoluteAssetDatabaseLocation, bool(AZStd::string&));
        MOCK_METHOD2(GetRelativeProductPathFromFullSourceOrProductPath, bool(const AZStd::string& fullPath, AZStd::string& relativeProductPath));
        MOCK_METHOD3(GenerateRelativeSourcePath,
            bool(const AZStd::string& sourcePath, AZStd::string& relativePath, AZStd::string& watchFolder));
        MOCK_METHOD2(GetFullSourcePathFromRelativeProductPath, bool(const AZStd::string& relPath, AZStd::string& fullSourcePath));
        MOCK_METHOD5(GetAssetInfoById, bool(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const AZStd::string& platformName, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath));
        MOCK_METHOD3(GetSourceInfoBySourcePath, bool(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder));
        MOCK_METHOD3(GetSourceInfoBySourceUUID, bool(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder));
        MOCK_METHOD1(GetScanFolders, bool(AZStd::vector<AZStd::string>& scanFolders));
        MOCK_METHOD1(GetAssetSafeFolders, bool(AZStd::vector<AZStd::string>& assetSafeFolders));
        MOCK_METHOD1(IsAssetPlatformEnabled, bool(const char* platform));
        MOCK_METHOD1(GetPendingAssetsForPlatform, int(const char* platform));
        MOCK_METHOD2(GetAssetsProducedBySourceUUID, bool(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo));
        MOCK_METHOD1(ClearFingerprintForAsset, bool(const AZStd::string& sourcePath));
    };
}
