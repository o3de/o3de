/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DetourNavMesh.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace RecastNavigation
{
    struct NavMeshSetHeader
    {
        int magic;
        int version;
        int numTiles;
        dtNavMeshParams params;
    };

    struct NavMeshTileHeader
    {
        dtTileRef tileRef;
        int dataSize;
    };

    class NavigationMeshAsset final
        : public AZ::Data::AssetData
    {
    public:
        static constexpr inline const char* DisplayName = "NavigationMeshAsset";
        static constexpr inline const char* Extension = "navmesh";
        static constexpr inline const char* Group = "Navigation";

        AZ_RTTI(NavigationMeshAsset, "{5275fa85-c737-4b7b-bee4-375521698f1e}", AZ::Data::AssetData)
        AZ_CLASS_ALLOCATOR(NavigationMeshAsset, AZ::SystemAllocator, 0);

        const dtNavMesh* m_navMesh = nullptr;

        NavMeshSetHeader m_header;
        AZStd::vector<NavMeshTileHeader> m_tileHeaders;
        AZStd::unique_ptr<AZStd::vector<AZStd::vector<AZ::u8>>> m_tileData;
    };

    class NavigationMeshAssetHandler final
        : public AzFramework::GenericAssetHandler<NavigationMeshAsset>
    {
    public:
        NavigationMeshAssetHandler();

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;

        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;

        static bool SaveToStream(const dtNavMesh* mesh, AZ::IO::GenericStream* stream);
    };
} // AZ::Render
