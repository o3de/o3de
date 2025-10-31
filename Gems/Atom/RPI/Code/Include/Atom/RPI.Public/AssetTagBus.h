/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RPI.Public/AssetQuality.h>

namespace AZ
{
    namespace RPI
    {
        template<typename T>
        class AssetTagInterface
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;

            //! Returns the quality associated with an asset tag
            virtual AssetQuality GetQuality(const AZ::Name& assetTag) const = 0;

            //! Returns all registered tags
            virtual AZStd::vector<AZ::Name> GetTags() const = 0;

            //! Registers an asset as using a tag, which allows the tag system to reload the asset if required when the tag is updated
            //! Can be called multiple times with the same assetId
            virtual void RegisterAsset(AZ::Name assetTag, const Data::AssetId& assetId) = 0;

            //! Registers a tag to the system, must be called before using that tag with other functions
            virtual void RegisterTag(AZ::Name tag) = 0;

            //! Updates the quality associated with a tag, which may trigger asset reload for registered assets
            virtual void SetQuality(const AZ::Name& assetTag, AssetQuality quality) = 0;
        };

        template<typename T>
        class AssetTagNotification
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::Name;
            using MutexType = AZStd::recursive_mutex;
            //////////////////////////////////////////////////////////////////////////

            virtual void OnAssetTagQualityUpdated(AssetQuality quality) = 0;
        };

        class ImageAsset;
        class ModelAsset;

        using ImageTagBus = EBus<AssetTagInterface<ImageAsset>>;
        using ImageTagNotificationBus = EBus<AssetTagNotification<ImageAsset>>;

        using ModelTagBus = EBus<AssetTagInterface<ModelAsset>>;
        using ModelTagNotificationBus = EBus<AssetTagNotification<ModelAsset>>;
    } // namespace RPI
}
