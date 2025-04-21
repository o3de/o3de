/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/AssetTagBus.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Asset/AssetSystemBus.h>

namespace AZ
{
    namespace RPI
    {

        StreamingImageAssetHandler::~StreamingImageAssetHandler()
        {
            for (const auto& pendingAsset: m_pendingReloadImageAsset)
            {
                HandleMipChainAssetBuses(pendingAsset.second.m_imageAsset, false);
            }
        }

        Data::AssetHandler::LoadResult StreamingImageAssetHandler::LoadAssetData(
            const Data::Asset<Data::AssetData>& asset,
            AZStd::shared_ptr<Data::AssetDataStream> stream,
            const Data::AssetFilterCB& assetLoadFilterCB)
        {
            AZ_UNUSED(assetLoadFilterCB);

            StreamingImageAsset* assetData = asset.GetAs<StreamingImageAsset>();
            AZ_Assert(assetData, "Asset is of the wrong type.");
            AZ_Assert(m_serializeContext, "Unable to retrieve serialize context.");

            Data::AssetHandler::LoadResult loadResult = Data::AssetHandler::LoadResult::Error;
            if (assetData)
            {
                if (stream->GetLength() > 0)
                {
                    loadResult = Utils::LoadObjectFromStreamInPlace<StreamingImageAsset>(*stream, *assetData, m_serializeContext)
                        ? Data::AssetHandler::LoadResult::LoadComplete
                        : Data::AssetHandler::LoadResult::Error;
                }
               
                if (loadResult == Data::AssetHandler::LoadResult::LoadComplete)
                {
                    // ImageMipChainAsset has some internal variables need to initialized after it was loaded.
                    assetData->m_tailMipChain.Init();

                    if (const auto& imageTags = assetData->GetTags(); !imageTags.empty())
                    {
                        AssetQuality highestMiplevel = AssetQualityLowest;
                        for (const AZ::Name& tag : imageTags)
                        {
                            AssetQuality tagQuality = AssetQualityHighest;
                            ImageTagBus::BroadcastResult(tagQuality, &ImageTagBus::Events::GetQuality, tag);

                            highestMiplevel = AZStd::min(highestMiplevel, tagQuality);
                        }

                        assetData->RemoveFrontMipchains(highestMiplevel);

                        for (const AZ::Name& tag : imageTags)
                        {
                            ImageTagBus::Broadcast(&ImageTagBus::Events::RegisterAsset, tag, assetData->GetId());
                        }
                    }

                    // Handle StreamingImageAsset reload (which the asset can be found from asset manager)
                    auto& assetManager = AZ::Data::AssetManager::Instance();
                    Data::Asset<StreamingImageAsset> foundImageAsset = assetManager.FindAsset(asset.GetId(), AZ::Data::AssetLoadBehavior::Default);
                    // If the asset has full mipchain assets, then we want to reload all of them when reload StreamingImageAsset
                    if (foundImageAsset && foundImageAsset.GetData() != asset.GetData() && foundImageAsset->HasFullMipChainAssets())
                    {
                        // Save the reloaded StreamingImageAsset data
                        Uuid imageGuid = asset.GetId().m_guid;
                        PendingImageAssetInfo pendingAssetInfo;
                        pendingAssetInfo.m_imageAsset = asset;
                        for (auto& mipChainAsset : assetData->m_mipChains)
                        {
                            Data::AssetId mipChainAssetId = mipChainAsset.m_asset.GetId();
                            if (mipChainAssetId.IsValid())
                            {
                                // Release the mipchain asset since it could reference the old asset.
                                if (auto foundAsset = assetManager.FindAsset(mipChainAssetId, AZ::Data::AssetLoadBehavior::PreLoad))
                                {
                                    // if the asset was loaded, trigger reload
                                    foundAsset.Reload();
                                    mipChainAsset.m_asset = foundAsset;
                                }
                                else
                                {
                                    // The asset wasn't loaded, queue load
                                    // This may happen if the image's resolution increased which generate new mipchain assets 
                                    mipChainAsset.m_asset.QueueLoad();
                                }
                                pendingAssetInfo.m_mipChainAssetSubIds.push_back(mipChainAssetId.m_subId);
                                Data::AssetBus::MultiHandler::BusConnect(mipChainAssetId);
                            }
                        }

                        if (!pendingAssetInfo.m_mipChainAssetSubIds.empty())
                        {
                            AZStd::scoped_lock<AZStd::mutex> lock(m_accessPendingAssetsMutex);
                            m_pendingReloadImageAsset[imageGuid] = AZStd::move(pendingAssetInfo);
                        }
                    }
                }
            }

            return loadResult;
        }

        void StreamingImageAssetHandler::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            HandleMipChainAssetLoad(asset, true);
        }

        void StreamingImageAssetHandler::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            HandleMipChainAssetLoad(asset, false);
        }
        
        void StreamingImageAssetHandler::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            HandleMipChainAssetLoad(asset, true);
        }

        void StreamingImageAssetHandler::OnAssetReloadError(Data::Asset<Data::AssetData> asset)
        {
            HandleMipChainAssetLoad(asset, false);
        }
        
        void StreamingImageAssetHandler::HandleMipChainAssetBuses(Data::Asset<Data::AssetData> streamingImageAsset, bool connect)
        {
            StreamingImageAsset* assetData = streamingImageAsset.GetAs<StreamingImageAsset>();
            for (auto& mipChainAsset : assetData->m_mipChains)
            {
                // Need to skip tail mipchain which is in the list and doesn't have an asset id.
                if (mipChainAsset.m_asset.GetId().IsValid())
                {
                    if (connect)
                    {
                        Data::AssetBus::MultiHandler::BusConnect(mipChainAsset.m_asset.GetId());
                    }
                    else
                    {
                        Data::AssetBus::MultiHandler::BusDisconnect(mipChainAsset.m_asset.GetId());
                    }
                }
            }
        }

        void StreamingImageAssetHandler::HandleMipChainAssetLoad(Data::Asset<Data::AssetData> asset, bool isLoadSuccess)
        {
            bool hasError = false;
            bool reloadEnded = false;
            Data::Asset<Data::AssetData> imageAsset;

            {
                // Note: lock the mutex at minimum scope and avoid calling broadcast or bus connect/disconnect when mutex is locked.
                AZStd::scoped_lock<AZStd::mutex> lock(m_accessPendingAssetsMutex);
                auto itr = m_pendingReloadImageAsset.find(asset.GetId().m_guid);
                if (itr == m_pendingReloadImageAsset.end())
                {
                    return;
                }

                if (!isLoadSuccess)
                {
                    reloadEnded = true;
                    hasError = true;
                }

                uint32_t loadedMipChainAssets = 0;
                for (u32& subId : itr->second.m_mipChainAssetSubIds)
                {
                    if (subId == asset.GetId().m_subId)
                    {
                        StreamingImageAsset* assetData = itr->second.m_imageAsset.GetAs<StreamingImageAsset>();

                        // Assign the reloaded/loaded asset to StreamingImageAsset
                        for (auto& mipChainAsset : assetData->m_mipChains)
                        {
                            if (mipChainAsset.m_asset.GetId() == asset.GetId())
                            {
                                mipChainAsset.m_asset = asset;
                                // set the subId to 0 after it's loaded
                                subId = 0;
                                break;
                            }
                        }

                    }
                    if (subId == 0)
                    {
                        loadedMipChainAssets++;
                    }
                }

                if (loadedMipChainAssets == itr->second.m_mipChainAssetSubIds.size())
                {
                    reloadEnded = true;
                    hasError = false;
                }

                if (reloadEnded)
                {
                    imageAsset = itr->second.m_imageAsset;
                    m_pendingReloadImageAsset.erase(itr);
                }
            }

            if (reloadEnded)
            {
                HandleMipChainAssetBuses(imageAsset, false);
                if (hasError)
                {
                    Data::AssetManagerBus::Broadcast(&Data::AssetManagerBus::Events::OnAssetReloadError, imageAsset);
                }
                else
                {
                    Data::AssetManagerBus::Broadcast(&Data::AssetManagerBus::Events::OnAssetReloaded, imageAsset);
                }
            }
        }

        void StreamingImageAssetHandler::InitAsset(const Data::Asset<Data::AssetData>& asset, bool loadStageSucceeded, bool isReload)
        {
            m_accessPendingAssetsMutex.lock();
            bool isPendingReload = m_pendingReloadImageAsset.find(asset.GetId().m_guid) != m_pendingReloadImageAsset.end();
            m_accessPendingAssetsMutex.unlock();

            // Broadcast the asset loading events via AssetHandler::InitAsset
            // If the asset was reloaded successfully and also have pending mipchain assets to load, skip here and delay the broadcast the reloaded event.
            if (!(loadStageSucceeded && isReload && isPendingReload))
            {
                AZ_Assert(!isPendingReload, "The asset shouldn't be added to pending reload asset list");
                AssetHandler::InitAsset(asset, loadStageSucceeded, isReload);
            }
        }

        Data::AssetId StreamingImageAssetHandler::AssetMissingInCatalog(const Data::Asset<Data::AssetData>& asset)
        {
            AZ_Info("Streaming Image",
                "Streaming Image id " AZ_STRING_FORMAT " not found in asset catalog, using fallback image.\n",
                AZ_STRING_ARG(asset.GetId().ToFixedString()));

            // Find out if the asset is missing completely, or just still processing
            // and escalate the asset to the top of the list
            AzFramework::AssetSystem::AssetStatus missingAssetStatus = AzFramework::AssetSystem::AssetStatus::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                missingAssetStatus, &AzFramework::AssetSystem::AssetSystemRequests::GetAssetStatusById, asset.GetId().m_guid);

            // Determine which fallback image to use
            const char* relativePath = DefaultImageAssetPaths::DefaultFallback;

            bool useDebugFallbackImages = true;
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->GetObject(useDebugFallbackImages, "/O3DE/Atom/RPI/UseDebugFallbackImages");
            }

            if (useDebugFallbackImages)
            {
                switch (missingAssetStatus)
                {
                case AzFramework::AssetSystem::AssetStatus::AssetStatus_Queued:
                case AzFramework::AssetSystem::AssetStatus::AssetStatus_Compiling:
                    relativePath = DefaultImageAssetPaths::Processing;
                    break;
                case AzFramework::AssetSystem::AssetStatus::AssetStatus_Failed:
                    relativePath = DefaultImageAssetPaths::ProcessingFailed;
                    break;
                case AzFramework::AssetSystem::AssetStatus::AssetStatus_Missing:
                case AzFramework::AssetSystem::AssetStatus::AssetStatus_Unknown:
                case AzFramework::AssetSystem::AssetStatus::AssetStatus_Compiled:
                    relativePath = DefaultImageAssetPaths::Missing;
                    break;
                }
            }

            // Make sure the fallback image has been processed
            AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, relativePath);

            // Return the asset id of the fallback image
            Data::AssetId assetId{};
            bool autoRegisterIfNotFound = false;
            Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, relativePath,
                azrtti_typeid<AZ::RPI::StreamingImageAsset>(), autoRegisterIfNotFound);

            return assetId;
        }
    } // namespace RPI
} // namespace AZ
