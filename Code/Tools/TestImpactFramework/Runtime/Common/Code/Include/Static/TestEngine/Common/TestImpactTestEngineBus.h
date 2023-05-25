/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/Job/TestImpactTestEngineJob.h>

#include <AzCore/EBus/EBus.h>

namespace TestImpact
{
    //! Bus for test engine notifications.
    template<typename TestTarget>
    class TestEngineNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides ...
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Callback completed test engine jobs.
        //! @param testJob The test engine job that has completed.
        virtual void OnJobComplete([[maybe_unused]] const TestEngineJob<TestTarget>& testJob)
        {
        }
    };

    template<typename TestTarget>
    using TestEngineNotificationBus = AZ::EBus<TestEngineNotifications<TestTarget>>;
} // namespace TestImpact
