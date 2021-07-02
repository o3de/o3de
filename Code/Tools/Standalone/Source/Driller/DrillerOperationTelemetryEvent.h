/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_DRILLEROPERATIONTELEMETRYEVENT_H
#define DRILLER_DRILLEROPERATIONTELEMETRYEVENT_H

#include <AzCore/std/string/string.h>

#include "Source/Telemetry/TelemetryEvent.h"

namespace Driller
{
    // A class for any general Driller Operations(i.e. something which doesn't strictly related
    // to a specific window, for those a localized window operation should be used).
    class DrillerOperationTelemetryEvent
        : public Telemetry::TelemetryEvent
    {
    public:
        DrillerOperationTelemetryEvent()
            : Telemetry::TelemetryEvent("DrillerOperation")
        {
        }
    };

    class DrillerWindowLifepsanTelemetry
    {
    public:
        DrillerWindowLifepsanTelemetry(const char* windowName);
        virtual ~DrillerWindowLifepsanTelemetry();

    private:
        int m_windowId;
        AZStd::string m_windowName;
        DrillerOperationTelemetryEvent m_telemetryEvent;
    };
}

#endif
