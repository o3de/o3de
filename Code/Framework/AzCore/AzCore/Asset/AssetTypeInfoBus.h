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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    /**
     * Bus for acquiring information about a given asset type, usually serviced by the relevant asset handler.
     * Extensions, load parameters, custom stream settings, etc.
     */
    class AssetTypeInfo
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Data::AssetType BusIdType;
        typedef AZStd::recursive_mutex MutexType;
        //////////////////////////////////////////////////////////////////////////

        //! this is the same type Id (uuid) as your AssetData-derived class's RTTI type.
        virtual AZ::Data::AssetType GetAssetType() const = 0;

        //! Retrieve the friendly name for the asset type.
        virtual const char* GetAssetTypeDisplayName() const { return "Unknown"; }

        //! This is the group or category that this kind of asset appears under for filtering and displaying in the browser.
        virtual const char* GetGroup() const { return "Other"; }

        //!  You can implement this to apply a specific icon to all assets of your type instead of using built in heuristics
        virtual const char* GetBrowserIcon() const { return ""; }

        //!  you can return the kind of component best suited to spawn on an entity if this kind of asset is dragged
        //!  to the viewport or to the component entity area.
        virtual AZ::Uuid GetComponentTypeId() const { return AZ::Uuid::CreateNull(); }

        //! Retrieve file extensions for the asset type.
        virtual void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) { (void)extensions; }

        //! Determines if a component can be created from the asset type
        //! This will be called before attempting to create a component from an asset (drag&drop, etc)
        //! You can use this to filter by subIds or do your own validation here if needed
        virtual bool CanCreateComponent([[maybe_unused]] const AZ::Data::AssetId& assetId) const { return true; }

        //! Determines if other products conflict with the given one when multiple are generated from a source asset.
        //! This will be called before attempting to create a component from an asset (drag&drop, etc)
        //! You can use this to filter by conflicting product types or in case you want to skip for UX reasons.
        //! @param[in] productAssetTypes Asset types of all generated products, including the one for our given type in this bus.
        virtual bool HasConflictingProducts([[maybe_unused]] const AZStd::vector<AZ::Data::AssetType>& productAssetTypes) const { return false; }
    };

    using AssetTypeInfoBus = AZ::EBus<AssetTypeInfo>;
} // namespace AZ

