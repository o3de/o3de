/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace Data
    {
        // Private system events - external systems should not listen for these
        class AssetLoadEvents
            : public EBusTraits
        {
        public:
            AZ_RTTI(AssetLoadEvents, "{7F8128CD-3951-46C0-A9CA-E6F1F6A5B6FB}");

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using MutexType = AZStd::recursive_mutex;
            using BusIdType = AssetId;

            virtual ~AssetLoadEvents() {}

            /// Called when an asset's data is loaded into memory for assets which have dependencies
            /// which have been set to load first (Preload dependencies) 
            virtual void OnAssetDataLoaded([[maybe_unused]] Asset<AssetData> rootAsset) {}
        };
        using AssetLoadBus = EBus<AssetLoadEvents>;

    }  // namespace Data
}   // namespace AZ



