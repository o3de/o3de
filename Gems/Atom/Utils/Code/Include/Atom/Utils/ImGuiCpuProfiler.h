/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Random.h>

#include <Atom/RHI.Reflect/CpuTimingStatistics.h>
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

        // Stores data about a region that is agreggated from all collected frames
        // Data collection can be toggled on and off through m_record. 
        struct RegionStatistics
        {
            float CalcAverageTimeMs() const;
            void RecordRegion(const AZ::RHI::CachedTimeRegion& region);

            bool m_draw = false;
            bool m_record = true;
            u64 m_invocations = 0;
            AZStd::sys_time_t m_totalTicks = 0;
        };

        //! Visual profiler for Cpu statistics.
        //! It uses ImGui as the library for displaying the Attachments and Heaps.
        //! It shows all heaps that are being used by the RHI and how the
        //! resources are allocated in each heap.
        class ImGuiCpuProfiler
            : SystemTickBus::Handler
        {
            // Region Name -> Array of ThreadRegion entries
            using RegionEntryMap = AZStd::map<AZStd::string, AZStd::vector<ThreadRegionEntry>>;
            // Group Name -> RegionEntryMap
            using GroupRegionMap = AZStd::map<AZStd::string, RegionEntryMap>;

            using TimeRegion = AZ::RHI::CachedTimeRegion;
            using GroupRegionName = AZ::RHI::CachedTimeRegion::GroupRegionName;

        public:
            ImGuiCpuProfiler() = default;
            ~ImGuiCpuProfiler() = default;

            //! Draws the provided Cpu statistics.
            void Draw(bool& keepDrawing, const AZ::RHI::CpuTimingStatistics& cpuTimingStatistics);

            //! Draws the CPU profiling visualizer in a new window. 
            void DrawVisualizer(bool& keepDrawing, const AZ::RHI::CpuTimingStatistics& currentCpuTimingStatistics);

        private:
            static constexpr float RowHeight = 50.0;
            static constexpr int DefaultFramesToCollect = 50;

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

            // Visualizer methods

            // Get the profiling data from the last frame, only called when the profiler is not paused.
            void CollectFrameData();

            // Cull old data from internal storage, only called when profiler is not paused.
            void CullFrameData(const AZ::RHI::CpuTimingStatistics& currentCpuTimingStatistics);

            // Draws a single block onto the timeline
            void DrawBlock(const TimeRegion& block, u64 targetRow);

            // Draw horizontal lines between threads in the timeline
            void DrawThreadSeparator(u64 threadBoundary, u64 maxDepth);

            // Draw the "Thread XXXXX" label onto the viewport
            void DrawThreadLabel(u64 baseRow, AZStd::thread_id threadId);

            // Draws all active function statistics windows
            void DrawRegionStatistics();

            // Draw the vertical lines separating frames in the timeline
            void DrawFrameBoundaries();

            // Draw the ruler with frame time labels
            void DrawRuler();

            // Converts raw ticks to a pixel value suitable to give to ImDrawList, handles window scrolling
            float ConvertTickToPixelSpace(AZStd::sys_time_t tick) const;

            AZStd::sys_time_t GetViewportTickWidth() const;

            // Gets the color for a block using the GroupRegionName as a key into the cache
            // Generates a random ImU32 if the block does not yet have a color
            ImU32 GetBlockColor(const TimeRegion& block);

            // System tick bus overrides
            virtual void OnSystemTick() override;

            // Visualizer state

            bool m_showVisualizer = false;

            int m_framesToCollect = DefaultFramesToCollect;

            // Tally of the number of saved profiling events so far
            u64 m_savedRegionCount = 0;

            // Viewport tick bounds, these are used to convert tick space -> screen space and cull so we only draw onscreen objects
            AZStd::sys_time_t m_viewportStartTick;
            AZStd::sys_time_t m_viewportEndTick;

            // Map to store each thread's TimeRegions, individual vectors are sorted by start tick
            AZStd::unordered_map<AZStd::thread_id, AZStd::vector<TimeRegion>> m_savedData;

            // Region color cache
            AZStd::unordered_map<const GroupRegionName*, ImVec4> m_regionColorMap;

            // Tracks the frame boundaries
            AZStd::vector<AZStd::sys_time_t> m_frameEndTicks = { INT64_MIN };

            // Main data structure for storing function statistics to be shown in the popup windows.
            // For now we default allocate for all regions on the first render frame and then use RegionStatistics.m_draw to determine
            // if we should draw the window or not. FIXME(ATOM-15948) this should be changed once RegionStatistics gets heavier. 
            AZStd::unordered_map<const GroupRegionName*, RegionStatistics> m_regionStatisticsMap;
        };
    } // namespace Render
} // namespace AZ

#include "ImGuiCpuProfiler.inl"
