/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef DRILLER_REPLICA_REPLICAOPERATIONTELEMETRYEVENT_H
#define DRILLER_REPLICA_REPLICAOPERATIONTELEMETRYEVENT_H

#include "Source/Telemetry/TelemetryEvent.h"

namespace Driller
{
    class ReplicaOperationTelemetryEvent
        : public Telemetry::TelemetryEvent
    {
    public:
        ReplicaOperationTelemetryEvent()
            : Telemetry::TelemetryEvent("ReplicaDataViewOperation")
        {
        }
    };
}

#endif
