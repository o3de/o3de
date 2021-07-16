/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AssetBuilderSDK
{
    struct AssetBuilderDesc;
}

namespace PythonAssetBuilder
{
    class PythonAssetBuilderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! registers an asset builder using a builder descriptor
        virtual AZ::Outcome<bool, AZStd::string> RegisterAssetBuilder(const AssetBuilderSDK::AssetBuilderDesc& desc) = 0;

        //! fetches the current executable folder
        virtual AZ::Outcome<AZStd::string, AZStd::string> GetExecutableFolder() const = 0;
    };

    using PythonAssetBuilderRequestBus = AZ::EBus<PythonAssetBuilderRequests>;

}
