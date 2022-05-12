/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetContainer.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>

namespace AZ::Data
{
    AssetContainer::AssetContainer(Asset<AssetData> rootAsset, const AssetLoadParameters& loadParams)
    {
        m_rootAsset = AssetInternal::WeakAsset<AssetData>(rootAsset);
        m_containerAssetId = m_rootAsset.GetId();

        AddDependentAssets(rootAsset, loadParams);
    }

    AssetContainer::~AssetContainer()
    {
        // Validate that if the AssetManager is performing normal processing duties, the AssetContainer is only destroyed once all
        // dependent asset loads have completed.
        if (AssetManager::IsReady() && !AssetManager::Instance().ShouldCancelAllActiveJobs())
        {
            AZ_Assert(m_waitingCount == 0, "Container destroyed while dependent assets are still loading.  The dependent assets may "
                "end up in a perpetual loading state if there is no top-level container signalling the completion of the full load.");
        }

        AssetBus::MultiHandler::BusDisconnect();
        AssetLoadBus::MultiHandler::BusDisconnect();
    }

    AZStd::vector<AZStd::pair<AssetInfo, Asset<AssetData>>> AssetContainer::CreateAndQueueDependentAssets(
        const AZStd::vector<AssetInfo>& dependencyInfoList, const AssetLoadParameters& loadParamsCopyWithNoLoadingFilter)
    {
        AZStd::vector<AZStd::pair<AssetInfo, Asset<AssetData>>> dependencyAssets;

        for (auto& thisInfo : dependencyInfoList)
        {
            auto dependentAsset = AssetManager::Instance().FindOrCreateAsset(
                thisInfo.m_assetId, thisInfo.m_assetType, AZ::Data::AssetLoadBehavior::Default);

            if (!dependentAsset || !dependentAsset.GetId().IsValid())
            {
                AZ_Warning("AssetContainer", false, "Dependency Asset %s (%s) was not found\n",
                           thisInfo.m_assetId.ToString<AZStd::string>().c_str(), thisInfo.m_relativePath.c_str());
                RemoveWaitingAsset(thisInfo.m_assetId);
                continue;
            }
            dependencyAssets.emplace_back(thisInfo, AZStd::move(dependentAsset));
        }

        // Queue the loading of all of the dependent assets before loading the root asset.
        for (auto& [dependentAssetInfo, dependentAsset] : dependencyAssets)
        {
            // Queue each asset to load.
            auto queuedDependentAsset = AssetManager::Instance().GetAssetInternal(
                dependentAsset.GetId(), dependentAsset.GetType(),
                AZ::Data::AssetLoadBehavior::Default, loadParamsCopyWithNoLoadingFilter,
                dependentAssetInfo, HasPreloads(dependentAsset.GetId()));

            // Verify that the returned asset reference matches the one that we found or created and queued to load.
            AZ_Assert(dependentAsset == queuedDependentAsset, "GetAssetInternal returned an unexpected asset reference for Asset %s",
                      dependentAsset.GetId().ToString<AZStd::string>().c_str());
        }

        return dependencyAssets;
    }

    void AssetContainer::AddDependentAssets(Asset<AssetData> rootAsset, const AssetLoadParameters& loadParams)
    {
        AssetId rootAssetId = rootAsset.GetId();
        AssetType rootAssetType = rootAsset.GetType();

        // Every asset we're going to be waiting on a load for - the root and all valid dependencies
        AZStd::vector<AssetId> waitingList;
        waitingList.push_back(rootAssetId);

        // Every asset dependency that we're aware of, whether or not it gets filtered out by the asset filter callback.
        // This will be used at the point that asset references get serialized in to see whether or not we've received any
        // unexpected assets that didn't appear in our asset catalog dependency list that need to be loaded anyways.
        AZStd::vector<AssetId> handledAssetDependencyList;

        // Cached AssetInfo to save another lookup inside Assetmanager
        AZStd::vector<AssetInfo> dependencyInfoList;
        Outcome<AZStd::vector<ProductDependency>, AZStd::string> getDependenciesResult = Failure(AZStd::string());

        // Track preloads in an additional list - they're in our waiting/dependencyInfo lists as well, but preloads require us to
        // suppress emitting "AssetReady" until everything we care about in this context is ready
        PreloadAssetListType preloadDependencies;
        if (loadParams.m_dependencyRules == AssetDependencyLoadRules::UseLoadBehavior)
        {
            AZStd::unordered_set<AssetId> noloadDependencies;
            AssetCatalogRequestBus::BroadcastResult(getDependenciesResult, &AssetCatalogRequestBus::Events::GetLoadBehaviorProductDependencies,
                                                    rootAssetId, noloadDependencies, preloadDependencies);
            if (!noloadDependencies.empty())
            {
                AZStd::lock_guard<AZStd::recursive_mutex> dependencyLock(m_dependencyMutex);
                m_unloadedDependencies.insert(noloadDependencies.begin(), noloadDependencies.end());
            }
        }
        else if (loadParams.m_dependencyRules == AssetDependencyLoadRules::LoadAll)
        {
            AssetCatalogRequestBus::BroadcastResult(getDependenciesResult, &AssetCatalogRequestBus::Events::GetAllProductDependencies, rootAssetId);
        }
        // Do as much validation of dependencies as we can before the AddWaitingAssets and GetAsset calls for dependencies below
        if (getDependenciesResult.IsSuccess())
        {
            for (const auto& thisAsset : getDependenciesResult.GetValue())
            {
                AssetInfo assetInfo;
                AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, thisAsset.m_assetId);

                // No matter whether or not the asset dependency is valid, loaded, or filtered out, mark it as successfully handled.
                // When we encounter the asset reference during serialization, we will know that it should intentionally be skipped.
                // Otherwise, it would be treated as a missing dependency and assert.
                handledAssetDependencyList.emplace_back(thisAsset.m_assetId);

                if (!assetInfo.m_assetId.IsValid())
                {
                    // Handlers may just not currently be around for a given asset type so we only warn here
                    AZ_Warning("AssetContainer", false, "Asset %s (%s) references/depends on asset %s which does not exist in the catalog and cannot be loaded.",
                        rootAsset.GetHint().c_str(),
                        rootAssetId.ToString<AZStd::string>().c_str(),
                        thisAsset.m_assetId.ToString<AZStd::string>().c_str());
                    m_invalidDependencies++;
                    continue;
                }
                if (assetInfo.m_assetId == rootAssetId)
                {
                    // Circular dependencies in our graph need to be raised as errors as they could cause problems elsewhere
                    AZ_Error("AssetContainer", false, "Circular dependency found under asset %s", rootAssetId.ToString<AZStd::string>().c_str());
                    m_invalidDependencies++;
                    continue;
                }
                if (!AssetManager::Instance().GetHandler(assetInfo.m_assetType))
                {
                    // Handlers may just not currently be around for a given asset type so we only warn here
                    m_invalidDependencies++;
                    continue;
                }
                if (loadParams.m_assetLoadFilterCB)
                {
                    if (!loadParams.m_assetLoadFilterCB({thisAsset.m_assetId, assetInfo.m_assetType,
                        AZ::Data::ProductDependencyInfo::LoadBehaviorFromFlags(thisAsset.m_flags) }))
                    {
                        continue;
                    }
                }
                dependencyInfoList.push_back(assetInfo);
            }
        }
        for (auto& thisInfo : dependencyInfoList)
        {
            waitingList.push_back(thisInfo.m_assetId);
        }

        // Add waiting assets ahead of time to hear signals for any which may already be loading
        AddWaitingAssets(waitingList);
        SetupPreloadLists(move(preloadDependencies), rootAssetId);

        auto loadParamsCopyWithNoLoadingFilter = loadParams;

        // All asset dependencies below the root asset should be provided by the asset catalog, and therefore should *not*
        // get triggered to load when the asset reference is serialized in.  However, it's useful to detect, warn, and handle
        // the case where the asset dependencies are NOT set up correctly.
        loadParamsCopyWithNoLoadingFilter.m_assetLoadFilterCB = [handledAssetDependencyList](const AssetFilterInfo& filterInfo)
        {
            // NoLoad dependencies should always get filtered out and not loaded.
            if (filterInfo.m_loadBehavior == AZ::Data::AssetLoadBehavior::NoLoad)
            {
                return false;
            }

            // In the normal case, the dependent asset appears in the handled asset list, and we should return false so that
            // the asset isn't attempted to be loaded, since the asset will already be triggered to get loaded or was possibly
            // already filtered out by the load filter callback.
            // In the error case, the asset dependencies haven't been produced by the builder correctly, so assets
            // have shown up that the asset container hasn't triggered to load and isn't listening for.  Assert that this case
            // has happened so that the builder for this asset type can be fixed.
            // Ideally we would proceed forward and load them by returning "true", but the triggered load would use this lambda
            // function as the asset load filter for that load as well, which isn't correct.  If we ever want to support that
            // behavior, we would need to rework the way filters work as well as the code in AssetSerializer.cpp to pass down
            // the loadParams.m_assetLoadFilterCB that was passed into the AddDependentAssets() methods to use as the dependent
            // asset filter instead of this lambda function.
            AZ_UNUSED(handledAssetDependencyList); // Prevent unused warning in release builds
            AZ_Assert(AZStd::find(handledAssetDependencyList.begin(), handledAssetDependencyList.end(), filterInfo.m_assetId) !=
                handledAssetDependencyList.end(),
                "Dependent Asset ID (%s) is expected to load, but the Asset Catalog has no dependency recorded. "
                "Examine the asset builder for the asset relying on this to ensure it is generating the correct dependencies.",
                filterInfo.m_assetId.ToString<AZStd::string>().c_str());

            // The dependent asset should have already been created and at least queued to load prior to reaching this point.
            // The asset serializer needs to get a successful result from FindAsset(), or else our asset reference will fail
            // to point to the asset data once it is loaded.
            if (!Data::AssetManager::Instance().FindAsset(filterInfo.m_assetId, AZ::Data::AssetLoadBehavior::Default))
            {
                AZ_Assert(!Data::AssetManager::Instance().FindAsset(filterInfo.m_assetId, AZ::Data::AssetLoadBehavior::Default),
                    "Dependent Asset ID (%s) can't be found in the AssetManager, which means the asset referencing it has probably "
                    "started loading before the dependent asset has been queued to load.  Verify that the asset dependencies have "
                    "been created correctly for the parent asset.",
                    filterInfo.m_assetId.ToString<AZStd::string>().c_str());
            }

            return false;
        };

        // This will contain the list of dependent assets that have been created (or found) and queued to load.
        // We also keep a copy of the AssetInfo structure as a small optimization to avoid a redundant lookup in GetAssetInternal.
        AZStd::vector<AZStd::pair<AssetInfo, Asset<AssetData>>> dependencyAssets;

        // Make sure all the dependencies are created first before we try to load them.
        // Since we've set the load filter to not load dependencies, we need to ensure all the assets are created beforehand
        // so the dependencies can be hooked up as soon as each asset gets serialized in, even if they start getting serialized
        // while we're still in the middle of triggering all of the asset loads below.
        dependencyAssets = CreateAndQueueDependentAssets(dependencyInfoList, loadParamsCopyWithNoLoadingFilter);

        // Add all of the queued dependent assets as dependencies
        {
            AZStd::lock_guard<AZStd::recursive_mutex> dependencyLock(m_dependencyMutex);
            for (auto& [dependentAssetInfo, dependentAsset] : dependencyAssets)
            {
                AddDependency(AZStd::move(dependentAsset));
            }
        }

        // Finally, after creating and queueing the dependent assets, queue the root asset.  This is saved until last to ensure that
        // it doesn't have any chance of serializing in until after all the dependent assets have been queued for loading and have
        // been added to the list of dependencies.
        auto thisAsset = AssetManager::Instance().GetAssetInternal(rootAssetId, rootAssetType, rootAsset.GetAutoLoadBehavior(),
            loadParamsCopyWithNoLoadingFilter, AssetInfo(), HasPreloads(rootAssetId));

        if (!thisAsset)
        {
            AZ_Assert(false, "Root asset with id %s failed to load, asset container is invalid.",
                rootAssetId.ToString<AZStd::string>().c_str());
            ClearWaitingAssets();
            // initComplete remains false, because we have failed to initialize successfully.
            return;
        }

        m_initComplete = true;

        // *After* setting initComplete to true, check to see if the assets are already ready.
        // This check needs to wait until after setting initComplete because if they *are* ready, we want the final call to
        // RemoveWaitingAsset to trigger the OnAssetContainerReady/Canceled event.  If we call CheckReady() *before* setting
        // initComplete, if all the assets are ready, the event will never get triggered.
        CheckReady();
    }

    bool AssetContainer::IsReady() const
    {
        return (m_rootAsset && m_waitingCount == 0);
    }

    bool AssetContainer::IsLoading() const
    {
        return (m_rootAsset || m_waitingCount);
    }

    bool AssetContainer::IsValid() const
    {
        return (m_containerAssetId.IsValid() && m_initComplete && m_rootAsset);
    }

    void AssetContainer::CheckReady()
    {
        if (!m_dependencies.empty())
        {
            for (auto& [assetId, dependentAsset] : m_dependencies)
            {
                if (dependentAsset->IsReady() || dependentAsset->IsError())
                {
                    HandleReadyAsset(dependentAsset);
                }
            }
        }
        if (auto asset = m_rootAsset.GetStrongReference(); asset.IsReady() || asset.IsError())
        {
            HandleReadyAsset(asset);
        }
    }

    Asset<AssetData> AssetContainer::GetRootAsset()
    {
        return m_rootAsset.GetStrongReference();
    }

    AssetId AssetContainer::GetContainerAssetId()
    {
        return m_containerAssetId;
    }

    void AssetContainer::ClearRootAsset()
    {
        AssetId rootId = m_rootAsset.GetId();

        {
            AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);

            // Erase the entry in the preloadWaitList for the root asset if one exists.
            m_preloadWaitList.erase(rootId);

            // It's possible that the root asset has preload dependencies, so make sure to check the preload list and remove
            // the entry for the root asset if it has one.
            auto rootAssetPreloadIter = m_preloadList.find(rootId);
            if (rootAssetPreloadIter != m_preloadList.end())
            {
                // Since the root asset has a preload list, that means the preload wait list will also have references to the
                // root asset.  (The preload wait list is a list of assets waiting on a preload asset to finish)  Clear those
                // out as well.
                auto waitAssetSet = rootAssetPreloadIter->second;
                for (auto& waitId : waitAssetSet)
                {
                    auto waitAssetIter = m_preloadWaitList.find(waitId);
                    if (waitAssetIter != m_preloadWaitList.end())
                    {
                        waitAssetIter->second.erase(rootId);
                    }
                }

                m_preloadList.erase(rootAssetPreloadIter);
            }
        }

        // Clear out the root asset before removing it from the waiting list to ensure that we trigger an "OnAssetContainerCanceled"
        // event instead of "OnAssetContainerReady".
        m_rootAsset = {};
        RemoveWaitingAsset(rootId);

    }

    void AssetContainer::AddDependency(const Asset<AssetData>& newDependency)
    {
        m_dependencies[newDependency->GetId()] = newDependency;
    }
    void AssetContainer::AddDependency(Asset<AssetData>&& newDependency)
    {
        m_dependencies[newDependency->GetId()] = AZStd::move(newDependency);
    }

    void AssetContainer::OnAssetReady(Asset<AssetData> asset)
    {
        HandleReadyAsset(asset);
    }

    void AssetContainer::OnAssetError(Asset<AssetData> asset)
    {
        AZ_Warning("AssetContainer", false, "Error loading asset %s", asset->GetId().ToString<AZStd::string>().c_str());
        HandleReadyAsset(asset);
    }

    void AssetContainer::HandleReadyAsset(Asset<AssetData> asset)
    {
        // Wait until we've finished initialization before allowing this
        // If a ready event happens before we've gotten all the maps/structures set up, there may be some missing data
        // which can lead to a crash
        // We'll go through and check the ready status of every dependency immediately after finishing initialization anyway
        if (m_initComplete)
        {
        RemoveFromAllWaitingPreloads(asset->GetId());
        RemoveWaitingAsset(asset->GetId());
    }
    }

    void AssetContainer::OnAssetDataLoaded(Asset<AssetData> asset)
    {
        // Remove only from this asset's waiting list.  Anything else should
        // listen for OnAssetReady as the true signal.  This is essentially removing the
        // "marker" we placed in SetupPreloads that we need to wait for our own data
        RemoveFromWaitingPreloads(asset->GetId(), asset->GetId());
    }

    void AssetContainer::RemoveFromWaitingPreloads(const AssetId& waiterId, const AssetId& preloadID)
    {
        {
            AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);

            auto remainingPreloadIter = m_preloadList.find(waiterId);
            if (remainingPreloadIter == m_preloadList.end())
            {
                // If we got here without an entry on the preload list, it probably means this asset was triggered to load multiple
                // times, some with dependencies and some without.  To ensure that we don't disturb the loads that expect the
                // dependencies, just silently return and don't treat the asset as finished loading.  We'll rely on the other load
                // to send an OnAssetReady() whenever its expected dependencies are met.
                return;
            }
            if (!remainingPreloadIter->second.erase(preloadID))
            {
                AZ_Warning("AssetContainer", !m_initComplete, "Couldn't remove %s from waiting list of %s", preloadID.ToString<AZStd::string>().c_str(), waiterId.ToString<AZStd::string>().c_str());
                return;
            }
            if (!remainingPreloadIter->second.empty())
            {
                return;
            }
        }
        auto thisAsset = GetAssetData(waiterId);
        AssetManager::Instance().ValidateAndPostLoad(thisAsset, true, false, nullptr);
    }

    void AssetContainer::RemoveFromAllWaitingPreloads(const AssetId& thisId)
    {
        AZStd::unordered_set<AssetId> checkList;
        {
            AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);

            auto waitingList = m_preloadWaitList.find(thisId);
            if (waitingList != m_preloadWaitList.end())
            {
                checkList = move(waitingList->second);
                m_preloadWaitList.erase(waitingList);
            }
        }
        for (auto& thisDepId : checkList)
        {
            if (thisDepId != thisId)
            {
                RemoveFromWaitingPreloads(thisDepId, thisId);
            }
        }
    }

    void AssetContainer::ClearWaitingAssets()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
        m_waitingCount = 0;
        for (auto& thisAsset : m_waitingAssets)
        {
            AssetBus::MultiHandler::BusDisconnect(thisAsset);
        }
        m_waitingAssets.clear();
    }

    void AssetContainer::ListWaitingAssets() const
    {
#if defined(AZ_ENABLE_TRACING)
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
        AZ_TracePrintf("AssetContainer", "Waiting on assets:\n");
        for (auto& thisAsset : m_waitingAssets)
        {
            AZ_TracePrintf("AssetContainer", "  %s\n",thisAsset.ToString<AZStd::string>().c_str());
        }
#endif
    }

    void AssetContainer::ListWaitingPreloads([[maybe_unused]] const AssetId& assetId) const
    {
#if defined(AZ_ENABLE_TRACING)
        AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);
        auto preloadEntry = m_preloadList.find(assetId);
        if (preloadEntry != m_preloadList.end())
        {
            AZ_TracePrintf("AssetContainer", "%s waiting on preloads : \n",assetId.ToString<AZStd::string>().c_str());
            for (auto& thisId : preloadEntry->second)
            {
                AZ_TracePrintf("AssetContainer", "  %s\n",thisId.ToString<AZStd::string>().c_str());
            }
        }
        else
        {
            AZ_TracePrintf("AssetContainer", "%s isn't waiting on any preloads:\n", assetId.ToString<AZStd::string>().c_str());
        }
#endif
    }

    void AssetContainer::AddWaitingAssets(const AZStd::vector<AssetId>& assetList)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
        for (auto& thisAsset : assetList)
        {
            if (m_waitingAssets.insert(thisAsset).second)
            {
                ++m_waitingCount;
                AssetBus::MultiHandler::BusConnect(thisAsset);
                AssetLoadBus::MultiHandler::BusConnect(thisAsset);
            }
        }
    }

    void AssetContainer::AddWaitingAsset(const AssetId& thisAsset)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
        if (m_waitingAssets.insert(thisAsset).second)
        {
            ++m_waitingCount;
            AssetBus::MultiHandler::BusConnect(thisAsset);
            AssetLoadBus::MultiHandler::BusConnect(thisAsset);
        }
    }

    void AssetContainer::RemoveWaitingAsset(const AssetId& thisAsset)
    {
        bool allReady{ false };
        {
            bool disconnectEbus = false;

            { // Intentionally limiting lock scope
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_readyMutex);
                // If we're trying to remove something already removed, just ignore it
                if (m_waitingAssets.erase(thisAsset))
                {
                    m_waitingCount -= 1;
                    disconnectEbus = true;

                }
                if (m_waitingAssets.empty())
                {
                    allReady = true;
                }
            }

            if(disconnectEbus)
            {
                AssetBus::MultiHandler::BusDisconnect(thisAsset);
                AssetLoadBus::MultiHandler::BusDisconnect(thisAsset);
            }
        }

        // If there are no assets left to be loaded, trigger the final AssetContainer notification (ready or canceled).
        // We guard against prematurely sending it (m_initComplete) because it's possible for assets to get removed from our waiting
        // list *while* we're still building up the list, so the list would appear to be empty too soon.
        // We also guard against sending it multiple times (m_finalNotificationSent), because in some error conditions, it may be
        // possible to try to remove the same asset multiple times, which if it's the last asset, it could trigger multiple
        // notifications.
        if (allReady && m_initComplete && !m_finalNotificationSent)
        {
            m_finalNotificationSent = true;
            if (m_rootAsset)
            {
                AssetManagerBus::Broadcast(&AssetManagerBus::Events::OnAssetContainerReady, this);
            }
            else
            {
                AssetManagerBus::Broadcast(&AssetManagerBus::Events::OnAssetContainerCanceled, this);
            }
        }
    }

    AssetContainer::operator bool() const
    {
        return m_rootAsset ? true : false;
    }

    const AssetContainer::DependencyList& AssetContainer::GetDependencies() const
    {
        return m_dependencies;
    }

    const AZStd::unordered_set<AssetId>& AssetContainer::GetUnloadedDependencies() const
    {
        return m_unloadedDependencies;
    }

    void AssetContainer::SetupPreloadLists(PreloadAssetListType&& preloadList, const AssetId& rootAssetId)
    {
        if (!preloadList.empty())
        {
            // This method can be entered as additional NoLoad dependency groups are loaded - the container could
            // be in the middle of loading so we need to grab both mutexes.
            AZStd::scoped_lock<AZStd::recursive_mutex, AZStd::recursive_mutex> lock(m_readyMutex, m_preloadMutex);

            for (auto thisListPair = preloadList.begin(); thisListPair != preloadList.end();)
            {
                // We only should add ourselves if we have another valid preload we're waiting on
                bool foundAsset{ false };
                // It's possible this set of preload dependencies was culled out by lack of asset handler
                // Or filtering rules.  This is not an error, we should just remove it from the list of
                // Preloads we're waiting on
                if (!m_waitingAssets.count(thisListPair->first))
                {
                    thisListPair = preloadList.erase(thisListPair);
                    continue;
                }
                for (auto thisAsset = thisListPair->second.begin(); thisAsset != thisListPair->second.end();)
                {
                    // These are data errors.  We'll emit the error but carry on.  The container
                    // will load the assets but won't/can't create a circular preload dependency chain
                    if (*thisAsset == rootAssetId)
                    {
                        AZ_Error("AssetContainer", false, "Circular preload dependency found - %s has a preload"
                            "dependency back to root %s\n",
                            thisListPair->first.ToString<AZStd::string>().c_str(),
                            rootAssetId.ToString<AZStd::string>().c_str());
                        thisAsset = thisListPair->second.erase(thisAsset);
                        continue;
                    }
                    else if (*thisAsset == thisListPair->first)
                    {
                        AZ_Error("AssetContainer", false, "Circular preload dependency found - Root asset %s has a preload"
                            "dependency on %s which depends back back to itself\n",
                            rootAssetId.ToString<AZStd::string>().c_str(),
                            thisListPair->first.ToString<AZStd::string>().c_str());
                        thisAsset = thisListPair->second.erase(thisAsset);
                        continue;
                    }
                    else if (m_preloadWaitList.count(thisListPair->first) && m_preloadWaitList[thisListPair->first].count(*thisAsset))
                    {
                        AZ_Error("AssetContainer", false, "Circular dependency found - Root asset %s has a preload"
                            "dependency on %s which has a circular dependency with %s\n",
                            rootAssetId.ToString<AZStd::string>().c_str(),
                            thisListPair->first.ToString<AZStd::string>().c_str(),
                            thisAsset->ToString<AZStd::string>().c_str());
                        thisAsset = thisListPair->second.erase(thisAsset);
                        continue;
                    }
                    else if (m_waitingAssets.count(*thisAsset))
                    {
                        foundAsset = true;
                        m_preloadWaitList[*thisAsset].insert(thisListPair->first);
                        ++thisAsset;
                    }
                    else
                    {
                        // This particular preload dependency of this asset was culled
                        // similar to the case above this can be due to no established asset handler
                        // or filtering rules.  We'll just erase the entry because we're not loading this
                        thisAsset = thisListPair->second.erase(thisAsset);
                        continue;
                    }
                }
                if (foundAsset)
                {
                    // We've established that this asset has at least one preload dependency it needs to wait on
                    // so we additionally add the waiting asset as its own preload so all of our "waiting assets"
                    // are managed in the same list.  We can't consider this asset to be "ready" until all
                    // of its preloads are ready and it has been loaded.  It will request an OnAssetDataLoaded
                    // notification from AssetManager rather than an OnAssetReady because of these additional dependencies.
                    thisListPair->second.insert(thisListPair->first);
                    m_preloadWaitList[thisListPair->first].insert(thisListPair->first);
                }
                ++thisListPair;
            }
            for(auto& thisList : preloadList)
            {
                // Only save the entry to the final preload list if it has at least one dependent asset still remaining after
                // the checks above.
                if (!thisList.second.empty())
                {
                    m_preloadList[thisList.first].insert(thisList.second.begin(), thisList.second.end());
                }
            }
        }
    }

    bool AssetContainer::HasPreloads(const AssetId& assetId) const
    {
        AZStd::lock_guard<AZStd::recursive_mutex> preloadGuard(m_preloadMutex);
        auto preloadEntry = m_preloadList.find(assetId);
        if (preloadEntry != m_preloadList.end())
        {
            return !preloadEntry->second.empty();
        }
        return false;
    }

    Asset<AssetData> AssetContainer::GetAssetData(const AssetId& assetId) const
    {
        AZStd::lock_guard<AZStd::recursive_mutex> dependenciesGuard(m_dependencyMutex);
        if (auto rootAsset = m_rootAsset.GetStrongReference(); rootAsset.GetId() == assetId)
        {
            return rootAsset;
        }
        auto dependencyIter = m_dependencies.find(assetId);
        if (dependencyIter != m_dependencies.end())
        {
            return dependencyIter->second;
        }
        AZ_Warning("AssetContainer", false, "Asset %s not found in container", assetId.ToString<AZStd::string>().c_str());
        return {};
    }

    int AssetContainer::GetNumWaitingDependencies() const
    {
        return m_waitingCount.load();
    }

    int AssetContainer::GetInvalidDependencies() const
    {
        return m_invalidDependencies.load();
    }
} // namespace AZ::Data
