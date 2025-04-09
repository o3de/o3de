/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Asset/AssetManager.h>

namespace ScriptCanvas
{
    class AssetDescription;

    class AssetRegistryRequests : public AZ::EBusTraits
    {
    public:

        using BusIdType = AZ::Data::AssetType;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual AZ::Data::AssetHandler* GetAssetHandler() = 0;
        virtual AssetDescription* GetAssetDescription(AZ::Data::AssetType assetType) = 0;
        virtual AZStd::vector<AZStd::string> GetAssetHandlerFileFilters() = 0;

    };

    using AssetRegistryRequestBus = AZ::EBus<AssetRegistryRequests>;
}

DECLARE_EBUS_EXTERN(ScriptCanvas::AssetRegistryRequests);
