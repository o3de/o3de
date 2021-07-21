/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Spawnable/SpawnableAssetHandler.h>
#include <AzToolsFramework/Prefab/Spawnable/ProcesedObjectStore.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    ProcessedObjectStore::ProcessedObjectStore(AZStd::string uniqueId, AZStd::unique_ptr<AZ::Data::AssetData> asset, SerializerFunction assetSerializer)
        : m_uniqueId(AZStd::move(uniqueId))
        , m_assetSerializer(AZStd::move(assetSerializer))
        , m_asset(AZStd::move(asset))
    {}

    bool ProcessedObjectStore::Serialize(AZStd::vector<uint8_t>& output) const
    {
        if (m_assetSerializer)
        {
            return m_assetSerializer(output, *this);
        }
        else
        {
            return false;
        }
    }

    bool ProcessedObjectStore::HasAsset() const
    {
        return m_asset != nullptr;
    }

    const AZ::Data::AssetType& ProcessedObjectStore::GetAssetType() const
    {
        AZ_Assert(m_asset, "Called GetAssetType on ProcessedObjectStore when there was no valid asset.");
        return m_asset->GetType();
    }

    AZ::Data::AssetData& ProcessedObjectStore::GetAsset()
    {
        AZ_Assert(m_asset, "Called GetAsset on ProcessedObjectStore when there was no valid asset.");
        return *m_asset;
    }

    const AZ::Data::AssetData& ProcessedObjectStore::GetAsset() const
    {
        AZ_Assert(m_asset, "Called GetAsset on ProcessedObjectStore when there was no valid asset.");
        return *m_asset;
    }

    AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& ProcessedObjectStore::GetReferencedAssets()
    {
        return m_referencedAssets;
    }

    const AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& ProcessedObjectStore::GetReferencedAssets() const
    {
        return m_referencedAssets;
    }

    AZStd::unique_ptr<AZ::Data::AssetData> ProcessedObjectStore::ReleaseAsset()
    {
        return AZStd::move(m_asset);
    }

    uint32_t ProcessedObjectStore::BuildSubId(AZStd::string_view id)
    {
        return AzFramework::SpawnableAssetHandler::BuildSubId(id);
    }

    const AZStd::string& ProcessedObjectStore::GetId() const
    {
        return m_uniqueId;
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
