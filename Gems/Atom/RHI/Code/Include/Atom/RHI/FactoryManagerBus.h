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

namespace AZ::RHI
{
    class Factory;

    //! A request to register a new Factory Backend.
    //! The Factory Manager will choose from all registered factories the one to use
    //! during system component activation.
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

        /// Enumerate the list of factories that exist in the Factory Manager.
        /// @param callback - The function that will get called for each factory that exists.
        /// The callback should return 'true' to continue enumerating or 'false' to stop enumerating.
        using FactoryVisitCallback = AZStd::function<bool(Factory* factory)>;
        virtual void EnumerateFactories(FactoryVisitCallback callback) = 0;
    public:

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };

    /// EBus for registering an RHI factory.
    using FactoryManagerBus = AZ::EBus<FactoryManagerRequest>;

    //! Notification regarding the state of the RHI factory
    class FactoryManagerNotification : public AZ::EBusTraits
    {
    public:
        virtual ~FactoryManagerNotification() = default;

        /// Called after a factory has been selected and registered.
        virtual void FactoryRegistered(){};

        /// Called after a factory has been selected and unregistered.
        virtual void FactoryUnregistered(){};

    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
    };

    /// EBus for sending notifications frpm the RHI factory.
    using FactoryManagerNotificationBus = AZ::EBus<FactoryManagerNotification>;
}
