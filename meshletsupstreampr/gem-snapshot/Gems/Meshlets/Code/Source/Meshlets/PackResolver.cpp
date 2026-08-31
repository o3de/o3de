/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Meshlets/PackResolver.h>
#include <Meshlets/Reflect/MeshletPackAsset.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <cstring>

namespace AZ::Meshlets
{
    PackResolver::PackResolver() = default;

    PackResolver::~PackResolver()
    {
        if (AzFramework::AssetCatalogEventBus::Handler::BusIsConnected())
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        }
    }

    namespace
    {
        // Try to read a pack's source-model id from the catalog. Returns true on
        // success and writes the model id into outModelId.
        bool TryReadSourceModelIdFromPack(const AZ::Data::AssetId& packId, AZ::Data::AssetId& outModelId)
        {
            auto asset = AZ::Data::AssetManager::Instance()
                .GetAsset<MeshletPackAsset>(packId, AZ::Data::AssetLoadBehavior::PreLoad);
            asset.BlockUntilLoadComplete();
            if (!asset.IsReady()) return false;
            const PackHeaderRecord* h = asset->GetPackHeader();
            if (!h) return false;
            AZ::Uuid guid;
            std::memcpy(&guid, h->m_sourceModelGuid, sizeof(h->m_sourceModelGuid));
            outModelId = AZ::Data::AssetId(guid, h->m_sourceModelSubId);
            return true;
        }
    }

    namespace
    {
        // Shared body for Added and Changed: load the pack, read header,
        // refresh the model→pack map entry. Pulled out so the two event
        // handlers don't drift.
        void IngestPackByAssetId(
            const AZ::Data::AssetId& assetId,
            AZStd::mutex& mutex,
            AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::AssetId>& modelToPackMap,
            const char* eventLabel)
        {
            AZ::Data::AssetInfo info;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(info,
                &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
            if (info.m_assetType != azrtti_typeid<MeshletPackAsset>())
            {
                return;
            }

            AZ::Data::AssetId modelId;
            if (!TryReadSourceModelIdFromPack(assetId, modelId))
            {
                return;
            }

            AZStd::lock_guard<AZStd::mutex> lock(mutex);
            modelToPackMap[modelId] = assetId;
            AZ_TracePrintf("Meshlets",
                "PackResolver: %s — pack %s mapped to model %s\n",
                eventLabel,
                assetId.ToString<AZStd::string>().c_str(),
                modelId.ToString<AZStd::string>().c_str());
        }
    }

    void PackResolver::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        IngestPackByAssetId(assetId, m_mutex, m_modelToPackMap, "asset added");
    }

    void PackResolver::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        IngestPackByAssetId(assetId, m_mutex, m_modelToPackMap, "asset changed");
    }

    void PackResolver::OnCatalogAssetRemoved(
        const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo)
    {
        if (assetInfo.m_assetType != azrtti_typeid<MeshletPackAsset>())
        {
            return;
        }
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        // Linear sweep is fine — the map is at most O(meshlet-enabled-models).
        for (auto it = m_modelToPackMap.begin(); it != m_modelToPackMap.end(); )
        {
            if (it->second == assetId)
            {
                AZ_TracePrintf("Meshlets",
                    "PackResolver: removing pack %s for model %s (catalog asset removed)\n",
                    assetId.ToString<AZStd::string>().c_str(),
                    it->first.ToString<AZStd::string>().c_str());
                it = m_modelToPackMap.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void PackResolver::RebuildIndex()
    {
        if (!AzFramework::AssetCatalogEventBus::Handler::BusIsConnected())
        {
            // Listen for live catalog updates so packs added by AP after
            // editor startup (e.g., a fresh JSON sidecar AP just processed)
            // appear in the map without requiring a restart.
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_modelToPackMap.clear();

        const AZ::Data::AssetType packType = azrtti_typeid<MeshletPackAsset>();

        // EnumerateAssets is invoked via the canonical
        // AssetCatalogRequests::EnumerateAssets bus call. The begin / end
        // callbacks are no-args; the per-asset callback gets (id, info).
        auto onBegin = [](){};
        auto onEnd   = [](){};
        auto onAsset = [this, packType](const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (info.m_assetType != packType) return;

            // Blocking-load the pack to read its header. RebuildIndex is called once
            // at startup; steady-state lookups go through Find which is an
            // unordered_map probe.
            auto asset = AZ::Data::AssetManager::Instance()
                .GetAsset<MeshletPackAsset>(id, AZ::Data::AssetLoadBehavior::PreLoad);
            asset.BlockUntilLoadComplete();
            if (asset.IsReady())
            {
                const PackHeaderRecord* h = asset->GetPackHeader();
                if (h)
                {
                    // Reconstruct AssetId from raw GUID + sub-id (Task 2 spec correction).
                    AZ::Uuid guid;
                    std::memcpy(&guid, h->m_sourceModelGuid, sizeof(h->m_sourceModelGuid));
                    AZ::Data::AssetId modelId(guid, h->m_sourceModelSubId);
                    m_modelToPackMap[modelId] = id;
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequests::EnumerateAssets,
            onBegin, onAsset, onEnd);
    }

    AZ::Data::AssetId PackResolver::Find(const AZ::Data::AssetId& modelAssetId) const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        auto it = m_modelToPackMap.find(modelAssetId);
        return (it != m_modelToPackMap.end()) ? it->second : AZ::Data::AssetId();
    }

    size_t PackResolver::GetMappingCount() const
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        return m_modelToPackMap.size();
    }

} // namespace AZ::Meshlets
