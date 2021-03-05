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
#ifndef WOODPECKER_TELEMETRY_TELEMETRYCOMPONENT_H
#define WOODPECKER_TELEMETRY_TELEMETRYCOMPONENT_H

#include <AzCore/Component/Component.h>
#include <AzCore/std/chrono/chrono.h>

#include "TelemetryBus.h"

namespace Telemetry
{
    class TelemetryComponent
        : public AZ::Component
        , public TelemetryEventsBus::Handler
    {
    public:
        AZ_COMPONENT(TelemetryComponent, "{CE41EE3C-AF98-4B22-BA7C-2D425D1F468A}")

        TelemetryComponent() = default;

        //////////////////
        // AZ::Component
        void Activate();
        void Deactivate();
        static void Reflect(AZ::ReflectContext* context);
        //////////////////

        //////////////////////////////////////////
        // Telemetry::TelemetryEventBus::Handler
        void Initialize(const char* applicationName, AZ::u32 processIntervalInSeconds, bool doSDKInitShutdown) override;
        void LogEvent(const TelemetryEvent& event) override;
        void Shutdown() override;
        //////////////////////////////////////////
    };
}

#endif