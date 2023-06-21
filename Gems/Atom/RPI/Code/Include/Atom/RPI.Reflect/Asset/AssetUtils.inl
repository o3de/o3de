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

                // Try to get an asset id for the requested path. Don't print an error yet if it isn't found though.
                Data::AssetId assetId = GetAssetIdForProductPath(path.c_str(), TraceLevel::None, azrtti_typeid<AssetDataT>());

                // If the asset id isn't valid for this path, it's possible that the asset hasn't been compiled yet.
                if (!assetId.IsValid())
                {
                    // Try compiling the asset and getting the id again.
                    AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
                    AzFramework::AssetSystemRequestBus::BroadcastResult(
                        status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, path);

                    // This time, print an error if the asset id can't be determined.
                    assetId = GetAssetIdForProductPath(path.c_str(), TraceLevel::Error, azrtti_typeid<AssetDataT>());
                }

                // We'll start the load whether or not the asset id is valid. It will immediately call the callback with failure if
                // the asset id is invalid.
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
