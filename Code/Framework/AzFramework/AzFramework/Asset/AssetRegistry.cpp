
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/AssetRegistry.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/SystemFile.h> // for max path

namespace AssetRegistryInternal
{
    // to prevent people from shooting themselves in the foot here we are going to normalize the path.
    // since sha1 is sensitive to both slash direction and case!
    // note that this is not generating asset IDs, its merely creating UUIDs that map from
    // asset path -> asset Id, so that storing asset path as strings is not necessary.
    AZ::Uuid CreateUUIDForName(const char* name)
    {
        if (!name)
        {
            return AZ::Uuid::CreateNull();
        }

        // avoid allocating memory.  Note that the input paths are expected to be relative paths
        // from the root and thus should be much shorter than AZ_MAX_PATH_LEN
        char tempBuffer[AZ_MAX_PATH_LEN] = { 0 };
        tempBuffer[AZ_ARRAY_SIZE(tempBuffer) - 1] = 0;
        
        // here we try to pass over the memory only once.
        for (AZStd::size_t pos = 0; pos < AZ_ARRAY_SIZE(tempBuffer) - 1; ++pos)
        {
            char currentValue = name[pos];
            if (!currentValue)
            {
                tempBuffer[pos] = 0;
                break;
            }
            else if (currentValue == '\\')
            {
                tempBuffer[pos] = '/';
            }
            else
            {
                tempBuffer[pos] = (char)tolower(currentValue);
            }
        }

        return AZ::Uuid::CreateName(tempBuffer);
    }
}

namespace AzFramework
{
    using namespace AssetRegistryInternal;
    //=========================================================================
    // AssetRegistry::Clear
    //=========================================================================
    void AssetRegistry::Clear()
    {
        m_assetDependencies = {};
        m_assetIdToInfo = AssetIdToInfoMap();
        m_assetPathToId = AssetPathToIdMap();
    }

    //=========================================================================
    // AssetRegistry::ReflectSerialize
    //=========================================================================
    void AssetRegistry::ReflectSerialize(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AZ::Data::AssetInfo>()
                ->Version(2)
                ->Field("assetId", &AZ::Data::AssetInfo::m_assetId)
                ->Field("relativePath", &AZ::Data::AssetInfo::m_relativePath)
                ->Field("sizeBytes", &AZ::Data::AssetInfo::m_sizeBytes)
                ->Field("assetType", &AZ::Data::AssetInfo::m_assetType);

            serializeContext->Class<AZ::Data::ProductDependency>()
                ->Version(1)
                ->Field("assetId", &AZ::Data::ProductDependency::m_assetId)
                ->Field("flags", &AZ::Data::ProductDependency::m_flags);

            serializeContext->Class<AssetRegistry>()
                ->Version(5)
                ->Field("m_assetIdToInfo", &AssetRegistry::m_assetIdToInfo)
                ->Field("m_assetPathToIdMap", &AssetRegistry::m_assetPathToId)
                ->Field("m_legacyAssetIdToRealAssetId", &AssetRegistry::m_legacyAssetIdToRealAssetId)
                ->Field("m_assetDependencies", &AssetRegistry::m_assetDependencies);
            // note that the above m_assetPathToIdMap used to be called m_assetPathToId in prior serialization
            // and m_assetPathToIdByUUID prior to that, so do not rename it to those more obvious fields in the future.
        }
    }

    //=========================================================================
    // AssetRegistry::RegisterAsset
    //=========================================================================
    void AssetRegistry::RegisterAsset(AZ::Data::AssetId id, const AZ::Data::AssetInfo& assetInfo)
    {
        // One day we'd like to remove the reverse lookup of name -> id since nothing should be recording asset names for purposes of logic or lookup.
        // But for now, we still support legacy systems which store asset relative pathnames instead of storing asset ID's.

        // to achieve this we have two maps
        // one map which maps from [asset relative path] -> [Asset ID]
        // one map which maps from [Asset ID] -> [AssetInfo struct]
        
        // whats important to note is that the [asset relative path] map, for performance and memory purposes, uses the
        // SHA1 hash of the [asset relative path] instead of storing strings.
        // this makes it take a fixed amount of space and a fixed amount of time to do a lookup

        // whats also important to note is that sometimes, there are aliases for assets due to legacy systems.
        // in other words, two different AssetID's can map to the same file info about an asset, because they were known via a different ID-assignment system in the past.

        // Its not worth worrying about the case here where two source files (blah.png, blah.tif) output the same asset (blah.dds)
        // because either way, there is an asset there, so it has to be added to both maps.  there will just be two entries
        // in the [Asset ID] -> [AssetInfo struct] that points at the same output file.  Notifying the user that this has occurred
        // should happen at a much higher level.
        
        SetAssetIdByPath(assetInfo.m_relativePath.c_str(), id);
        m_assetIdToInfo.insert_key(id).first->second = assetInfo;
    }

    //=========================================================================
    // AssetRegistry::UnregisterAsset
    //=========================================================================
    void AssetRegistry::UnregisterAsset(AZ::Data::AssetId id)
    {
        // just as with RegisterAsset, its not worth worrying about the balance of these two maps.
        // When you delete an asset (ie, its actual data is gone) it MUST be removed from the [Id] -> [AssetInfo] map
        // and its irrelevant whether it gets removed from the [Path] -> [Id] map because that hash is only used
        // to resolve Ids, and if that resolved Id points at a missing asset, the [Path] -> [Id] map will know that.
        auto existingAsset = m_assetIdToInfo.find(id);
        if (existingAsset != m_assetIdToInfo.end())
        {
            m_assetPathToId.erase(CreateUUIDForName(existingAsset->second.m_relativePath.c_str()));
        }
        
        m_assetIdToInfo.erase(id);
        m_assetDependencies.erase(id);
    }

    void AssetRegistry::RegisterLegacyAssetMapping(const AZ::Data::AssetId& legacyId, const AZ::Data::AssetId& newId)
    {
        m_legacyAssetIdToRealAssetId[legacyId] = newId;
    }

    void AssetRegistry::UnregisterLegacyAssetMapping(const AZ::Data::AssetId& legacyId)
    {
        m_legacyAssetIdToRealAssetId.erase(legacyId);
    }

    void AssetRegistry::SetAssetDependencies(const AZ::Data::AssetId& id, const AZStd::vector<AZ::Data::ProductDependency>& dependencies)
    {
        m_assetDependencies[id] = dependencies;
    }

    void AssetRegistry::RegisterAssetDependency(const AZ::Data::AssetId& id, const AZ::Data::ProductDependency& dependency)
    {
        m_assetDependencies[id].push_back(dependency);
    }

    AZStd::vector<AZ::Data::ProductDependency> AssetRegistry::GetAssetDependencies(const AZ::Data::AssetId& id)
    {
        return m_assetDependencies[id];
    }

    AZ::Data::AssetId AssetRegistry::GetAssetIdByLegacyAssetId(const AZ::Data::AssetId& legacyAssetId) const
    {
        auto found = m_legacyAssetIdToRealAssetId.find(legacyAssetId);
        if (found != m_legacyAssetIdToRealAssetId.end())
        {
            return found->second;
        }
        return AZ::Data::AssetId();
    }

    AzFramework::AssetRegistry::LegacyAssetIdToRealAssetIdMap AssetRegistry::GetLegacyMappingSubsetFromRealIds(const AZStd::vector<AZ::Data::AssetId>& realIds) const
    {
        LegacyAssetIdToRealAssetIdMap subset;
        auto realIdsBeginItr = realIds.begin();
        auto realIdsEndItr = realIds.end();
        for (const auto& legacyToRealPair : m_legacyAssetIdToRealAssetId)
        {
            if (AZStd::find(realIdsBeginItr, realIdsEndItr, legacyToRealPair.second) != realIdsEndItr)
            {
                subset.insert(legacyToRealPair);
            }
        }
        return subset;
    }

    AZ::Data::AssetId AssetRegistry::GetAssetIdByPath(const char* assetPath) const
    {
        if ((!assetPath) || (assetPath[0] == 0))
        {
            // the empty path has no asset ID.
            return AZ::Data::AssetId(); 
        }

        auto entry = m_assetPathToId.find(CreateUUIDForName(assetPath));
        if (entry != m_assetPathToId.end())
        {
            return entry->second;
        }
        return AZ::Data::AssetId();
    }

    void AssetRegistry::SetAssetIdByPath(const char* assetPath, const AZ::Data::AssetId& id)
    {
        AZ_Assert(assetPath, "Invalid asset path provided to SetAssetID!\n");
        AZ_Assert(id.IsValid(), "Invalid asset id provided to SetAssetID!\n");
        if ((!assetPath) || (!id.IsValid()))
        {
            return;
        }
        
        m_assetPathToId.insert_key(CreateUUIDForName(assetPath)).first->second = AZStd::move(id);
    }

    void AssetRegistry::AddRegistry(AZStd::shared_ptr<AssetRegistry> assetRegistry)
    {
        for (const auto& element : assetRegistry->m_assetIdToInfo)
        {
            m_assetIdToInfo[element.first] = element.second;
            // remove dependency info that exists for this asset, as the change could have removed any dependenices this asset had.
            m_assetDependencies.erase(element.first);   
        }
        for (const auto& element : assetRegistry->m_assetDependencies)
        {
            m_assetDependencies[element.first] = element.second;
        }
        for (const auto& element : assetRegistry->m_assetPathToId)
        {
            m_assetPathToId[element.first] = element.second;
        }
        for (const auto& element : assetRegistry->m_legacyAssetIdToRealAssetId)
        {
            m_legacyAssetIdToRealAssetId[element.first] = element.second;
        }
    }

} // namespace AzFramework
