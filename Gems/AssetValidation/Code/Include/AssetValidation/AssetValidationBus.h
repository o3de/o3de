/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>

namespace AssetValidation
{
    class AssetValidationRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        using AssetSourceList = AZStd::vector<AZStd::pair< AZ::Data::AssetId, AZ::u32 >>;
        //! Request to toggle Seed Mode
        virtual void SeedMode() { }
        //! Given an asset fileName does it exist in our dependency graph beneath any of our declared seed assetIds
        virtual bool IsKnownAsset(const char* /*fileName*/) { return false; }
        //! When Seed Mode is on, given an asset fileName does it exist in our dependency graph - if so emit missing seed notification
        virtual bool CheckKnownAsset(const char* /*fileName*/) { return false; }
        //! Add a new seed assetId to the graph.  sourceID is used for deterministic add/removal - the same seed assetId
        //! Could come from multiple sources so we need to be able to track multiple copies to avoid cleanup errors
        virtual bool AddSeedAssetId(AZ::Data::AssetId /*assetId*/, AZ::u32 /*sourceId*/) { return false; }
        // Remove a seed assetId to the graph.  sourceID is used for deterministic removal - the same seed assetId
        //! Could come from multiple sources so we need to be able to track multiple copies to avoid cleanup errors
        virtual bool RemoveSeedAssetId(AZ::Data::AssetId /* assetId */, AZ::u32 /*sourceId */) { return false; }
        //! Request to add a seed file to be tracked by file path
        virtual bool RemoveSeedAssetIdList(AssetSourceList /* assetList */) { return false; }

        //! Request to add a seed to tracking by a relative path under the /dev folder
        virtual bool AddSeedPath([[maybe_unused]] const char* fileName) { return false; }
        //! Request to Remove a Seed from tracking by relative path under the /dev folder
        virtual bool RemoveSeedPath([[maybe_unused]] const char* fileName) { return false; }
        //! Check current status of seed mode
        virtual bool IsSeedMode() { return false; }
        //! List all known current assetIds and paths
        virtual void ListKnownAssets() { }
        //! Add all seeds from a seed file at a specified path
        virtual bool AddSeedList([[maybe_unused]] const char* filePath) { return true; }
        //! Remove all seeds from a seed file at a specified path
        virtual bool RemoveSeedList([[maybe_unused]] const char* filePath) { return true; }
        //! Toggle exclude printing
        virtual void TogglePrintExcluded() { }
    };
    using AssetValidationRequestBus = AZ::EBus<AssetValidationRequests>;

    class AssetValidationNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Notification that SeedMode has been turned on or off
        virtual void SetSeedMode(bool /*modeOn*/) {}
        // Notification that an unknown asset has been discovered through CheckKnownAsset
        virtual void UnknownAsset([[maybe_unused]] const char* fileName) { }
    };
    using AssetValidationNotificationBus = AZ::EBus<AssetValidationNotifications>;
} // namespace AssetValidation
