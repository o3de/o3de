/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
