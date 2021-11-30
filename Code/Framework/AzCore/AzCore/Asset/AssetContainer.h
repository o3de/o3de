/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager_private.h>
#include <AzCore/Asset/AssetInternal/WeakAsset.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>

namespace AZ
{
    namespace Data
    {
        struct AssetLoadParameters;

        // AssetContainer loads an asset and all of its dependencies as a collection which is parallellized as much as possible.
        // With the container, the data will all load in parallel.  Dependent asset loads will still obey the expected rules
        // where PreLoad assets will emit OnAssetReady before the parent does, and QueueLoad assets will emit OnAssetReady in
        // no guaranteed order.  However, the OnAssetContainerReady signals will not emit until all PreLoad and QueueLoad assets
        // are ready.  NoLoad dependencies are not loaded by default but can be loaded along with their dependencies using the
        // same rules as above by using the LoadAll dependency rule.
        class AssetContainer :
            AZ::Data::AssetBus::MultiHandler,
            AZ::Data::AssetLoadBus::MultiHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetContainer, SystemAllocator, 0);

            AssetContainer() = default;

            AssetContainer(Asset<AssetData> asset, const AssetLoadParameters& loadParams);
            ~AssetContainer();

            bool IsReady() const;
            bool IsLoading() const;
            bool IsValid() const;

            /// Get a reference to the current root asset.
            /// This will either be the asset the container was originally created for, or invalid if the asset load has been canceled.
            Asset<AssetData> GetRootAsset();

            /// Get a reference to the asset id for the asset the container was originally created for.
            /// Even if the root asset has been cleared, this will still contain the originally-requested id.
            AssetId GetContainerAssetId();

            // Remove an asset from the container.
            void ClearRootAsset();


            operator bool() const;

            using DependencyList = AZStd::unordered_map< AZ::Data::AssetId, AZ::Data::Asset<AssetData>>;
            const DependencyList& GetDependencies() const;

            int GetNumWaitingDependencies() const;
            int GetInvalidDependencies() const;

            void ListWaitingAssets() const;
            void ListWaitingPreloads(const AZ::Data::AssetId& assetId) const;

            // Default behavior is to store dependencies flagged as "NoLoad" AutoLoadBehavior
            // These can be kicked off with a LoadDependency request

            const AZStd::unordered_set<AZ::Data::AssetId>& GetUnloadedDependencies() const;

            //////////////////////////////////////////////////////////////////////////
            // AssetBus
            void OnAssetReady(Asset<AssetData> asset) override;
            void OnAssetError(Asset<AssetData> asset) override;

            //////////////////////////////////////////////////////////////////////////
            // AssetLoadBus
            void OnAssetDataLoaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        protected:

            virtual AZStd::vector<AZStd::pair<AssetInfo, Asset<AssetData>>> CreateAndQueueDependentAssets(
                const AZStd::vector<AssetInfo>& dependencyInfoList, const AssetLoadParameters& loadParamsCopyWithNoLoadingFilter);

            // Waiting assets are those which have not yet signalled ready.  In the case of PreLoad dependencies the data may have completed the load cycle but
            // the Assets aren't considered "Ready" yet if there are PreLoad dependencies still loading and will still be in the list until the point that asset and
            // All of its preload dependencies have been loaded, when it signals OnAssetReady
            void AddWaitingAsset(const AZ::Data::AssetId& waitingAsset);
            void AddWaitingAssets(const AZStd::vector<AZ::Data::AssetId>& waitingAssets);
            void RemoveWaitingAsset(const AZ::Data::AssetId& waitingAsset);
            void ClearWaitingAssets();

            // Internal check to validate ready status at the end of initialization
            void CheckReady();

            // Add an individual asset to our list of known dependencies.  Does not include the root asset which is in m_rootAset
            void AddDependency(const Asset<AssetData>& newDependency);
            void AddDependency(Asset<AssetData>&& addDependency);

            // Add a "graph section" to our list of dependencies.  This checks the catalog for all Pre and Queue load assets which are dependents of the requested asset and kicks off loads
            // NoLoads which are encounted are placed in another list and can be loaded on demand with the LoadDependency call.
            void AddDependentAssets(Asset<AssetData> rootAsset, const AssetLoadParameters& loadParams);

            // If "PreLoad" assets are found in the graph these are cached and tracked with both OnAssetReady and OnAssetDataLoaded messages.
            // OnAssetDataLoaded is used to suppress what would normally be an OnAssetReady call - we need to use the container to evaluate whether
            // all of an asset's preload dependencies are ready before completing the load cycle where OnAssetReady will be signalled and the asset
            // will be removed from the waiting list in the container
            void SetupPreloadLists(PreloadAssetListType&& preloadList, const AZ::Data::AssetId& rootAssetId);
            bool HasPreloads(const AZ::Data::AssetId& assetId) const;

            // Remove a specific id from the list an asset is waiting for and complete the load if everything is ready
            void RemoveFromWaitingPreloads(const AZ::Data::AssetId& waitingId, const AZ::Data::AssetId& preloadAssetId);
            // Iterate over the list that was waiting for this asset and remove it from each
            void RemoveFromAllWaitingPreloads(const AZ::Data::AssetId& assetId);
            Asset<AssetData> GetAssetData(const AZ::Data::AssetId& assetId) const;

            // Used for final CheckReady after setup as well as internal handling for OnAssetReady
            // duringInit if we're coming from the checkReady method - containers that start ready don't need to signal
            void HandleReadyAsset(AZ::Data::Asset<AZ::Data::AssetData> asset);

            // Optimization to save the lookup in the dependencies map
            AssetInternal::WeakAsset<AssetData> m_rootAsset;

            // The root asset id is stored here semi-redundantly on initialization so that we can still refer to it even if the
            // root asset reference gets cleared.
            AssetId m_containerAssetId;

            mutable AZStd::recursive_mutex m_dependencyMutex;
            DependencyList m_dependencies;

            mutable AZStd::recursive_mutex m_readyMutex;
            AZStd::set<AssetId> m_waitingAssets;
            AZStd::atomic_int m_waitingCount{0};
            AZStd::atomic_int m_invalidDependencies{ 0 };
            AZStd::unordered_set<AZ::Data::AssetId> m_unloadedDependencies;
            AZStd::atomic_bool m_initComplete{ false };
            AZStd::atomic_bool m_finalNotificationSent{false};

            mutable AZStd::recursive_mutex m_preloadMutex;
            // AssetId -> List of assets it is still waiting on
            PreloadAssetListType m_preloadList;

            // AssetId -> List of assets waiting on it
            PreloadAssetListType m_preloadWaitList;
        private:
            AssetContainer operator=(const AssetContainer& copyContainer) = delete;
            AssetContainer operator=(const AssetContainer&& copyContainer) = delete;
            AssetContainer(const AssetContainer& copyContainer) = delete;
            AssetContainer(AssetContainer&& copyContainer) = delete;
        };

    }  // namespace Data
}   // namespace AZ



