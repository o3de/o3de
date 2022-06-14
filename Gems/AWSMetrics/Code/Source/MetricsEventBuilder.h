/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MetricsAttribute.h>
#include <MetricsEvent.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>

namespace AWSMetrics
{
    //! MetricsEventBuilder builds a new metrics event and adds metrics attributes to it.
    class MetricsEventBuilder
    {
    public:
        MetricsEventBuilder();
        virtual ~MetricsEventBuilder() = default;

        //! Add default attributes to the metrics event including event_id, source and timestamp.
        //! @param clientId Unique identifier for the client.
        //! @param metricSourceOverride Event source used to override the default value.
        //! @return The builder itself.
        virtual MetricsEventBuilder& AddDefaultMetricsAttributes(const AZStd::string& clientId, const AZStd::string& metricSourceOverride = "");

        //! Add attributes to the metrics event.
        //! @param attributes List of attributes to add to the metrics event.
        //! @return The builder itself.
        virtual MetricsEventBuilder& AddMetricsAttributes(const AZStd::vector<MetricsAttribute>& attributes);

        //! Set the priority of the metrics event.
        //! @param priority Priority of the metrics event.
        //! @return The builder itself.
        MetricsEventBuilder& SetMetricsPriority(int priority);

        //! Build a metrics event.
        //! Make sure that this function is only called once for each builder instance. Otherwise it will a new metrics event.
        //! @return Metrics event constructed by the builder.
        MetricsEvent Build();

    private:
        //! Add the client Id attribute to the metrics event.
        //! @param clientId Unique identifier for the client.
        void AddClientIdAttribute(const AZStd::string& clientId);

        //! Add the event Id attribute (an UUID) to the metrics event.
        void AddEventIdAttribute();

        //! Add the event source attribute to the metrics event.
        //! Default to be AWSMetricsGem.
        //! @param metricSourceOverride Event source used to override the default value.
        void AddSourceAttribute(const AZStd::string& metricSourceOverride = "");

        //! Add the event timestamp attribute in the UTC ISO8601 format to the metrics event.
        void AddTimestampAttribute();

        MetricsEvent m_currentMetricsEvent; //!< Metrics event constructed by the builder.
    };
} // namespace AWSMetrics
