/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ::RHI
{
    class ShaderPlatformInterface;

    //! A request to register a new ShaderPlatformInterface for a specific RHI backend.
    //! The shader asset builder will use the registered object to compile and create the proper
    //! shader asset. Each enabled RHI must register one ShaderPlatformInterface if the shader
    //! can be generated in the current platform.
    class ShaderPlatformInterfaceRegister
        : public AZ::EBusTraits
    {
    public:

        virtual ~ShaderPlatformInterfaceRegister() = default;

        //! Register a new ShaderPlatformInterface to generate shader assets.
        //!
        //!  @param shaderPlatformInterface The object used for generating the shader.
        virtual void RegisterShaderPlatformHandler(ShaderPlatformInterface* shaderPlatformInterface) = 0;

        //! Unregister the ShaderPlatformInterface for an RHI.
        //!
        //!  @param shaderPlatformInterface The object to unregister.
        virtual void UnregisterShaderPlatformHandler(ShaderPlatformInterface* shaderPlatformInterface) = 0;

    public:

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };

    /// EBus for registering a ShaderPlatformInterface for shader generation.
    using ShaderPlatformInterfaceRegisterBus = AZ::EBus<ShaderPlatformInterfaceRegister>;
}
