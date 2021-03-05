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
    struct PlatformInfo;
}

namespace AZ
{
    namespace RHI
    {
        class ShaderPlatformInterface;
    }

    namespace ShaderBuilder
    {
        /**
         * A request to get the registered shader platform interfaces.
         */
        class ShaderPlatformInterfaceRequest
            : public AZ::EBusTraits
        {
        public:

            virtual ~ShaderPlatformInterfaceRequest() = default;

            /**
             * Get the list of registered shader platform interfaces for a specific platform.
             *
             *  @param platformInfo The target platform.
             */
            virtual AZStd::vector<RHI::ShaderPlatformInterface*> GetShaderPlatformInterface(const AssetBuilderSDK::PlatformInfo& platformInfo) = 0;

        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        };

        /// EBus for requesting the ShaderPlatformInterfaces for a platform.
        using ShaderPlatformInterfaceRequestBus = AZ::EBus<ShaderPlatformInterfaceRequest>;
    }
}
