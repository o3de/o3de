/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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
        void Activate() override;
        void Deactivate() override;
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
