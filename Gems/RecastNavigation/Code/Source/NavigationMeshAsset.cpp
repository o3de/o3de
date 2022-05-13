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
    static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
    static const int NAVMESHSET_VERSION = 1;

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
        if (assetData && stream->GetLength() > 0)
        {
            assetData->m_header = {};
            assetData->m_tileHeaders.clear();
            assetData->m_tileData = AZStd::make_unique<AZStd::vector<AZStd::vector<AZ::u8>>>();

            // Read the header
            AZ::IO::SizeType bytesRead = stream->Read(sizeof(NavMeshSetHeader), &assetData->m_header);
            if (bytesRead != sizeof(NavMeshSetHeader))
            {
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            // Read tiles.
            bool reading = true;
            while (reading)
            {
                NavMeshTileHeader tileHeader = {};
                bytesRead = stream->Read(sizeof(NavMeshTileHeader), &tileHeader);
                if (bytesRead == sizeof(NavMeshTileHeader))
                {
                    AZStd::vector<AZ::u8> tileData;
                    tileData.resize(tileHeader.dataSize);
                    bytesRead = stream->Read(tileHeader.dataSize, tileData.data());
                    if (bytesRead == tileHeader.dataSize)
                    {
                        // successful read
                        assetData->m_tileHeaders.push_back(tileHeader);
                        assetData->m_tileData->push_back(tileData);
                    }
                    else
                    {
                        reading = false;
                    }
                }
                else
                {
                    reading = false;
                }
            }

            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    bool NavigationMeshAssetHandler::SaveAssetData(
        [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset, [[maybe_unused]] AZ::IO::GenericStream* stream)
    {
        NavigationMeshAsset* navAsset = asset.GetAs<NavigationMeshAsset>();
        AZ_Assert(navAsset, "This should be a navigation mesh asset, as this is the only type we process!");
        if (navAsset && navAsset->m_navMesh)
        {
            const dtNavMesh* mesh = navAsset->m_navMesh;
            return SaveToStream(mesh, stream);
        }

        return false;
    }

    bool NavigationMeshAssetHandler::SaveToStream(const dtNavMesh* mesh, AZ::IO::GenericStream* stream)
    {
        // Store header.
        NavMeshSetHeader header = {};
        header.magic = NAVMESHSET_MAGIC;
        header.version = NAVMESHSET_VERSION;
        header.numTiles = 0;
        for (int i = 0; i < mesh->getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = mesh->getTile(i);
            if (!tile || !tile->header || !tile->dataSize) continue;
            header.numTiles++;
        }
        memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));

        stream->Write(sizeof(NavMeshSetHeader), &header);

        // Store tiles.
        for (int i = 0; i < mesh->getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = mesh->getTile(i);
            if (!tile || !tile->header || !tile->dataSize) continue;

            NavMeshTileHeader tileHeader;
            tileHeader.tileRef = mesh->getTileRef(tile);
            tileHeader.dataSize = tile->dataSize;

            stream->Write(sizeof(tileHeader), &tileHeader);

            stream->Write(tile->dataSize, tile->data);
        }

        return true;
    }
} // AZ::Render

#pragma optimize("", on)
