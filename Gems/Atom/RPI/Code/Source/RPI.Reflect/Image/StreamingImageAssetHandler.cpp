/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
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
            // Escalate the asset to the top of the list
            AzFramework::AssetSystemRequestBus::Broadcast(
                &AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetByUuid, asset.GetId().m_guid);

            // Make sure the default fallback image has been processed so that it is part of the asset catalog
            AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, "textures/defaults/defaultnouvs.tif.streamingimage");

            // Get the id of the fallback image
            Data::AssetInfo assetInfo;
            assetInfo.m_relativePath = "textures/defaults/defaultnouvs.tif.streamingimage";
            Data::AssetId assetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &Data::AssetCatalogRequestBus::Events::GenerateAssetIdTEMP,
                assetInfo.m_relativePath.c_str());

            assetInfo.m_assetId = Data::AssetId(assetId.m_guid, StreamingImageAsset::GetImageAssetSubId());

            // Register the fallback image with the asset catalog
            Data::AssetCatalogRequestBus::Broadcast(
                &Data::AssetCatalogRequestBus::Events::RegisterAsset, assetInfo.m_assetId, assetInfo);

            // Use a default Image as a fallback
            return assetInfo.m_assetId;
        }
    }
}
