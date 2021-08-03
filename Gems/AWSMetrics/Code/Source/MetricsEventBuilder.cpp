
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsConstant.h>
#include <MetricsEventBuilder.h>
#include <MetricsEvent.h>

#include <AzCore/Math/Uuid.h>

#include <ctime>

#pragma warning(disable : 4996)


namespace AWSMetrics
{
    MetricsEventBuilder::MetricsEventBuilder()
        : m_currentMetricsEvent(MetricsEvent())
    {
        AddTimestampAttribute();
    }

    MetricsEventBuilder& MetricsEventBuilder::AddDefaultMetricsAttributes(const AZStd::string& clientId, const AZStd::string& metricSourceOverride)
    {
        AddClientIdAttribute(clientId);
        AddEventIdAttribute();
        AddSourceAttribute(metricSourceOverride);

        return *this;
    }

    void MetricsEventBuilder::AddClientIdAttribute(const AZStd::string& clientId)
    {
        m_currentMetricsEvent.AddAttribute(MetricsAttribute(AwsMetricsAttributeKeyClientId, clientId));
    }

    void MetricsEventBuilder::AddEventIdAttribute()
    {
        AZ::Uuid eventId = AZ::Uuid::Create();
        m_currentMetricsEvent.AddAttribute(MetricsAttribute(AwsMetricsAttributeKeyEventId, eventId.ToString<AZStd::string>()));
    }

    void MetricsEventBuilder::AddSourceAttribute(const AZStd::string& eventSourceOverride)
    {
        AZStd::string eventSource = eventSourceOverride.empty() ? DefaultMetricsSource : eventSourceOverride;
        m_currentMetricsEvent.AddAttribute(MetricsAttribute(AwsMetricsAttributeKeyEventSource, eventSource));
    }

    void MetricsEventBuilder::AddTimestampAttribute()
    {
        // Timestamp format is using the UTC ISO8601 format
        time_t now;
        time(&now);
        char buffer[50];
        strftime(buffer, sizeof(buffer), "%FT%TZ", gmtime(&now));

        m_currentMetricsEvent.AddAttribute(MetricsAttribute(AwsMetricsAttributeKeyEventTimestamp, AZStd::string(buffer)));
    }

    MetricsEventBuilder& MetricsEventBuilder::AddMetricsAttributes(const AZStd::vector<MetricsAttribute>& attributes)
    {
        m_currentMetricsEvent.AddAttributes(attributes);

        return *this;
    }

    MetricsEventBuilder& MetricsEventBuilder::SetMetricsPriority(int priority)
    {
        m_currentMetricsEvent.SetEventPriority(priority);

        return *this;
    }

    MetricsEvent MetricsEventBuilder::Build()
    {
        MetricsEvent result = AZStd::move(m_currentMetricsEvent);

        // Set m_currentMetricsEvent to a new metrics event instance after the first call to this function.
        m_currentMetricsEvent = MetricsEvent();

        return AZStd::move(result);
    }
}
