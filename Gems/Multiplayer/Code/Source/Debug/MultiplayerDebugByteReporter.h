/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>

namespace Multiplayer
{
    class MultiplayerDebugByteReporter
    {
    public:
        MultiplayerDebugByteReporter();
        virtual ~MultiplayerDebugByteReporter() = default;

        void ReportBytes(size_t byteSize);
        void AggregateBytes(size_t byteSize);
        void ReportAggregateBytes();

        float GetAverageBytes() const;
        size_t GetMaxBytes() const;
        size_t GetMinBytes() const;
        size_t GetTotalBytes() const;
        float GetKbitsPerSecond();

        void Combine(const MultiplayerDebugByteReporter& other);
        virtual void Reset();

        size_t GetTotalCount() const { return m_count; }

    private:
        size_t m_count;
        size_t m_totalBytes;
        size_t m_totalBytesThisSecond;
        float  m_totalBytesLastSecond;
        size_t m_minBytes;
        size_t m_maxBytes;
        size_t m_aggregateBytes;

        AZStd::chrono::steady_clock::time_point m_lastUpdateTime;
    };

    class MultiplayerDebugComponentReporter final
        : public MultiplayerDebugByteReporter
    {
    public:
        MultiplayerDebugComponentReporter() = default;

        void ReportField(const char* fieldName, size_t byteSize);
        void ReportFragmentEnd();

        using Report = AZStd::pair<AZStd::string, MultiplayerDebugByteReporter*>;
        AZStd::vector<Report> GetFieldReports();

        void Combine(const MultiplayerDebugComponentReporter& other);

    private:
        AZStd::map<AZStd::string, MultiplayerDebugByteReporter> m_fieldReports;
        MultiplayerDebugByteReporter m_componentDirtyBytes;
    };

    class MultiplayerDebugEntityReporter final
        : public MultiplayerDebugByteReporter
    {
    public:
        MultiplayerDebugEntityReporter() = default;

        void ReportField(AZ::u32 index, const char* componentName, const char* fieldName, size_t byteSize);
        void ReportFragmentEnd();

        void Combine(const MultiplayerDebugEntityReporter& other);
        void Reset() override;

        const char* GetEntityName() const { return m_entityName.c_str(); }
        void SetEntityName(const char* entityName)
        {
            // copying because the entity might go away
            m_entityName = entityName;
        }

        AZStd::map<AZStd::string, MultiplayerDebugComponentReporter>& GetComponentReports();

    private:
        MultiplayerDebugComponentReporter* m_currentComponentReport = nullptr;
        AZStd::map<AZStd::string, MultiplayerDebugComponentReporter> m_componentReports;
        AZStd::string m_entityName;
    };
}
