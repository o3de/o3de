/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef TELEMETRY_TELEMETRYBUS_H
#define TELEMETRY_TELEMETRYBUS_H

#include <AzCore/EBus/EBus.h>

#include "Source/Telemetry/TelemetryEvent.h"

namespace Telemetry
{
    class TelemetryComponent;

    class TelemetryEvents
        : public AZ::EBusTraits
    {
    public:
        virtual void Initialize(const char* applicationName, AZ::u32 processIntervalInSecs, bool doSDKInitShutdown) = 0;
        virtual void LogEvent(const TelemetryEvent& event) = 0;
        virtual void Shutdown() = 0;
    };

    typedef AZ::EBus<TelemetryEvents> TelemetryEventsBus;
}
#endif
