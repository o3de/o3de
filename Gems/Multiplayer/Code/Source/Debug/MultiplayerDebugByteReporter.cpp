/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerDebugByteReporter.h"

#include <AzCore/Casting/numeric_cast.h>

#include <iomanip> // for std::setfill
#include <sstream>
#include <AzCore/std/sort.h>

namespace Multiplayer
{
    MultiplayerDebugByteReporter::MultiplayerDebugByteReporter()
    {
        MultiplayerDebugByteReporter::Reset();
    }

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
            return 0.0f;
        }

        return aznumeric_cast<float>(m_totalBytes) / aznumeric_cast<float>(m_count);
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
        const auto now = AZStd::chrono::steady_clock::now();

        // Check the amount of time elapsed and update totals if necessary.
        // Time here is measured in whole seconds from the epoch, providing synchronization in
        // reporting intervals across all byte reporters.
        const AZStd::chrono::seconds nowSeconds = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(now.time_since_epoch());
        const AZStd::chrono::seconds secondsSinceLastUpdate = nowSeconds -
            AZStd::chrono::duration_cast<AZStd::chrono::seconds>(m_lastUpdateTime.time_since_epoch());
        if (secondsSinceLastUpdate.count())
        {
            // normalize over elapsed milliseconds
            constexpr int k_millisecondsPerSecond = 1000;
            const auto msSinceLastUpdate = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(now - m_lastUpdateTime);
            m_totalBytesLastSecond = k_millisecondsPerSecond * aznumeric_cast<float>(m_totalBytesThisSecond) / aznumeric_cast<float>(msSinceLastUpdate.count());
            m_totalBytesThisSecond = 0;
            m_lastUpdateTime = now;
        }

        constexpr float bitsPerByte = 8.0f;
        constexpr int bitsPerKilobit = 1024;
        return bitsPerByte * m_totalBytesLastSecond / bitsPerKilobit;
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

    void MultiplayerDebugComponentReporter::ReportField(const char* fieldName, size_t byteSize)
    {
        MultiplayerDebugByteReporter::AggregateBytes(byteSize);
        m_fieldReports[fieldName].ReportBytes(byteSize);
    }

    void MultiplayerDebugComponentReporter::ReportFragmentEnd()
    {
        MultiplayerDebugByteReporter::ReportAggregateBytes();
        m_componentDirtyBytes.ReportAggregateBytes();
    }

    AZStd::vector<MultiplayerDebugComponentReporter::Report> MultiplayerDebugComponentReporter::GetFieldReports()
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

    void MultiplayerDebugComponentReporter::Combine(const MultiplayerDebugComponentReporter& other)
    {
        MultiplayerDebugByteReporter::Combine(other);

        for (const auto& fieldIterator : other.m_fieldReports)
        {
            m_fieldReports[fieldIterator.first].Combine(fieldIterator.second);
        }

        m_componentDirtyBytes.Combine(other.m_componentDirtyBytes);
    }

    void MultiplayerDebugEntityReporter::ReportField(AZ::u32 index, const char* componentName,
        const char* fieldName, size_t byteSize)
    {
        if (m_currentComponentReport == nullptr)
        {
            std::stringstream component;
            component << "[" << std::setw(2) << std::setfill('0') << aznumeric_cast<int>(index) << "]" << " " << componentName;
            m_currentComponentReport = &m_componentReports[component.str().c_str()];
        }

        m_currentComponentReport->ReportField(fieldName, byteSize);
        MultiplayerDebugByteReporter::AggregateBytes(byteSize);
    }

    void MultiplayerDebugEntityReporter::ReportFragmentEnd()
    {
        if (m_currentComponentReport)
        {
            m_currentComponentReport->ReportFragmentEnd();
            m_currentComponentReport = nullptr;
        }

        MultiplayerDebugByteReporter::ReportAggregateBytes();
    }

    void MultiplayerDebugEntityReporter::Combine(const MultiplayerDebugEntityReporter& other)
    {
        MultiplayerDebugByteReporter::Combine(other);

        for (const auto& componentIterator : other.m_componentReports)
        {
            m_componentReports[componentIterator.first].Combine(componentIterator.second);
        }

        SetEntityName(other.GetEntityName());
    }

    void MultiplayerDebugEntityReporter::Reset()
    {
        MultiplayerDebugByteReporter::Reset();

        m_componentReports.clear();
    }

    AZStd::map<AZStd::string, MultiplayerDebugComponentReporter>& MultiplayerDebugEntityReporter::GetComponentReports()
    {
        return m_componentReports;
    }
}
