/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/AssetSystemBus.h>
 
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetSystem::AssetSystemNotifications);
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetSystem::AssetSystemRequest);
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetSystem::AssetSystemJobRequest);

namespace AZStd
{
    template class AZTF_API_EXPORT  vector<AzToolsFramework::AssetSystem::JobInfo>;
}

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        const char* JobStatusString(JobStatus status)
        {
            switch(status)
            {
                case JobStatus::Any: return "Any";
                case JobStatus::Queued: return "Queued";
                case JobStatus::InProgress: return "InProgress";
                case JobStatus::Failed: return "Failed";
                case JobStatus::Failed_InvalidSourceNameExceedsMaxLimit: return "Failed_InvalidSourceNameExceedsMaxLimit";
                case JobStatus::Completed: return "Completed";
                case JobStatus::Missing: return "Missing";
            }
            return "";
        }

        AZ::Data::AssetId FindAssetIdFromFileName(AZStd::string_view fileName, AZ::Data::AssetType assetTypeId)
        {
            using namespace AZ::Data;
            using assetSystemBus = AzFramework::AssetSystemRequestBus;
            AssetId assetId;
            if (fileName.empty())
            {
                return assetId;
            }

            // source search string is the given file name, made into a lexically normal path
            AZStd::string sourceSearchString = AZ::IO::PathView(fileName).LexicallyNormal().String();
            // product search string is the same, but lowercased.
            AZStd::string productSearchString = sourceSearchString;
            AZStd::to_lower(productSearchString.begin(), productSearchString.end());
          
            auto CheckCatalogFn = [](const AZStd::string& searchString, const AssetType& typeId) -> AssetId
            {
                // the fastest possible match is when the filename is a known asset file and is already available:
                AssetId retVal;
                AssetCatalogRequestBus::BroadcastResult(retVal, &AssetCatalogRequests::GetAssetIdByPath, searchString.c_str(), typeId, false);
                if (retVal.IsValid())
                {
                    if (typeId.IsNull())
                    {
                        return retVal;
                    }
                    // if we found an asset, but we also have an expected type, make sure it matches:
                    AssetInfo assetInfo;
                    AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequests::GetAssetInfoById, retVal);
                    if (assetInfo.m_assetType == typeId)
                    {
                        return retVal;
                    }
                }
                return {};
            };

            assetId = CheckCatalogFn(productSearchString, assetTypeId);
            if (assetId.IsValid())
            {
                return assetId;
            }

            // if we get here, three possibilities remain, and we'll tackle them in this order since each additional
            // number below introduces more delay time:
            // 1. the file is a source file, not a product file (asking for "blah.material" instead of "blah.azmaterial")
            // 2. the file is a product file, but the asset catalog is not up to date
            // 3. the file is missing entirely (it will never appear).

            using toolsAssetBus = AzToolsFramework::AssetSystemRequestBus;
            AZ::Data::AssetInfo sourceInfo;
            AZStd::string watchFolder; // this is the root folder the asset was found in, like, which Gem it belongs to for example
            toolsAssetBus::Broadcast(&toolsAssetBus::Events::GetSourceInfoBySourcePath, sourceSearchString.c_str(), sourceInfo, watchFolder);
            if (sourceInfo.m_assetId.IsValid())
            {
                // make sure its compiled, so that it has produced its products:
                assetSystemBus::Broadcast(&assetSystemBus::Events::CompileAssetSyncById, sourceInfo.m_assetId);
                // Get the asset id that the above source asset ("blah.material") produces, ie, "blah.azmaterial"
                AZStd::vector<AZ::Data::AssetInfo> producedAssets;
                toolsAssetBus::Broadcast(&toolsAssetBus::Events::GetAssetsProducedBySourceUUID, sourceInfo.m_assetId.m_guid, producedAssets);
                if (!assetTypeId.IsNull())
                {
                    // filter the list down to only those that match the expected type:
                    AZStd::erase_if(
                        producedAssets,
                        [assetTypeId](const AssetInfo& info)
                        {
                            return info.m_assetType != assetTypeId;
                        });
                }
                // find the best match for the given file name by checking the longest matching path portion
                // without case sensitivity and without slash direction sensitivity:
                size_t bestMatchLen = 0;
                for (const auto& product : producedAssets)
                {
                    AZStd::string pathString = AZ::IO::PathView(product.m_relativePath).LexicallyNormal().String();
                    AZStd::to_lower(pathString.begin(), pathString.end());
                    // find the length of the longest common prefix between the two strings:
                    size_t maxCompareLen = AZStd::min(pathString.size(), productSearchString.size());
                    size_t matchLen = 0;
                    for (size_t i = 0; i < maxCompareLen; ++i)
                    {
                        if (pathString[i] != productSearchString[i])
                        {
                            break;
                        }
                        matchLen++;
                    }
                    if (matchLen > bestMatchLen)
                    {
                        bestMatchLen = matchLen;
                        assetId = product.m_assetId;
                    }
                }
                return assetId;
            }
            else
            {
                // its not a source file name.  Last resort is to try to compile it and check it again:
                assetSystemBus::Broadcast(&assetSystemBus::Events::CompileAssetSync, productSearchString.c_str());
                assetId = CheckCatalogFn(productSearchString, assetTypeId);
                if (assetId.IsValid())
                {
                    return assetId;
                }
                // note that if this "else" happens, its the most expensive case, but once it passes, the next time
                // it will be fast since the asset will be compiled and in the catalog.
            }
            return {};
        }

        JobInfo::JobInfo()
        {
            m_jobRunKey = rand();
        }

        AZ::u32 JobInfo::GetHash() const
        {
            AZ::Crc32 crc(m_watchFolder.c_str());
            crc.Add(m_sourceFile.c_str());
            crc.Add(m_platform.c_str());
            crc.Add(m_jobKey.c_str());
            crc.Add(m_builderGuid.ToString<AZStd::string>().c_str());
            return crc;
        }

        void JobInfo::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<JobInfo>()
                    ->Version(4)
                    ->Field("sourceFile", &JobInfo::m_sourceFile)
                    ->Field("platform", &JobInfo::m_platform)
                    ->Field("builderUuid", &JobInfo::m_builderGuid)
                    ->Field("jobKey", &JobInfo::m_jobKey)
                    ->Field("jobRunKey", &JobInfo::m_jobRunKey)
                    ->Field("status", &JobInfo::m_status)
                    ->Field("firstFailLogTime", &JobInfo::m_firstFailLogTime)
                    ->Field("firstFailLogFile", &JobInfo::m_firstFailLogFile)
                    ->Field("lastFailLogTime", &JobInfo::m_lastFailLogTime)
                    ->Field("lastFailLogFile", &JobInfo::m_lastFailLogFile)
                    ->Field("lastLogTime", &JobInfo::m_lastLogTime)
                    ->Field("lastLogFile", &JobInfo::m_lastLogFile)
                    ->Field("jobID", &JobInfo::m_jobID)
                    ->Field("watchFolder", &JobInfo::m_watchFolder)
                    ->Field("errorCount", &JobInfo::m_errorCount)
                    ->Field("warningCount", &JobInfo::m_warningCount)
                    ;
            }
        }
    }
}
