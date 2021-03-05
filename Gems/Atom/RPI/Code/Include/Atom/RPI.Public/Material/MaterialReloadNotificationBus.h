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
