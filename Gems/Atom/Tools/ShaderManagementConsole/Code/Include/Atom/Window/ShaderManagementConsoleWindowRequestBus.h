/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleWindowRequestBus provides 
    class ShaderManagementConsoleWindowRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        /// Creates and shows main window
        virtual void CreateShaderManagementConsoleWindow() = 0;

        //! Destroys main window
        virtual void DestroyShaderManagementConsoleWindow() = 0;
    };
    using ShaderManagementConsoleWindowRequestBus = AZ::EBus<ShaderManagementConsoleWindowRequests>;

} // namespace ShaderManagementConsole
