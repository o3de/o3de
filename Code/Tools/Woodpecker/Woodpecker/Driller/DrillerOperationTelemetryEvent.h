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

#ifndef DRILLER_DRILLEROPERATIONTELEMETRYEVENT_H
#define DRILLER_DRILLEROPERATIONTELEMETRYEVENT_H

#include <AzCore/std/string/string.h>

#include "Woodpecker/Telemetry/TelemetryEvent.h"

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