/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "TelemetryComponent.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace Telemetry
{
    void TelemetryComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<TelemetryComponent, AZ::Component>()
                ->Version(1)
            ;
        }
    }

    void TelemetryComponent::Activate()
    {
        TelemetryEventsBus::Handler::BusConnect();
    }

    void TelemetryComponent::Deactivate()
    {
        Shutdown();
        TelemetryEventsBus::Handler::BusDisconnect();
    }

    void TelemetryComponent::Initialize([[maybe_unused]] const char* applicationName, [[maybe_unused]] AZ::u32 processInterval, [[maybe_unused]] bool doAPIInitShutdown)
    {
    }

    void TelemetryComponent::LogEvent([[maybe_unused]] const TelemetryEvent& telemetryEvent)
    {
    }

    void TelemetryComponent::Shutdown()
    {
    }
}
