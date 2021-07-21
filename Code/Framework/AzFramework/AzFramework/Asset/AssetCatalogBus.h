/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZFRAMEWORK_ASSETCATALOGBUS_H
#define AZFRAMEWORK_ASSETCATALOGBUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ::Data
{
    class AssetInfo;
}

namespace AzFramework
{
    /**
     * Event bus for asset catalogs.
     */
    class AssetCatalogEvents
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        /// Game/runtime notification for when catalog is loaded and it's possible to resolve asset Ids.
        virtual void OnCatalogLoaded(const char* /*catalogFile*/) {}

        /// Notifies listeners that an existing asset has changed on disk (reload has not yet occurred).
        virtual void OnCatalogAssetChanged(const AZ::Data::AssetId& /*assetId*/) {}

        /// Notifies listeners that a new asset has been discovered.
        virtual void OnCatalogAssetAdded(const AZ::Data::AssetId& /*assetId*/) {}

        /// Notifies listeners that an asset has been removed.  This event occurs after the asset has been removed from the catalog.
        /// assetInfo contains the catalog info from before the asset was removed.
        virtual void OnCatalogAssetRemoved(const AZ::Data::AssetId& /*assetId*/, const AZ::Data::AssetInfo& /*assetInfo*/) {}
    };

    using AssetCatalogEventBus = AZ::EBus<AssetCatalogEvents>;

    /**
     * Event bus for legacy file-based asset changes. New code should use the AssetCatalogEventBus.
     */
    class LegacyAssetEvents
        : public AZ::EBusTraits
    {
    public:
        ///////////////////////////////////////////////////////////////////////
        static const bool EnableEventQueue = true; // enabled queued events, asset msgs come from any thread
        using EventQueueMutexType = AZStd::mutex;

        using BusIdType = AZ::u32; // bus is addressed by CRC of extension
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        ///////////////////////////////////////////////////////////////////////

        /// Notifies listeners that a file changed
        virtual void OnFileChanged(AZStd::string /*assetPath*/) {}
        virtual void OnFileRemoved(AZStd::string /*assetPath*/) {}
    };

    using LegacyAssetEventBus = AZ::EBus<LegacyAssetEvents>;
} // namespace AzFramework

#pragma once

#endif // AZFRAMEWORK_ASSETCATALOGBUS_H
