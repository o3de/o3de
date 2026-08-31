/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace AZ::Meshlets
{
    //! Maps a source ModelAsset.AssetId to its corresponding
    //! .azmeshletpack product AssetId. Populated by walking the
    //! AssetCatalog at startup for all .azmeshletpack products and reading
    //! their PackHeaderRecord.m_sourceModelAssetId. Cached for O(1) lookup
    //! at AcquireInstance time.
    //!
    //! Subscribes to AzFramework::AssetCatalogEventBus so packs added/removed
    //! by AssetProcessor while the editor is running update the map live —
    //! without this, a freshly-baked .azmeshletpack wouldn't appear until
    //! the editor restarts.
    class PackResolver
        : private AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        PackResolver();
        ~PackResolver() override;

        //! Walk the AssetCatalog now and populate the mapping. Safe to call
        //! more than once (rebuilds the table). Call after the asset
        //! catalog is available (typically late in system-component
        //! Activate). Also connects to AssetCatalogEventBus for live
        //! updates if not already connected.
        void RebuildIndex();

        //! Returns the .azmeshletpack AssetId for a given .azmodel AssetId,
        //! or AZ::Data::AssetId() if no pack is associated.
        AZ::Data::AssetId Find(const AZ::Data::AssetId& modelAssetId) const;

        size_t GetMappingCount() const;

    protected:
        // AssetCatalogEventBus::Handler
        //! Fires when AssetProcessor adds a new product to the catalog
        //! (e.g., the JSON sidecar builder just produced a new
        //! .azmeshletpack). We blocking-load it, read its source-model
        //! id, and update the map.
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;

        //! Fires when an existing catalog asset changes — e.g., AP
        //! re-processed the sidecar after a builder version bump, or the
        //! artist edited the JSON's cluster params. Re-evaluate the pack
        //! the same way we would on Added: read its header, refresh the
        //! map entry. Critical for the case where the previous build of
        //! the pack had the wrong m_assetType in the catalog and got
        //! filtered out — without Changed handling the editor would
        //! never see the re-emitted product until restart.
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;

        //! Fires when a product is removed (e.g., the artist deleted the
        //! sidecar JSON and AP retired its product). Remove any map
        //! entries pointing at this pack id.
        void OnCatalogAssetRemoved(
            const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

    private:
        mutable AZStd::mutex m_mutex;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::AssetId> m_modelToPackMap;
    };

} // namespace AZ::Meshlets
