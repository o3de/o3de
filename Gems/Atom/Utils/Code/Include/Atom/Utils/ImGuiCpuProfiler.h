/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI.Reflect/CpuTimingStatistics.h>

namespace AZ
{
    namespace RHI
    {
        struct CpuTimingStatistics;
    }

    namespace Render
    {
        struct ThreadRegionEntry 
        {
            AZStd::thread_id m_threadId;
            AZStd::sys_time_t m_startTick = 0;
            AZStd::sys_time_t m_endTick = 0;
        };

        //! Visual profiler for Cpu statistics.
        //! It uses ImGui as the library for displaying the Attachments and Heaps.
        //! It shows all heaps that are being used by the RHI and how the
        //! resources are allocated in each heap.
        class ImGuiCpuProfiler
        {
            // Region Name -> Array of ThreadRegion entries
            using RegionEntryMap = AZStd::map<AZStd::string, AZStd::vector<ThreadRegionEntry>>;
            // Group Name -> RegionEntryMap
            using GroupRegionMap = AZStd::map<AZStd::string, RegionEntryMap>;

        public:
            ImGuiCpuProfiler() = default;
            ~ImGuiCpuProfiler() = default;

            //! Draws the provided Cpu statistics.
            void Draw(bool& keepDrawing, const AZ::RHI::CpuTimingStatistics& cpuTimingStatistics);

        private:
            // Update the GroupRegionMap with the latest cached time regions
            void UpdateGroupRegionMap();

            // ImGui filter used to filter TimedRegions.
            ImGuiTextFilter m_timedRegionFilter;

            GroupRegionMap m_groupRegionMap;

            // Pause cpu profiling. The profiler will show the statistics of the last frame before pause
            bool m_paused = false;

            // Total frames need to be saved
            int m_captureFrameCount = 1;

            AZ::RHI::CpuTimingStatistics m_cpuTimingStatisticsWhenPause;

            AZStd::string m_lastCapturedFilePath;
        };
    } // namespace Render
}

#include "ImGuiCpuProfiler.inl"
