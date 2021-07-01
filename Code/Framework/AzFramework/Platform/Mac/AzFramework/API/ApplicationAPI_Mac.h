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
    class DarwinLifecycleEvents
        : public AZ::EBusTraits
    {
    public:
        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~DarwinLifecycleEvents() {}

        using Bus = AZ::EBus<DarwinLifecycleEvents>;

        virtual void OnWillResignActive() {}
        virtual void OnDidResignActive() {}     // Constrain

        virtual void OnWillBecomeActive() {}
        virtual void OnDidBecomeActive() {}     // Unconstrain

        virtual void OnWillHide() {}
        virtual void OnDidHide() {}             // Suspend

        virtual void OnWillUnhide() {}
        virtual void OnDidUnhide() {}           // Resume

        virtual void OnWillTerminate() {}       // Terminate
    };
} // namespace AzFramework
