/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
