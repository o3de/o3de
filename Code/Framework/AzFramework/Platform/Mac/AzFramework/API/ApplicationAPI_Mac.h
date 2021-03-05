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
