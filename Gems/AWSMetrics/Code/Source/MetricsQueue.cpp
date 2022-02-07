/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MetricsQueue.h>
#include <Framework/JsonWriter.h>

#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/std/sort.h>

#include <sstream>

namespace AWSMetrics
{
    const MetricsEvent& MetricsQueue::operator[](int index) const
    {
        AZ_Assert(index >= 0 && index < m_metrics.size(), "Index for the metrics queue is out of range.");

        return m_metrics[index];
    }

    void MetricsQueue::AddMetrics(const MetricsEvent& metrics)
    {
        m_sizeSerializedToJson += metrics.GetSizeInBytes();

        m_metrics.emplace_back(AZStd::move(metrics));
    }

    void MetricsQueue::AppendMetrics(MetricsQueue& metricsQueue)
    {
        if (metricsQueue.GetNumMetrics() == 0)
        {
            return;
        }

        m_sizeSerializedToJson += metricsQueue.GetSizeInBytes();

        if (m_metrics.size() == 0)
        {
            m_metrics = AZStd::move(metricsQueue.m_metrics);
        }
        else
        {
            AZStd::move(metricsQueue.m_metrics.begin(), metricsQueue.m_metrics.end(), std::back_inserter(m_metrics));
        }
    }

    void MetricsQueue::PushMetricsToFront(MetricsQueue& metricsQueue)
    {
        if (metricsQueue.GetNumMetrics() == 0)
        {
            return;
        }

        m_sizeSerializedToJson += metricsQueue.GetSizeInBytes();

        if (m_metrics.size() == 0)
        {
            m_metrics = AZStd::move(metricsQueue.m_metrics);
        }
        else
        {
            AZStd::move(metricsQueue.m_metrics.begin(), metricsQueue.m_metrics.end(), std::front_inserter(m_metrics));
        }
    }

    int MetricsQueue::FilterMetricsByPriority(size_t maxSizeInBytes)
    {
        if (GetSizeInBytes() < maxSizeInBytes)
        {
            return 0;
        }

        int numCurrentMetricsEvents = GetNumMetrics();
        AZStd::vector<AZStd::pair<const MetricsEvent*, int>> sortedMetrics;
        for (int index = 0; index < numCurrentMetricsEvents; ++index)
        {
            sortedMetrics.emplace_back(AZStd::pair<const MetricsEvent*, int>(&m_metrics[index], index));
        }

        // Sort the existing metrics event by event priority.
        // We need to reverse the relative order of the metrics events with the same priority to
        // avoid newer events being discarded when the max size capacity is reached.
        AZStd::stable_sort(sortedMetrics.begin(), sortedMetrics.end(),
            [](const AZStd::pair<const MetricsEvent*, int>& left, const AZStd::pair<const MetricsEvent*, int>& right)
            {
                if (left.first->GetEventPriority() == right.first->GetEventPriority())
                {
                    return left.second > right.second;
                }
                else
                {
                    return left.first->GetEventPriority() < right.first->GetEventPriority();
                }
            }
        );

        AZStd::deque<MetricsEvent> result;       
        m_sizeSerializedToJson = 0;
        for (const AZStd::pair<const MetricsEvent*, int>& pair : sortedMetrics)
        {
            if (m_sizeSerializedToJson < maxSizeInBytes)
            {
                m_sizeSerializedToJson += pair.first->GetSizeInBytes();
                result.emplace_back(AZStd::move(*(pair.first)));
            }
            else
            {
                break;
            }
        }


        m_metrics.clear();
        m_metrics = AZStd::move(result);

        return numCurrentMetricsEvents - GetNumMetrics();
    }

    void MetricsQueue::ClearMetrics()
    {
        m_sizeSerializedToJson = 0;
        m_metrics.clear();
    }

    int MetricsQueue::GetNumMetrics() const
    {
        return static_cast<int>(m_metrics.size());
    }

    size_t MetricsQueue::GetSizeInBytes() const
    {
        return m_sizeSerializedToJson;
    }

    AZStd::string MetricsQueue::SerializeToJson()
    {
        std::stringstream stringStream;
        AWSCore::JsonOutputStream jsonStream{stringStream};
        AWSCore::JsonWriter writer{jsonStream};

        SerializeToJson(writer);

        return stringStream.str().c_str();
    }

    bool MetricsQueue::SerializeToJson(AWSCore::JsonWriter& writer) const
    {
        bool ok = true;
        ok = ok && writer.StartArray();

        for (auto& metrics : m_metrics)
        {
            ok = ok && metrics.SerializeToJson(writer);
        }
        ok = ok && writer.EndArray();
        return ok;
    }

    void MetricsQueue::PopBufferedEventsByServiceLimits(MetricsQueue& bufferedEvents, int maxPayloadSizeInBytes, int maxBatchedRecordsCount)
    {
        int curNum = 0;
        int curSizeInBytes = 0;
        while (m_metrics.size() > 0)
        {
            MetricsEvent& curEvent = m_metrics.front();

            curNum += 1;
            curSizeInBytes += static_cast<int>(curEvent.GetSizeInBytes());
            if (curNum <= maxBatchedRecordsCount && curSizeInBytes <= maxPayloadSizeInBytes)
            {
                m_sizeSerializedToJson -= curEvent.GetSizeInBytes();
                bufferedEvents.AddMetrics(curEvent);
                m_metrics.pop_front();
            }
            else
            {
                break;
            }
        }
    }

    bool MetricsQueue::ReadFromJson(const AZStd::string& filePath)
    {
        auto result = AZ::JsonSerializationUtils::ReadJsonFile(filePath);
        if (!result.IsSuccess() ||!ReadFromJsonDocument(result.GetValue()))
        {
            AZ_Error("AWSMetrics", false, "Failed to read metrics file %s", filePath.c_str());
            return false;
        }

        return true;
    }

    bool MetricsQueue::ReadFromJsonDocument(rapidjson::Document& doc)
    {
        if (!doc.IsArray())
        {
            return false;
        }

        for (rapidjson::SizeType metricsIndex = 0; metricsIndex < doc.Size(); metricsIndex++)
        {
            MetricsEvent metrics;
            if (!metrics.ReadFromJson(doc[metricsIndex]))
            {
                return false;
            }

            // Read through each element in the array and add it as a new metrics
            AddMetrics(metrics);
        }

        return true;
    }
}
