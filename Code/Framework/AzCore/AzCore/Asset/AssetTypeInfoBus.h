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

        static constexpr const int s_LowPriority = -10; //! use when you want your type to be the last resort
        static constexpr const int s_NormalPriority = 0; //! the default priority
        static constexpr const int s_HighPriority = 10;
        
        //! Used to assign a sort order to assets in the case where the user
        //! drags and drops a source file (like a FBX, but others too) which result in many
        //! different products of different types. Creating entities for each will cause a jumbled
        //! mess, so instead, the products will be sorted using this value as a hint
        //! and the first one in the resulting list will be picked to represent the drop operation.
        //! Highest number wins. In the case of ties, the list will also be sorted alphabetically
        //! and give a higher weight to assets with the same name as the source file that produced them.
        virtual int GetAssetTypeDragAndDropCreationPriority() const { return s_NormalPriority; }
    };

    using AssetTypeInfoBus = AZ::EBus<AssetTypeInfo>;
} // namespace AZ

DECLARE_EBUS_EXTERN_DLL_MULTI_ADDRESS(AssetTypeInfo);
