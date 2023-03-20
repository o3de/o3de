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
#include <TressFX/TressFXAsset.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            //! HairAsset is a simple AssetData wrapper around the TressFXAsset used by the AP.
            //! Is comprises of the hair vertices data file, the hair bone skinning information file
            //! and the collision data file.
            //! The plan is to separate the collision data as it can have the relation of 1:1, 1:N or N:1
            //! meaning that the hair can have multiple collision handling (not only single mesh), and at
            //! the other end multiple hairs can have the same collision (hairdo and fur for example).
            class HairAsset final
                : public AZ::Data::AssetData
            {
            public:
                static constexpr inline const char* DisplayName = "HairAsset";
                static constexpr inline const char* Extension = "tfxhair";
                static constexpr inline const char* Group = "Hair";

                AZ_RTTI(HairAsset, "{52842B73-8F75-4620-8231-31EBCC74DD85}", AZ::Data::AssetData);
                AZ_CLASS_ALLOCATOR(HairAsset, AZ::SystemAllocator);

                AZStd::unique_ptr<AMD::TressFXAsset> m_tressFXAsset;
            };

            // HairAssetHandler
            // This handler class help to load the .tfxhair file (which is a combined file of .tfx, .tfxbone and .tfxmesh)
            // from an AssetDataStream.
            class HairAssetHandler final
                : public AzFramework::GenericAssetHandler<HairAsset>
            {
            public:
                HairAssetHandler();

            private:
                Data::AssetHandler::LoadResult LoadAssetData(
                    const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream,
                    const Data::AssetFilterCB& assetLoadFilterCB) override;
            };
        }
    }
}
