/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/Platform/PlatformDefaults.h>

namespace AzToolsFramework
{
    namespace AssetCatalog
    {
        //! Acts as a variant of the AssetCatalogRequestsBus which handles requests based on platformID
        //! Used by PlatformAddressedAssetCatalog which loads up AssetCatalogs for enable platforms and allows
        //! tools to interact with them individually
        class PlatformAddressedAssetCatalogBusTraits
            : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            using MutexType = AZStd::recursive_mutex;

            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = int; // Type is AzFramework::PlatformId - Cast to int from enum not supported by vs2015 compiler
        };

        using PlatformAddressedAssetCatalogRequestBus = AZ::EBus<AZ::Data::AssetCatalogRequests, PlatformAddressedAssetCatalogBusTraits>;
    }
} // namespace AzToolsFramework

