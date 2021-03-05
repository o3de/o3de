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
#include "Woodpecker_precompiled.h"

#include "TelemetryComponent.h"

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
