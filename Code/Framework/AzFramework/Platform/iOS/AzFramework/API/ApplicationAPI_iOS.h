/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    class IosLifecycleEvents
        : public AZ::EBusTraits
    {
    public:
        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~IosLifecycleEvents() {}

        using Bus = AZ::EBus<IosLifecycleEvents>;

        virtual void OnWillResignActive() {}        // Constrain
        virtual void OnDidBecomeActive() {}         // Unconstrain

        virtual void OnDidEnterBackground() {}      // Suspend
        virtual void OnWillEnterForeground() {}     // Resume

        virtual void OnWillTerminate() {}           // Terminate
        virtual void OnDidReceiveMemoryWarning() {} // Low memory
    };
} // namespace AzFramework
