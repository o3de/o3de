
/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        m_currentMetricsEvent.AddAttribute(MetricsAttribute(METRICS_ATTRIBUTE_KEY_CLIENT_ID, clientId));
    }

    void MetricsEventBuilder::AddEventIdAttribute()
    {
        AZ::Uuid eventId = AZ::Uuid::Create();
        m_currentMetricsEvent.AddAttribute(MetricsAttribute(METRICS_ATTRIBUTE_KEY_EVENT_ID, eventId.ToString<AZStd::string>()));
    }

    void MetricsEventBuilder::AddSourceAttribute(const AZStd::string& eventSourceOverride)
    {
        AZStd::string eventSource = eventSourceOverride.empty() ? DefaultMetricsSource : eventSourceOverride;
        m_currentMetricsEvent.AddAttribute(MetricsAttribute(METRICS_ATTRIBUTE_KEY_EVENT_SOURCE, eventSource));
    }

    void MetricsEventBuilder::AddTimestampAttribute()
    {
        // Timestamp format is using the UTC ISO8601 format
        time_t now;
        time(&now);
        char buffer[50];
        strftime(buffer, sizeof(buffer), "%FT%TZ", gmtime(&now));

        m_currentMetricsEvent.AddAttribute(MetricsAttribute(METRICS_ATTRIBUTE_KEY_EVENT_TIMESTAMP, AZStd::string(buffer)));
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
