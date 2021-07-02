/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef DRILLER_PROFILER_PROFILEROPERATIONTELEMETRYEVENT_H
#define DRILLER_PROFILER_PROFILEROPERATIONTELEMETRYEVENT_H

#include "Source/Telemetry/TelemetryEvent.h"

namespace Driller
{
    class ProfilerOperationTelemetryEvent
        : public Telemetry::TelemetryEvent
    {
    public:
        ProfilerOperationTelemetryEvent()
            : Telemetry::TelemetryEvent("ProfileDataViewOperation")
        {
        }
    };
}

#endif
