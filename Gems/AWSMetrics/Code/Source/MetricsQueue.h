/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MetricsEvent.h>

#include <AzCore/std/containers/deque.h>

namespace AWSMetrics
{
    //! MetricsQueue is used to buffer the submitted metrics before sending them in batch to the backend or local file.
    class MetricsQueue
    {
    public:
        const MetricsEvent& operator[](int index) const;

        //! Add a new metrics to the queue.
        //! @param metrics Metrics to add.
        void AddMetrics(const MetricsEvent& metrics);

        //! Append an existing metrics queue to the current queue.
        //! @param metricsQueue metrics queue to append.
        void AppendMetrics(MetricsQueue& metricsQueue);

        //! Push an existing metrics queue to the front of the current queue.
        //! @param metricsQueue metrics queue to push.
        void PushMetricsToFront(MetricsQueue& metricsQueue);

        //! Filter out lower priority metrics event in the queue if the queue size reaches the maximum capacity.
        //! @param maxSizeInBytes Maximum capacity of the queue.
        //! @return Total number of metrics events dropped because of the size limit.
        int FilterMetricsByPriority(size_t maxSizeInBytes);

        //! Empty the metrics queue.
        //! Unsubmitted metrics will be lost after this operation.
        void ClearMetrics();

        int GetNumMetrics() const;

        //! Get the total size of all the metrics inside the queue.
        //! @return size of the queue in bytes
        size_t GetSizeInBytes() const;

        //! Serialize the metrics events queue to a string.
        //! @return Serialized string.
        AZStd::string SerializeToJson();

        //! Serialize the metrics queue to JSON for sending requests.
        //! @param writer JSON writer for the serialization.
        //! @return Whether the metrics queue is serialized successfully.
        bool SerializeToJson(AWSCore::JsonWriter& writer) const;

        //! Pop buffered metrics events by the payload size and record count limits and add them to a new queue.
        //! @param bufferedEvents The metrics queue to store poped metrics events.
        //! @param maxPayloadSizeInMb Maximum size for the service API request payload.
        //! @param maxBatchedRecordsCount Maximum number of records sent in a service API request.
        void PopBufferedEventsByServiceLimits(MetricsQueue& bufferedEvents, int maxPayloadSizeInMb, int maxBatchedRecordsCount);

        //! Read from a local JSON file to the metrics queue.
        //! @param filePath Path to the local JSON file.
        //! @return Whether the metrics queue is created successfully.
        bool ReadFromJson(const AZStd::string& filePath);

    private:
        bool ReadFromJsonDocument(rapidjson::Document& doc);

        AZStd::deque<MetricsEvent> m_metrics; //!< Metrics included in the queue.
        size_t m_sizeSerializedToJson = 0; //! < Metrics queue size serialized to json.
    };
} // namespace AWSMetrics
