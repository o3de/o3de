/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

namespace AZ
{
    namespace RPI
    {
        namespace AssetUtils
        {
            // AsyncAssetLoader //

            template <typename AssetDataT>
            AsyncAssetLoader AsyncAssetLoader::Create(const AZStd::string& path, [[maybe_unused]] uint32_t subId, AssetCallback callback)
            {
                AsyncAssetLoader loader(callback);

                Data::AssetId assetId = GetAssetIdForProductPath(path.c_str(), TraceLevel::Error, azrtti_typeid<AssetDataT>(), true);
                loader.StartLoad<AssetDataT>(assetId);
                return loader;
            }

            template <typename AssetDataT>
            AsyncAssetLoader AsyncAssetLoader::Create(Data::AssetId assetId, AssetCallback callback)
            {
                AsyncAssetLoader loader(callback);
                loader.StartLoad<AssetDataT>(assetId);
                return loader;
            }

            template <typename AssetDataT>
            void AsyncAssetLoader::StartLoad(Data::AssetId& assetId)
            {
                if (!assetId.IsValid())
                {
                    // Immediately call callback with empty asset and false for success.
                    m_callback(Data::Asset<AssetDataT>(), false);
                }
                m_asset = AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                m_asset.QueueLoad();
                Data::AssetBus::MultiHandler::BusConnect(assetId);
            }
        }
    }
}
