/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogBus.h>

AZ_INSTANTIATE_EBUS_MULTI_ADDRESS_WITH_TRAITS(AZTF_API, AZ::Data::AssetCatalogRequests, AzToolsFramework::AssetCatalog::PlatformAddressedAssetCatalogBusTraits)
