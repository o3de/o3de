/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/TestUtils/AssetSystemStub.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    void AssetSystemStub::Activate()
    {
        m_sourcePath_sourceInfo_map.clear();
        m_sourceGuid_sourceInfo_map.clear();
        BusConnect();
    }

    void AssetSystemStub::Deactivate()
    {
        m_sourcePath_sourceInfo_map.clear();
        m_sourceGuid_sourceInfo_map.clear();
        BusDisconnect();
    }

    void AssetSystemStub::RegisterSourceInfo(const AZStd::string& sourcePath, const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        assetInfo.m_assetId = assetId;
        assetInfo.m_relativePath = sourcePath;
        RegisterSourceInfo(sourcePath, assetInfo, "");
    }

    void AssetSystemStub::RegisterSourceInfo(const AZStd::string& sourcePath, const AZ::Data::AssetInfo& assetInfo, const AZStd::string& watchFolder)
    {
        SourceInfo sourceInfo{ assetInfo, watchFolder };

        // Because GetSourceInfoBySourcePath should always return 0 for the sub-id, since it's about the source file not product file.
        sourceInfo.m_assetInfo.m_assetId.m_subId = 0; 

        AZStd::string normalizedSourcePath = sourcePath;
        AzFramework::StringFunc::Path::Normalize(normalizedSourcePath);
        m_sourcePath_sourceInfo_map.emplace(normalizedSourcePath, sourceInfo);
        m_sourceGuid_sourceInfo_map.emplace(assetInfo.m_assetId.m_guid, sourceInfo);
    }

    void AssetSystemStub::RegisterScanFolder(const AZStd::string& scanFolderPath)
    {
        AZStd::string normalizedScanFolderPath = scanFolderPath;
        AzFramework::StringFunc::Path::Normalize(normalizedScanFolderPath);
        m_scanFolders.push_back(normalizedScanFolderPath);
    }

    bool AssetSystemStub::GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
    {
        AZStd::string normalizedSourcePath = sourcePath;
        AzFramework::StringFunc::Path::Normalize(normalizedSourcePath);

        auto iter = m_sourcePath_sourceInfo_map.find(normalizedSourcePath);
        if (iter != m_sourcePath_sourceInfo_map.end())
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

    bool AssetSystemStub::GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
    {
        auto iter = m_sourceGuid_sourceInfo_map.find(sourceUuid);
        if (iter == m_sourceGuid_sourceInfo_map.end())
        {
            return false;
        }

        assetInfo = iter->second.m_assetInfo;
        watchFolder = iter->second.m_watchFolder;
        return true;
    }

    bool AssetSystemStub::GetScanFolders([[maybe_unused]] AZStd::vector<AZStd::string>& scanFolders)
    {
        scanFolders = m_scanFolders;
        return true;
    }

    bool AssetSystemStub::GetAssetSafeFolders([[maybe_unused]] AZStd::vector<AZStd::string>& assetSafeFolders)
    {
        assetSafeFolders = m_scanFolders;
        return true;
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
        for (AZStd::pair<AZStd::string, SourceInfo> element : m_sourcePath_sourceInfo_map)
        {
            if (element.second.m_assetInfo.m_assetId.m_guid == sourceUuid)
            {
                productsAssetInfo.push_back(element.second.m_assetInfo);
            }
        }
        return true;
    }
}
