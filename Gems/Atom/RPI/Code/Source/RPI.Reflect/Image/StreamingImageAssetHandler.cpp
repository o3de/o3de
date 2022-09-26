/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Asset/AssetSystemBus.h>

namespace AZ
{
    namespace RPI
    {
        StreamingImageAssetReloader::StreamingImageAssetReloader(Data::AssetId assetId, CompleteCallback callback)
        {
            m_imageAssetId = assetId;
            m_completeCallback = callback;

            // Get current image asset
            m_imageAsset = AZ::Data::AssetManager::Instance().GetAsset<StreamingImageAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);

            // trigger reload for all its mip chains
            if (m_imageAsset->GetMipChainCount() > 0)
            {
                for (uint32_t mipChainIndex = 0; mipChainIndex < m_imageAsset->GetMipChainCount() - 1; mipChainIndex++)
                {
                    auto& mipChainAsset = m_imageAsset->m_mipChains[mipChainIndex].m_asset;
                    if (mipChainAsset.Get())
                    {
                        if (mipChainAsset.Reload())
                        {
                            m_pendingMipChainAssets.insert(mipChainAsset.GetId());
                            Data::AssetBus::MultiHandler::BusConnect(mipChainAsset.GetId());
                        }
                    }
                }
            }
            
            if (m_pendingMipChainAssets.size() > 0)
            {
                m_imageAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
            }
            else
            {
                m_imageAsset.Reload();
                Data::AssetBus::MultiHandler::BusConnect(m_imageAsset.GetId());
            }
        }
        
        StreamingImageAssetReloader::~StreamingImageAssetReloader()
        {
            if (!m_complete)
            {
                Data::AssetBus::MultiHandler::BusDisconnect(m_imageAssetId);
                for (uint32_t mipChainIndex = 0; mipChainIndex < m_imageAsset->GetMipChainCount() - 1; mipChainIndex++)
                {
                    auto& mipChainAsset = m_imageAsset->m_mipChains[mipChainIndex].m_asset;
                    Data::AssetBus::MultiHandler::BusDisconnect(mipChainAsset.GetId());
                }
            }
        }

        Data::AssetId StreamingImageAssetReloader::GetRemovalAssetId() const
        {
            return m_imageAssetId;
        }

        void StreamingImageAssetReloader::HandleMipChainAssetReload(Data::Asset<Data::AssetData> asset)
        {
            if (m_pendingMipChainAssets.size() > 0)
            {
                auto itr = m_pendingMipChainAssets.find(asset.GetId());
                if (itr != m_pendingMipChainAssets.end())
                {
                    m_reloadedMipChainAssets.push_back(asset);
                    m_pendingMipChainAssets.erase(itr);
                    Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
                    if (m_pendingMipChainAssets.size() == 0)
                    {
                        m_imageAsset.Reload();
                        Data::AssetBus::MultiHandler::BusConnect(m_imageAsset.GetId());
                    }
                }
            }
        }

        void StreamingImageAssetReloader::OnAssetReloadError(Data::Asset<Data::AssetData> asset)
        {
            // it's possible a mipchain asset was removed and the reload failed
            HandleMipChainAssetReload(asset);

            if (m_imageAssetId == asset.GetId())
            {
                Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
                m_complete = true;
                m_completeCallback(this);
            }
        }
        
        void StreamingImageAssetReloader::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            HandleMipChainAssetReload(asset);

            if (m_imageAssetId == asset.GetId())
            {
                m_imageAsset = asset;
                Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
                m_complete = true;
                m_completeCallback(this);
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
                loadResult = Utils::LoadObjectFromStreamInPlace<StreamingImageAsset>(*stream, *assetData, m_serializeContext) ?
                    Data::AssetHandler::LoadResult::LoadComplete :
                    Data::AssetHandler::LoadResult::Error;
                
                if (loadResult == Data::AssetHandler::LoadResult::LoadComplete)
                {
                    // ImageMipChainAsset has some internal variables need to initialized after it was loaded.
                    assetData->m_tailMipChain.Init();

                    // Find assets for all the mipchains
                    // This is to resolve reloading StreamingImageAsset won't load the asset reference even the StreamingImageAsset was loaded with LoadAll option
                    // If the asset was loaded. Check if it has full mipchain loaded.
                    auto& assetManager = AZ::Data::AssetManager::Instance();
                    Data::Asset<StreamingImageAsset> foundImageAsset = assetManager.FindAsset(asset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
                    if (foundImageAsset && foundImageAsset->HasFullMipChainAssets())
                    {
                        for (auto& mipChainAsset : assetData->m_mipChains)
                        {
                            if (mipChainAsset.m_asset.GetId().IsValid() && !mipChainAsset.m_asset.Get())
                            {
                                if (auto foundAsset = assetManager.FindAsset(mipChainAsset.m_asset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad))
                                {
                                    mipChainAsset.m_asset = foundAsset;
                                }
                                else
                                {
                                    // This may happen if the image's resolution increased which generate new mipchain assets 
                                    mipChainAsset.m_asset.QueueLoad();
                                    mipChainAsset.m_asset.BlockUntilLoadComplete();
                                }
                            }
                        }
                    }
                }
            }

            return loadResult;
        }

        Data::AssetId StreamingImageAssetHandler::AssetMissingInCatalog(const Data::Asset<Data::AssetData>& asset)
        {
            // Find out if the asset is missing completely, or just still processing
            // and escalate the asset to the top of the list
            AzFramework::AssetSystem::AssetStatus missingAssetStatus;
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
