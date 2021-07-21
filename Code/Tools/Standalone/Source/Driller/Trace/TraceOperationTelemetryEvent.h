/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef DRILLER_TRACE_TRACEOPERATIONTELEMETRYEVENT_H
#define DRILLER_TRACE_TRACEOPERATIONTELEMETRYEVENT_H

#include "Telemetry/TelemetryEvent.h"

namespace Driller
{
    class TraceOperationTelemetryEvent
        : public Telemetry::TelemetryEvent
    {
    public:
        TraceOperationTelemetryEvent()
            : Telemetry::TelemetryEvent("TraceDataViewOperation")
        {
        }
    };
}

#endif
