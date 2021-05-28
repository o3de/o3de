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

#include <AzCore/Casting/lossy_cast.h>
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
        AZ::Uuid subIdHash = AZ::Uuid::CreateData(id.data(), id.size());
        return azlossy_caster(subIdHash.GetHash());
    }

    const AZStd::string& ProcessedObjectStore::GetId() const
    {
        return m_uniqueId;
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
