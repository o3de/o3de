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
