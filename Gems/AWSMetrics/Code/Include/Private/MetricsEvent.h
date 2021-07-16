/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MetricsAttribute.h>

#include <AzCore/std/containers/vector.h>

namespace AWSMetrics
{
    static const char* DefaultMetricsSource = "AWSMetricGem";

    //! Metrics event is used to represent one event which contains a collection of metrics attributes.
    class MetricsEvent
    {
    public:
        MetricsEvent() = default;
        virtual ~MetricsEvent() = default;

        //! Add a new attribute to the metrics.
        //! @param attribute Attribute to add.
        void AddAttribute(const MetricsAttribute& attribute);

        //! Add attributes to the metrics event.
        //! @param attributes List of attributes to append.
        void AddAttributes(const AZStd::vector<MetricsAttribute>& attributes);

        int GetNumAttributes() const;

        //! Get the metrics event size serialized to json.
        //! @return metrics event size in bytes.
        size_t GetSizeInBytes() const;

        //! Serialize the metrics event to JSON for the sending requests.
        //! @param writer JSON writer for the serialization.
        //! @return Whether the metrics event is serialized successfully.
        bool SerializeToJson(AWSCore::JsonWriter& writer) const;

        //! Read from a JSON value to the metrics event.
        //! @param metricsObjVal JSON value to read from.
        //! @return Whether the metrics event is created successfully.
        bool ReadFromJson(rapidjson::Value& metricsObjVal);

        //! Validate the metrics event with the predefined JSON schema.
        //! @return whether the metrics event match the JSON schema.
        bool ValidateAgainstSchema();

        //! Add the count of failures for sending the metrics event.
        void MarkFailedSubmission();

        //! Get the count of failures for sending the metrics event.
        //! @return Count of failures for sending the metrics event.
        int GetNumFailures() const;

        //! Set the priority of a metrics event.
        //! @param priority Priority to set.
        void SetEventPriority(int priority);

        //! Get the priority of the metrics event.
        //! @return Priority of the metrics event.
        int GetEventPriority() const;

    private:
        //! Check whether the attribute exists in the metrics event.
        //! @param attributeName Attribute name to check.
        //! @return whether the attribute exists.
        bool AttributeExists(const AZStd::string& attributeName) const;

        AZStd::vector<MetricsAttribute> m_attributes; //!< Attributes included in the metrics.
        size_t m_sizeSerializedToJson = 0; //! < Metrics event size serialized to json.
        int m_numFailures = 0; //! < Count of failures for sending the metrics event.
        int m_eventPriority = 0; //! < Priority of the metrics event.
    };
} // namespace AWSMetrics
