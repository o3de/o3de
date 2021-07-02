/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AutomatedLauncherTesting
{
    class AutomatedLauncherTestingRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Call this method from your test logic when a test is complete.
        virtual void CompleteTest(bool success, const AZStd::string& message) = 0;
    };
    using AutomatedLauncherTestingRequestBus = AZ::EBus<AutomatedLauncherTestingRequests>;
} // namespace AutomatedLauncherTesting
