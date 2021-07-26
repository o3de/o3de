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
#include "MultiplayerDebugByteReporter.h"

#include <iomanip>
#include <sstream>
#include <AzCore/std/sort.h>

namespace MultiplayerDiagnostics
{
    void MultiplayerDebugByteReporter::ReportBytes(size_t byteSize)
    {
        m_count++;
        m_totalBytes += byteSize;
        m_totalBytesThisSecond += byteSize;
        m_minBytes = AZStd::min(m_minBytes, byteSize);
        m_maxBytes = AZStd::max(m_maxBytes, byteSize);
    }

    void MultiplayerDebugByteReporter::AggregateBytes(size_t byteSize)
    {
        m_aggregateBytes += byteSize;
    }

    void MultiplayerDebugByteReporter::ReportAggregateBytes()
    {
        ReportBytes(m_aggregateBytes);
        m_aggregateBytes = 0;
    }

    float MultiplayerDebugByteReporter::GetAverageBytes() const
    {
        if (m_count == 0)
        {
            AZ_Warning("MultiplayerDebugByteReporter", m_totalBytes == 0, "Attempted to average bytes with a zero count.");
            return 0.0f;
        }

        return (1.0f * m_totalBytes) / m_count;
    }

    size_t MultiplayerDebugByteReporter::GetMaxBytes() const
    {
        return m_maxBytes;
    }

    size_t MultiplayerDebugByteReporter::GetMinBytes() const
    {
        return m_minBytes;
    }

    size_t MultiplayerDebugByteReporter::GetTotalBytes() const
    {
        return m_totalBytes;
    }

    float MultiplayerDebugByteReporter::GetKbitsPerSecond()
    {
        auto now = AZStd::chrono::monotonic_clock::now();

        // Check the amount of time elapsed and update totals if necessary.
        // Time here is measured in whole seconds from the epoch, providing synchronization in
        // reporting intervals across all byte reporters.
        AZStd::chrono::seconds nowSeconds = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(now.time_since_epoch());
        AZStd::chrono::seconds secondsSinceLastUpdate = nowSeconds -
            AZStd::chrono::duration_cast<AZStd::chrono::seconds>(m_lastUpdateTime.time_since_epoch());
        if (secondsSinceLastUpdate.count())
        {
            // normalize over elapsed milliseconds
            const int k_millisecondsPerSecond = 1000;
            auto msSinceLastUpdate = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(now - m_lastUpdateTime);
            m_totalBytesLastSecond = k_millisecondsPerSecond * (1.f * m_totalBytesThisSecond / msSinceLastUpdate.count());
            m_totalBytesThisSecond = 0;
            m_lastUpdateTime = now;
        }

        const float k_bitsPerByte = 8.0f;
        const int k_bitsPerKilobit = 1024;
        return k_bitsPerByte * m_totalBytesLastSecond / k_bitsPerKilobit;
    }

    void MultiplayerDebugByteReporter::Combine(const MultiplayerDebugByteReporter& other)
    {
        m_count += other.m_count;
        m_totalBytes += other.m_totalBytes;
        m_totalBytesThisSecond += other.m_totalBytesThisSecond;
        m_minBytes = AZStd::GetMin(m_minBytes, other.m_minBytes);
        m_maxBytes = AZStd::GetMax(m_maxBytes, other.m_maxBytes);
    }

    void MultiplayerDebugByteReporter::Reset()
    {
        m_count = 0;
        m_totalBytes = 0;
        m_totalBytesThisSecond = 0;
        m_totalBytesLastSecond = 0;
        m_minBytes = std::numeric_limits<decltype(m_minBytes)>::max();
        m_maxBytes = 0;
        m_aggregateBytes = 0;
    }

    void ComponentReporter::ReportField(const char* fieldName, size_t byteSize)
    {
        MultiplayerDebugByteReporter::AggregateBytes(byteSize);
        m_fieldReports[fieldName].ReportBytes(byteSize);
    }

    void ComponentReporter::ReportFragmentEnd()
    {
        MultiplayerDebugByteReporter::ReportAggregateBytes();
        m_componentDirtyBytes.ReportAggregateBytes();
    }

    AZStd::vector<ComponentReporter::Report> ComponentReporter::GetFieldReports()
    {
        AZStd::vector<Report> copy;
        for (auto field = m_fieldReports.begin(); field != m_fieldReports.end(); ++field)
        {
            copy.emplace_back(field->first, &field->second);
        }

        auto sortByFrequency = [](const Report& a, const Report& b)
        {
            return a.second->GetTotalCount() > b.second->GetTotalCount();
        };

        AZStd::sort(copy.begin(), copy.end(), sortByFrequency);

        return copy;
    }

    void ComponentReporter::Combine(const ComponentReporter& other)
    {
        MultiplayerDebugByteReporter::Combine(other);

        for (const auto& fieldIter : other.m_fieldReports)
        {
            m_fieldReports[fieldIter.first].Combine(fieldIter.second);
        }

        m_componentDirtyBytes.Combine(other.m_componentDirtyBytes);
    }

    void EntityReporter::ReportField(AZ::u32 index, const char* componentName,
        const char* fieldName, size_t byteSize)
    {
        if (m_currentComponentReport == nullptr)
        {
            std::stringstream component;
            component << "[" << std::setw(2) << std::setfill('0') << static_cast<int>(index) << "]" << " " << componentName;
            m_currentComponentReport = &m_componentReports[component.str().c_str()];
        }

        m_currentComponentReport->ReportField(fieldName, byteSize);
        MultiplayerDebugByteReporter::AggregateBytes(byteSize);
    }

    void EntityReporter::ReportDirtyBits(AZ::u32 index, const char* componentName, size_t byteSize)
    {
        const char* const prefix = "MB::";
        if (strncmp(prefix, componentName, 4) == 0)
        {
            componentName += strlen(prefix);
        }

        if (m_currentComponentReport == nullptr)
        {
            std::stringstream component;
            component << "[" << std::setw(2) << std::setfill('0') << static_cast<int>(index) << "]" << " " << componentName;
            m_currentComponentReport = &m_componentReports[component.str().c_str()];
        }

        m_currentComponentReport->ReportDirtyBits(byteSize);
        m_gdeDirtyBytes.AggregateBytes(byteSize);
    }

    void EntityReporter::ReportFragmentEnd()
    {
        if (m_currentComponentReport)
        {
            m_currentComponentReport->ReportFragmentEnd();
            m_currentComponentReport = nullptr;
        }

        m_gdeDirtyBytes.ReportAggregateBytes();
        MultiplayerDebugByteReporter::ReportAggregateBytes();
    }

    void EntityReporter::Combine(const EntityReporter& other)
    {
        MultiplayerDebugByteReporter::Combine(other);

        for (const auto& componentIter : other.m_componentReports)
        {
            m_componentReports[componentIter.first].Combine(componentIter.second);
        }

        m_gdeDirtyBytes.Combine(other.m_gdeDirtyBytes);
    }

    void EntityReporter::Reset()
    {
        MultiplayerDebugByteReporter::Reset();

        m_componentReports.clear();
        m_gdeDirtyBytes.Reset();
    }

    AZStd::map<AZStd::string, ComponentReporter>& EntityReporter::GetComponentReports()
    {
        return m_componentReports;
    }
}
