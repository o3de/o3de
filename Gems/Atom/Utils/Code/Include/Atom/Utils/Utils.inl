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

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace Render
    {
        template <typename T>
        AZ::Outcome<void> WriteToBuffer(RHI::Ptr<RHI::Buffer> buffer, const AZStd::vector<T>& data)
        {
            const size_t byteCount = data.size() * sizeof(T);
            return WriteToBuffer(buffer, data.data(), byteCount);
        }

        template <typename AssetType>
        Data::Asset<AssetType> GetAssetFromPath(const AZStd::string_view path, AZ::Data::AssetLoadBehavior loadBehavior, bool loadBlocking)
        {
            Data::AssetId assetId;
            Data::AssetCatalogRequestBus::BroadcastResult(assetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, path.data(), AssetType::RTTI_Type(), false);
            Data::Asset<AssetType> asset = GetAssetFromId<AssetType>(assetId, loadBehavior, loadBlocking);

            if (!asset && path.size() > 0)
            {
                AZ_Warning("CreateAssetFromPath", false, "Unable to find asset Id for path %.*s.", aznumeric_cast<int>(path.size()), path.data());
            }
            return asset;
        }

        template <typename AssetType>
        Data::Asset<AssetType> GetAssetFromId(Data::AssetId assetId, AZ::Data::AssetLoadBehavior loadBehavior, bool loadBlocking)
        {
            Data::Asset<AssetType> asset;
            if (assetId.IsValid())
            {
                asset = AZ::Data::AssetManager::Instance().GetAsset<AssetType>(assetId, loadBehavior);

                if (loadBlocking)
                {
                    asset.BlockUntilLoadComplete();
                }
            }
            return asset;
        }
    }
}
