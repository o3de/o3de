/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/AssetCollectionAsyncLoader.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/algorithm.h>

namespace AZ
{
    //! This is the Job class that runs until all assetPaths become valid AssetIds.
    //! A valid AssetId doesn't mean that the asset is ready and loaded in memory,
    //! it simply means the assetPath is acknowledged by the AP as an asset in the database
    //! that CAN be loaded. 
    class AssetDiscoveryJob : public Job
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetDiscoveryJob, ThreadPoolAllocator, 0)

        //! @queryWaitMilliseconds How long to wait between each attempt to query for assets that are not yet present in the asset database.
        AssetDiscoveryJob(AssetCollectionAsyncLoader& assetCollectionLoadManager,
                          const AZStd::vector<AssetCollectionAsyncLoader::AssetToLoadInfo>& assetList,
                          JobContext* context = nullptr, uint32_t queryWaitMilliseconds = 1000)
            : Job(false, context)
            , m_assetCollectionLoadManager(assetCollectionLoadManager)
            , m_assetList(assetList)
            , m_queryWaitMilliseconds(queryWaitMilliseconds)
            , m_isRunning(false)
        {
        }

        void Process() override
        {
            m_isRunning.store(true);
            while (!IsCancelled())
            {
                //Remove assets from m_assetList as they appear in the asset database.
                m_assetList.erase(AZStd::remove_if(m_assetList.begin(), m_assetList.end(),
                    [&](const AssetCollectionAsyncLoader::AssetToLoadInfo& assetToLoadInfo)
                    {
                        const auto& assetPath = assetToLoadInfo.m_assetPath;
                        const auto& assetType = assetToLoadInfo.m_assetType;

                        Data::AssetId assetId;
                        Data::AssetCatalogRequestBus::BroadcastResult(
                            assetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                            assetPath.c_str(), assetType, false);
                        if (assetId.IsValid())
                        {
                            // Notify the AssetCollectionAsyncLoader of such wonderful news.
                            m_assetCollectionLoadManager.OnAssetIsValid(assetPath, assetId, assetType);
                            return true;
                        }
                        return false;
                    }),
                m_assetList.end());

                // If the asset list is empty this job is done.
                if (m_assetList.empty())
                {
                    break;
                }

                //time to sleep.
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_queryWaitMilliseconds));
            }
            m_isRunning.store(false);
        }

        bool IsRunning() const
        {
            return m_isRunning.load();
        }

    private:
        AssetCollectionAsyncLoader& m_assetCollectionLoadManager;
        AZStd::vector<AssetCollectionAsyncLoader::AssetToLoadInfo> m_assetList;
        uint32_t m_queryWaitMilliseconds;
        AZStd::atomic_bool m_isRunning;
    }; // class AssetDiscoveryJob


    bool AssetCollectionAsyncLoader::LoadAssetsAsync(const AZStd::vector<AssetToLoadInfo>& assetList, OnAssetReadyCallback onAssetReadyCB)
    {
        if (assetList.empty())
        {
            return false;
        }

        {
            if (!!m_assetDiscoveryJob && (static_cast<AssetDiscoveryJob*>(m_assetDiscoveryJob.get()))->IsRunning())
            {
                AZ_Error(AssetCollectionAsyncLoaderName, false, "Pending requests must be cancelled before calling LoadAssetsAsync");
                return false;
            }
            AZStd::shared_lock<decltype(m_mutex)> lock(m_mutex);
            if (!m_assetsToLoad.empty())
            {
                AZ_Error(AssetCollectionAsyncLoaderName, false, "Some assets were still pending for loading, call Cancel before calling LoadAssetsAsync");
                return false;
            }
        }

        // Let's clear and reset to start with a clean slate.
        Cancel();

        AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);

        m_onAssetReadyCB = onAssetReadyCB;
        m_assetsToLoad.reserve(assetList.size());
        for (const auto& assetToLoadInfo : assetList)
        {
            const auto& assetPath = assetToLoadInfo.m_assetPath;
            AZ_Warning(AssetCollectionAsyncLoaderName, !m_assetsToLoad.count(assetPath), "Asset with path %s was already scheduled for loading", assetPath.c_str());
            m_assetsToLoad.insert(assetPath);
        }

        // Prepare to create a cancellable job.
        AZ::JobManagerDesc desc;
        desc.m_jobManagerName = "AssetCollectionAsyncLoader";

        AZ::JobManagerThreadDesc threadDesc;
        desc.m_workerThreads.push_back(threadDesc);
        m_jobManager = AZStd::make_unique<AZ::JobManager>(desc);
        m_jobCancelGroup = AZStd::make_unique<AZ::JobCancelGroup>();
        m_jobContext = AZStd::make_unique<AZ::JobContext>(*m_jobManager, *m_jobCancelGroup);
        m_jobUsedForCancellation = AZStd::make_unique<AZ::JobCompletion>(m_jobContext.get());
        m_jobUsedForCancellation->Reset(true);

        //Kick off the job.
        m_assetDiscoveryJob = AZStd::make_unique<AssetDiscoveryJob>(*this, assetList, m_jobContext.get());
        m_assetDiscoveryJob->SetDependent(m_jobUsedForCancellation.get());
        m_assetDiscoveryJob->Start();

        return true;

    }

    void AssetCollectionAsyncLoader::Cancel()
    {
        if (!m_assetDiscoveryJob)
        {
            return;
        }
        m_jobCancelGroup->Cancel();
        m_jobUsedForCancellation->StartAndWaitForCompletion();
        m_jobCancelGroup->Reset();

        m_assetDiscoveryJob = nullptr;
        m_jobUsedForCancellation = nullptr;
        m_jobContext = nullptr;
        m_jobCancelGroup = nullptr;
        m_jobManager = nullptr;

        AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
        ResetLocked();
    }

    void AssetCollectionAsyncLoader::ResetLocked()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        m_onAssetReadyCB = nullptr;
        m_assetsToLoad.clear();
        m_assetsToNotify.clear();
        m_assetIdStrToAssetPath.clear();
        m_readyAssets.clear();
    }

    void AssetCollectionAsyncLoader::PostNotifyReadyAssetsCB(Data::Asset<Data::AssetData> asset, bool success)
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
        AZStd::string assetIdStr = asset.GetId().ToString<AZStd::string>();

        // Find the asset in private list.
        {
            AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);

            const auto findPathIt = m_assetIdStrToAssetPath.find(assetIdStr);
            AZ_Assert(findPathIt != m_assetIdStrToAssetPath.end(), "Got an update for asset %s but it doesn't belong to this load manager\n", asset.GetHint().c_str());
            AZStd::string assetPath = findPathIt->second;

            AZ_Assert(m_assetsToLoad.count(assetPath), "Asset with path %s, hint %s was not scheduled to load\n", assetPath.c_str(), asset.GetHint().c_str());

            m_assetsToLoad.erase(assetPath);
            m_readyAssets[assetPath] = asset;
            m_assetsToNotify[assetPath] = success;
        }

        // Notify the caller on main thread.
        AZ::TickBus::QueueFunction([&]()
            {
                size_t pendingAssetsCount = 0;
                decltype(m_assetsToNotify) assetList;

                {
                    AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                    assetList.swap(m_assetsToNotify);
                    pendingAssetsCount = m_assetsToLoad.size();
                }

                for (const auto& [assetPath, assetSuccess] : assetList)
                {
                    m_onAssetReadyCB(assetPath, assetSuccess, pendingAssetsCount);
                }
            });
    }

    void AssetCollectionAsyncLoader::OnAssetIsValid(AZStd::string_view assetPath, const Data::AssetId& assetId, const Data::AssetType& assetType)
    {
        const auto assetIdStr = assetId.ToString<AZStd::string>();

        {
            AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
            m_assetIdStrToAssetPath[assetIdStr] = assetPath;
        }

        Data::AssetBus::MultiHandler::BusConnect(assetId);
        // Kick off asset loading.
        auto asset = Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();
    }

    ///////////////////////////////////////////////////////////////////////
    // AZ::Data::AssetBus::Handler overrides
    void AssetCollectionAsyncLoader::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        PostNotifyReadyAssetsCB(asset, true /*success*/);
    }

    void  AssetCollectionAsyncLoader::OnAssetError(Data::Asset<Data::AssetData> asset)
    {
        PostNotifyReadyAssetsCB(asset, false /*success*/);
    }
}
