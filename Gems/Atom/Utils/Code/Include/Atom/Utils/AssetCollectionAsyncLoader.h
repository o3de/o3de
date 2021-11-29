/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    //! Accepts a list of assets to load.
    //! Notifies (On Main Thread) with a callback once all assets have been loaded.
    //! REMARK 1:
    //!     This class goes beyond simply calling GetAssetIdByPath, because it accepts a list of Asset Products
    //!     that may not exist at all. For this reason it spawns a job that will call GetAssetIdByPath until it returns
    //!     a valid Data::Asset<> for all requested asset products.
    //!     There's a possibility one of the assets may never exist, and the job could run forever, it is up to the caller
    //!     to cancel the job because it is taking too long. IN AN IDEAL scenario this class could query with AzToolsFramework
    //!     API whenever the AssetProcessor is finished processing all assets and if one of the requested assets never became valid
    //!     the job could auto finish with an error to the user, BUT we can not use the AzToolsFramework.
    //! REMARK 2:
    //!     An asset can still be valid but may have errors. This case is handled properly and the user of this API will be notified
    //!     whenever there are such errors.
    class AssetCollectionAsyncLoader final
        : public AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_DISABLE_COPY_MOVE(AssetCollectionAsyncLoader);
        AssetCollectionAsyncLoader() = default;
        virtual ~AssetCollectionAsyncLoader() { Cancel(); };

        static constexpr char AssetCollectionAsyncLoaderName[] = "AssetCollectionAsyncLoader";

        //! The user must provide this type of callback when calling LoadAssetsAsync(...)
        using OnAssetReadyCallback = AZStd::function<void(AZStd::string_view /*assetPath*/, bool /*success*/, size_t /*pendingAssetCount*/)>;

        struct AssetToLoadInfo
        {
            AZStd::string m_assetPath;
            Data::AssetType m_assetType;
        };

        //! Returns true if the list of assets were successfully enqueued for loading.
        //! Returns false if there were jobs pending or the input list was empty.
        //! @assetReadyCB Called on Main Thread. As each asset is loaded this callback is triggered. When "pendingAssetCount" reaches zero the user knows all assets have been loaded.
        bool LoadAssetsAsync(const AZStd::vector<AssetToLoadInfo>& assetList, OnAssetReadyCallback onAssetReadyCB);

        //! Should be called after onAssetsReadyCB was fired at least once.
        template<class AssetClass>
        Data::Asset<AssetClass> GetAsset(AZStd::string_view assetPath) const
        {
            AZStd::shared_lock<decltype(m_mutex)> lock(m_mutex);
            const auto findIt = m_readyAssets.find(assetPath);
            if (findIt == m_readyAssets.end())
            {
                AZ_Error(AssetCollectionAsyncLoaderName, false, "Asset with path %s NOT found as ready", assetPath.data());
                return Data::Asset<AssetClass>();
            }
            Data::Asset<Data::AssetData> asset = findIt->second;
            return Data::static_pointer_cast<AssetClass>(asset);
        }

        //! Cancel any pending asset loading jobs, etc.
        //! If no job is pending this function does nothing.
        void Cancel();

    private:
        mutable AZStd::shared_mutex m_mutex;

        //! All these objects are necessary if you want a job that is cancellable.
        AZStd::unique_ptr<JobManager> m_jobManager;
        AZStd::unique_ptr<JobCancelGroup> m_jobCancelGroup;
        AZStd::unique_ptr<JobContext> m_jobContext;
        AZStd::unique_ptr<JobCompletion> m_jobUsedForCancellation;
        //! The cancelable job that patiently waits for all assets to be valid for loading.
        AZStd::unique_ptr<Job> m_assetDiscoveryJob;

        OnAssetReadyCallback m_onAssetReadyCB = nullptr;

        // The key is the assetPath
        AZStd::unordered_set<AZStd::string> m_assetsToLoad;

        //! As assets get ready or report error, we will place them here by assetPath and eventually call onAssetReadyCB
        //! on tick bus. After each tickbus this list is emptied.
        //! Key is the asset path, the value is true if ready, false if error.
        AZStd::unordered_map<AZStd::string, bool> m_assetsToNotify;

        //! In this case the key is Data::AssetId::ToString(), the value is the asset path
        AZStd::unordered_map<AZStd::string, AZStd::string> m_assetIdStrToAssetPath;

        //! The key is the asset path
        AZStd::unordered_map<AZStd::string, Data::Asset<Data::AssetData>> m_readyAssets;

        // Called by the m_assetDiscoveryJob as it discovers assets in the AP Cache.
        friend class AssetDiscoveryJob;
        void OnAssetIsValid(AZStd::string_view assetPath, const Data::AssetId& assetId, const Data::AssetType& assetType);

        void ResetLocked();
        void PostNotifyReadyAssetsCB(Data::Asset<Data::AssetData> asset, bool success);

        ///////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler overrides
        void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
        void OnAssetError(Data::Asset<Data::AssetData> asset) override;
    };
} //namespace AZ
