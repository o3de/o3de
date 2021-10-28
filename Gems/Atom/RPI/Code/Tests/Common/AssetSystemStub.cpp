/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Common/AssetSystemStub.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    void AssetSystemStub::Activate()
    {
        m_sourceInfoMap.clear();
        BusConnect();
    }

    void AssetSystemStub::Deactivate()
    {
        m_sourceInfoMap.clear();
        BusDisconnect();
    }

    void AssetSystemStub::RegisterSourceInfo(const char* sourcePath, const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        assetInfo.m_assetId = assetId;
        RegisterSourceInfo(sourcePath, assetInfo, "");
    }

    void AssetSystemStub::RegisterSourceInfo(const char* sourcePath, const AZ::Data::AssetInfo& assetInfo, const AZStd::string& watchFolder)
    {
        SourceInfo sourceInfo{ assetInfo , watchFolder };

        // Because GetSourceInfoBySourcePath should always return 0 for the sub-id, since it's about the source file not product file.
        sourceInfo.m_assetInfo.m_assetId.m_subId = 0; 

        AZStd::string normalizedSourcePath = sourcePath;
        AzFramework::StringFunc::Path::Normalize(normalizedSourcePath);
        m_sourceInfoMap.emplace(normalizedSourcePath, sourceInfo);
    }

    bool AssetSystemStub::GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
    {
        AZStd::string normalizedSourcePath = sourcePath;
        AzFramework::StringFunc::Path::Normalize(normalizedSourcePath);

        auto iter = m_sourceInfoMap.find(normalizedSourcePath);

        if (iter != m_sourceInfoMap.end())
        {
            assetInfo = iter->second.m_assetInfo;
            watchFolder = iter->second.m_watchFolder;
            return true;
        }

        return false;
    }

    bool AssetSystemStub::GetRelativeProductPathFromFullSourceOrProductPath([[maybe_unused]] const AZStd::string& fullPath, [[maybe_unused]] AZStd::string& relativeProductPath)
    {
        return false;
    }

    bool AssetSystemStub::GenerateRelativeSourcePath(
        [[maybe_unused]] const AZStd::string& sourcePath, [[maybe_unused]] AZStd::string& relativePath,
        [[maybe_unused]] AZStd::string& watchFolder)
    {
        return false;
    }

    bool AssetSystemStub::GetFullSourcePathFromRelativeProductPath(
        [[maybe_unused]] const AZStd::string& relPath, [[maybe_unused]] AZStd::string& fullSourcePath)
    {
        return false;
    }

    bool AssetSystemStub::GetAssetInfoById([[maybe_unused]] const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetType& assetType, [[maybe_unused]] const AZStd::string& platformName, [[maybe_unused]] AZ::Data::AssetInfo& assetInfo, [[maybe_unused]] AZStd::string& rootFilePath)
    {
        return false;
    }

    bool AssetSystemStub::GetSourceInfoBySourceUUID([[maybe_unused]] const AZ::Uuid& sourceUuid, [[maybe_unused]] AZ::Data::AssetInfo& assetInfo, [[maybe_unused]] AZStd::string& watchFolder)
    {
        return false;
    }

    bool AssetSystemStub::GetScanFolders([[maybe_unused]] AZStd::vector<AZStd::string>& scanFolders)
    {
        return false;
    }

    bool AssetSystemStub::GetAssetSafeFolders([[maybe_unused]] AZStd::vector<AZStd::string>& assetSafeFolders)
    {
        return false;
    }

    bool AssetSystemStub::IsAssetPlatformEnabled(const char* /*platform*/)
    {
        return false;
    }

    int AssetSystemStub::GetPendingAssetsForPlatform(const char*  /*platform*/)
    {
        return 0;
    }

    bool AssetSystemStub::GetAssetsProducedBySourceUUID(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo)
    {
        productsAssetInfo.clear();
        for (AZStd::pair<AZStd::string, SourceInfo> element : m_sourceInfoMap)
        {
            if (element.second.m_assetInfo.m_assetId.m_guid == sourceUuid)
            {
                productsAssetInfo.push_back(element.second.m_assetInfo);
            }
        }
        return true;
    }
}
