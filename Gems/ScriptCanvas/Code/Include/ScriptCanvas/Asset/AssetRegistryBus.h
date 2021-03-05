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
