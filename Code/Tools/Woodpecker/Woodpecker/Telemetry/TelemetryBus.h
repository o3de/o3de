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
#ifndef TELEMETRY_TELEMETRYBUS_H
#define TELEMETRY_TELEMETRYBUS_H

#include <AzCore/EBus/EBus.h>

#include "Woodpecker/Telemetry/TelemetryEvent.h"

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