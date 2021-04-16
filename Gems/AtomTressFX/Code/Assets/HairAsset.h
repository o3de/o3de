/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <TressFX/TressFXAsset.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            // HairAsset
            // HairAsset is a simple AssetData wrapper of the tressFXAsset
            class HairAsset final
                : public AZ::Data::AssetData
            {
            public:
                static constexpr inline const char* DisplayName = "HairAsset";
                static constexpr inline const char* Extension = "tfxhair";
                static constexpr inline const char* Group = "Hair";

                AZ_RTTI(HairAsset, "{52842B73-8F75-4620-8231-31EBCC74DD85}", AZ::Data::AssetData);
                AZ_CLASS_ALLOCATOR(HairAsset, AZ::SystemAllocator, 0);

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
