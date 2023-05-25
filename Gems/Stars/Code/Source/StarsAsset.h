/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace AZ::Render
{
    class StarsAsset final
        : public AZ::Data::AssetData
    {
    public:
        static constexpr inline const char* DisplayName = "StarsAsset";
        static constexpr inline const char* Extension = "stars";
        static constexpr inline const char* Group = "Rendering";
        static constexpr inline const size_t HeaderSize = sizeof(uint32_t) * 3;

        AZ_RTTI(StarsAsset, "{211938C1-C54D-417D-977A-9928D7C630B1}", AZ::Data::AssetData)
        AZ_CLASS_ALLOCATOR(StarsAsset, AZ::SystemAllocator);

        AZStd::vector<AZ::u8> m_data;
    };

    class StarsAssetHandler final
        : public AzFramework::GenericAssetHandler<StarsAsset>
    {
    public:
        StarsAssetHandler();

    private:
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
    };
} // AZ::Render
