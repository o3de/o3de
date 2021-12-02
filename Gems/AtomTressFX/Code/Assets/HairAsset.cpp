/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Assets/HairAsset.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            HairAssetHandler::HairAssetHandler()
                : AzFramework::GenericAssetHandler<HairAsset>(HairAsset::DisplayName, HairAsset::Group, HairAsset::Extension)
            {
            }

            Data::AssetHandler::LoadResult HairAssetHandler::LoadAssetData(
                const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream,
                [[maybe_unused]]const Data::AssetFilterCB& assetLoadFilterCB)
            {
                HairAsset* assetData = asset.GetAs<HairAsset>();
                assetData->m_tressFXAsset.reset(new AMD::TressFXAsset());
                if(assetData->m_tressFXAsset->LoadCombinedHairData(stream.get()))
                {
                    return Data::AssetHandler::LoadResult::LoadComplete;
                }
                else
                {
                    assetData->m_tressFXAsset.release();
                }
                return Data::AssetHandler::LoadResult::Error;
            }
        }
    } // namespace Render
} // namespace AZ
