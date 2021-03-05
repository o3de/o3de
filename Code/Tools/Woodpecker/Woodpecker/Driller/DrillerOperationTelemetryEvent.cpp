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
#include "DrillerOperationTelemetryEvent.h"

namespace Driller
{
    static int k_windowId = 0;

    DrillerWindowLifepsanTelemetry::DrillerWindowLifepsanTelemetry(const char* windowName)
        : m_windowId(k_windowId++)
        , m_windowName(windowName)
    {
        m_telemetryEvent.SetAttribute("WindowOpen", m_windowName);
        m_telemetryEvent.SetMetric("WindowId", m_windowId);
        m_telemetryEvent.Log();
        m_telemetryEvent.ResetEvent();
    }

    DrillerWindowLifepsanTelemetry::~DrillerWindowLifepsanTelemetry()
    {
        m_telemetryEvent.SetAttribute("WindowClose", m_windowName);
        m_telemetryEvent.SetMetric("WindowId", m_windowId);
        m_telemetryEvent.Log();
    }
}