/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace RPI
    {
        class Material;
        class MaterialAsset;
        class MaterialTypeAsset;

        //! Connect to this EBus to get notifications whenever material objects reload.
        //! The bus address is the AssetId of the MaterialAsset or MaterialTypeAsset.
        //!
        //! Be careful when using the parameters provided by these functions. The bus ID is an AssetId, and it's possible for the system to have
        //! both *old* versions and *new reloaded* versions of the asset in memory at the same time, and they will have the same AssetId. Therefore
        //! your bus Handlers could receive Reinitialized messages from multiple sources. It may be necessary to check the memory addresses of these
        //! parameters against local members before using this data.
        class MaterialReloadNotifications
            : public EBusTraits
        {

        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef Data::AssetId BusIdType;
            typedef AZStd::recursive_mutex MutexType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~MaterialReloadNotifications() {}

            //! Called when a Material reinitializes itself in response to an asset being reloaded or reinitialized.
            virtual void OnMaterialReinitialized(const Data::Instance<Material>& material) { AZ_UNUSED(material); }

            //! Called when a MaterialAsset reinitializes itself in response to another asset being reloaded or reinitialized.
            virtual void OnMaterialAssetReinitialized(const Data::Asset<MaterialAsset>& materialAsset) { AZ_UNUSED(materialAsset); }

            //! Called when a MaterialTypeAsset reinitializes itself in response to another asset being reloaded or reinitialized.
            virtual void OnMaterialTypeAssetReinitialized(const Data::Asset<MaterialTypeAsset>& materialTypeAsset) { AZ_UNUSED(materialTypeAsset); }
        };

        typedef EBus<MaterialReloadNotifications> MaterialReloadNotificationBus;

    } // namespace RPI
} //namespace AZ
