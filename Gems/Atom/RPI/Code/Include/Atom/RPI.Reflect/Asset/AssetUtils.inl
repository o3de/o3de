/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
