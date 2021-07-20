/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <Atom/RHI/ValidationLayer.h>

namespace AZ
{
    namespace RHI
    {
        class Factory;
        /**
         * A request to register a new Factory Backend.
         * The Factory Manager will choose from all registered factories the one to use
         * during system component activation.
         */
        class FactoryManagerRequest
            : public AZ::EBusTraits
        {
        public:

            virtual ~FactoryManagerRequest() = default;

            /// Register an available RHI Factory on the current platform.
            virtual void RegisterFactory(Factory* factory) = 0;

            /// Unregister a previously registered factory.
            virtual void UnregisterFactory(Factory* factory) = 0;

            /// Called when all available factories have already register.
            virtual void FactoryRegistrationFinalize() = 0;

            /// Determine what level of validation the RHI device should use
            /// e.g. whether Vulkan or D3D should activate their debug layers and to what extent
            virtual AZ::RHI::ValidationMode DetermineValidationMode() const = 0;

        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        };

        /// EBus for registering an RHI factory.
        using FactoryManagerBus = AZ::EBus<FactoryManagerRequest>;
    }
}
