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



