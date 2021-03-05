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
