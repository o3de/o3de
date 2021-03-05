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

#include <AzCore/std/string/string.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        //! This internal interface exists only to communicate to an Asset Catalog or manager of Asset Catalogs
        //! such as the PlatformAddressedAssetCatlogManager
        //! If you want to know when new assets are ready to load or have changed, listen to the AssetCatalogEventBus
        //! which will notify you when assets have been updated on the main thread only, and only after the catalog itself has
        //! updated with the new information.
        class NetworkAssetUpdateInterface
        {
        public:
            AZ_RTTI(NetworkAssetUpdateInterface, "{5041D165-41CF-4ED0-AD90-FBB7025AB2DC}");

            NetworkAssetUpdateInterface() = default;
            virtual ~NetworkAssetUpdateInterface() = default;

            NetworkAssetUpdateInterface(NetworkAssetUpdateInterface&&) = delete;
            NetworkAssetUpdateInterface& operator=(NetworkAssetUpdateInterface&&) = delete;

            //! Called by the AssetProcessor when an asset in the cache has been modified.
            virtual void AssetChanged(AzFramework::AssetSystem::AssetNotificationMessage /*message*/) = 0;
            //! Called by the AssetProcessor when an asset in the cache has been removed.
            virtual void AssetRemoved(AzFramework::AssetSystem::AssetNotificationMessage /*message*/) = 0;

            //! If we want to hear about assets for multiple platforms or something other than the asset platform defined at startup
            virtual AZStd::string GetSupportedPlatforms() { return {}; }
        };
    } // namespace AssetSystem
} // namespace AzFramework
