/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace EMotionFX
{
    //! Provides a bus to tell the physics setup manipulators when their underlying properties have changed.
    class PhysicsSetupManipulatorRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Called when underlying properties are changed, so that the manipulators can be updated.
        virtual void OnUnderlyingPropertiesChanged() {}
    };
    using PhysicsSetupManipulatorRequestBus = AZ::EBus<PhysicsSetupManipulatorRequests>;
} // 
