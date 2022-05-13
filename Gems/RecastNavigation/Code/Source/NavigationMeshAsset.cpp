/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <NavigationMeshAsset.h>

#pragma optimize("", off)

namespace RecastNavigation
{
    NavigationMeshAssetHandler::NavigationMeshAssetHandler()
        : AzFramework::GenericAssetHandler<NavigationMeshAsset>(NavigationMeshAsset::DisplayName, NavigationMeshAsset::Group, NavigationMeshAsset::Extension)
    {
    }

    AZ::Data::AssetHandler::LoadResult NavigationMeshAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        NavigationMeshAsset* assetData = asset.GetAs<NavigationMeshAsset>();
        if(assetData && stream->GetLength() > 0)
        {
            assetData->m_data.resize(stream->GetLength());
            stream->Read(stream->GetLength(), assetData->m_data.data());

            // use a memory stream instead of AssetDataStream to validate the data,
            // AssetDataStream does not allow reverse seek
            AZ::IO::MemoryStream memStream(assetData->m_data.data(), assetData->m_data.size());

            constexpr uint32_t starsfileTag(0x52415453); // "STAR"
            uint32_t fileTag{0};
            if (!memStream.Read(sizeof(fileTag), &fileTag) || fileTag != starsfileTag)
            {
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            constexpr uint32_t starsfileVersion(0x00010001);
            uint32_t fileVersion{0};
            if (!memStream.Read(sizeof(fileVersion), &fileVersion) || fileVersion != starsfileVersion)
            {
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            uint32_t numStars{0};
            if(!memStream.Read(sizeof(numStars), &numStars))
            {
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    bool NavigationMeshAssetHandler::SaveAssetData(
        [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset, [[maybe_unused]] AZ::IO::GenericStream* stream)
    {
        return true;
    }
} // AZ::Render

#pragma optimize("", on)
