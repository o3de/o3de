/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

namespace CrashReporting
{
    //! Brings up sentry-native crash reporting for packaged game and server launchers. Enabling
    //! the CrashReporting gem in a project is all that is required - there is no launcher-side
    //! call to add, which is what the Crashpad-era gem was missing (nothing ever called
    //! GameCrashHandler::InitCrashHandler, so packaged builds never reported a crash).
    class SentryCrashReportingSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(SentryCrashReportingSystemComponent, "{6E1B9A9C-2F1E-4E8B-9C36-1D3D0A5F2B77}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        SentryCrashReportingSystemComponent() = default;
        ~SentryCrashReportingSystemComponent() override = default;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus::Handler - only connected when app-hang tracking is enabled, so the
        // watchdog can tell a busy frame from a wedged main thread.
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
    };
}
