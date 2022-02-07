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
                    StreamingImageAsset* assetData2 = asset.GetAs<StreamingImageAsset>();
                    assetData2->m_tailMipChain.Init();
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
