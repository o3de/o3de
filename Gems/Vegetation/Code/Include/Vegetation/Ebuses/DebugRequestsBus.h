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
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    using TimePoint = AZStd::chrono::system_clock::time_point;
    using TimeSpan = AZStd::sys_time_t;
    using FilterReasonCount = AZStd::unordered_map<AZStd::string_view, AZ::u32>;

    class DebugRequests
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        // reporting

        struct BaseTiming
        {
            TimeSpan m_averageTimeUs;
            TimeSpan m_peakTimeUs;
            TimeSpan m_lowestTimeUs;
            TimeSpan m_totalUpdateTimeUs;
            AZ::u32 m_totalCount = 0;
            AZ::u32 m_updateCount = 1;
            AZ::u32 m_numInstancesCreated = 0;
            TimePoint m_lastUpdateTime;
        };

        struct AreaSectorTiming
        {
            AZ::u32 m_numInstances = 0;
            FilterReasonCount m_numInstancesRejectedByFilters;
            TimeSpan m_totalTime;
            bool m_filteredByMasks = false;
        };

        using SectorId = AZStd::pair<int, int>;
        using AreaId = AZ::EntityId;

        struct SectorTiming : public BaseTiming
        {
            SectorId m_id;
            AZ::u32 m_numClaimPointsRemaining = 0; // number of sector points that were unused after a fill
            AZ::Vector3 m_worldPosition;
            AZStd::unordered_map<AreaId, AreaSectorTiming> m_perAreaData;
        };

        struct AreaTiming : public BaseTiming
        {
            AreaId m_id;
            AZ::u32 m_numClaimPointsRemaining = 0; // number of claim points that were unused after a fill
            AZStd::string m_areaName;
            AZStd::unordered_map<SectorId, AreaSectorTiming> m_perSectorData;
        };

        struct PerformanceReport
        {
            AZ::u64 m_count = 0;
            AZ::u64 m_activeInstanceCount = 0;
            TimePoint m_lastUpdateTime;
            AZStd::unordered_map<SectorId, SectorTiming> m_sectorTimingData;
            AZStd::unordered_map<AreaId, AreaTiming> m_areaTimingData;
        };

        virtual void GetPerformanceReport(PerformanceReport& report) const = 0;
        virtual void ClearPerformanceReport() = 0;

        // dumping

        enum class FilterTypeLevel
        {
            Danger,  // red level only
            Trace,   // green level and higher
            Warning, // yellow level and higher
        };

        enum class SortType
        {
            BySector, // sort by highest sector average descending
            BySectorDetailed, // Reports the time and instance count for each area in each sector
            ByArea,   // sort by highest area average descending
            ByAreaDetailed,   // Reports the time and instance count for each sector in each area.
        };

        virtual void DumpPerformanceReport(const PerformanceReport& report, FilterTypeLevel filter, SortType sort) const = 0;
    };
    using DebugRequestBus = AZ::EBus<DebugRequests>;
}
