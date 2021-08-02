/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "TelemetryEvent.h"

#include "TelemetryBus.h"

namespace Telemetry
{
    TelemetryEvent::TelemetryEvent(const char* eventName)
        : m_eventName(eventName)
    {
    }

    void TelemetryEvent::SetAttribute(const AZStd::string& name, const AZStd::string& value)
    {
        m_attributes[name] = value;
    }

    const AZStd::string& TelemetryEvent::GetAttribute(const AZStd::string& name)
    {
        static AZStd::string k_emptyString;

        AZStd::unordered_map< AZStd::string, AZStd::string >::iterator attributeIter = m_attributes.find(name);

        if (attributeIter != m_attributes.end())
        {
            return attributeIter->second;
        }

        return k_emptyString;
    }

    void TelemetryEvent::SetMetric(const AZStd::string& name, double metric)
    {
        m_metrics[name] = metric;
    }

    double TelemetryEvent::GetMetric(const AZStd::string& name)
    {
        AZStd::unordered_map< AZStd::string, double >::iterator metricIter = m_metrics.find(name);

        if (metricIter != m_metrics.end())
        {
            return metricIter->second;
        }

        return 0.0;
    }

    void TelemetryEvent::Log()
    {
        EBUS_EVENT(TelemetryEventsBus, LogEvent, (*this));
    }

    void TelemetryEvent::ResetEvent()
    {
        m_metrics.clear();
        m_attributes.clear();
    }

    const char* TelemetryEvent::GetEventName() const
    {
        return m_eventName.c_str();
    }

    const TelemetryEvent::AttributesMap& TelemetryEvent::GetAttributes() const
    {
        return m_attributes;
    }

    const TelemetryEvent::MetricsMap& TelemetryEvent::GetMetrics() const
    {
        return m_metrics;
    }
}
