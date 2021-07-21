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
#include <AzCore/std/containers/set.h>

#include <Atom/RHI.Reflect/CpuTimingStatistics.h>
#include <Atom/RHI/CpuProfiler.h>


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

        struct TableRow
        {
            void RecordRegion(const AZ::RHI::CachedTimeRegion& region, AZStd::thread_id threadId);
            double GetAverageInvocationsPerFrame() const;
            AZStd::string TableRow::GetExecutingThreadsLabel() const;
               
            AZStd::string m_groupName;
            AZStd::string m_regionName;
            AZStd::sys_time_t m_maxTicks;
            AZStd::sys_time_t m_runningAverageTicks;
            u64 m_invocations;

            AZStd::set<AZStd::thread_id> m_executingThreads;
        };

        //! Visual profiler for Cpu statistics.
        //! It uses ImGui as the library for displaying the Attachments and Heaps.
        //! It shows all heaps that are being used by the RHI and how the FIXME
        //! resources are allocated in each heap.
        class ImGuiCpuProfiler
            : SystemTickBus::Handler
        {
            friend struct TableRow;

            // Region Name -> Array of ThreadRegion entries
            using RegionEntryMap = AZStd::map<AZStd::string, TableRow>;
            // Group Name -> RegionEntryMap
            using GroupRegionMap = AZStd::map<AZStd::string, RegionEntryMap>;

            using TimeRegion = AZ::RHI::CachedTimeRegion;
            using GroupRegionName = AZ::RHI::CachedTimeRegion::GroupRegionName;

        public:
            ImGuiCpuProfiler() = default;
            ~ImGuiCpuProfiler() = default;

            //! Draws the overall CPU profiling window, defaults to the statistical view
            void Draw(bool& keepDrawing, const AZ::RHI::CpuTimingStatistics& cpuTimingStatistics);

            //! Draws the statistical view of the CPU profiling data
            void DrawStatisticsView();

            //! Draws the CPU profiling visualizer in a new window. 
            void DrawVisualizer();

        private:
            static constexpr float RowHeight = 50.0;
            static constexpr int DefaultFramesToCollect = 50;
            static constexpr float MediumFrameTimeLimit = 16.6; // 60 fps
            static constexpr float HighFrameTimeLimit = 33.3; // 30 fps

            static u64 ms_framesActive;

            // Draw the shared header between the two windows
            void DrawCommonHeader();

            // Draw the region statstics table in the order specified by the pointers in m_tableData
            void DrawTable();

            // Sort the table by a given column, rearranges the pointers in m_tableData
            void SortTable(ImGuiTableSortSpecs* sortSpecs);

            // ImGui filter used to filter TimedRegions.
            ImGuiTextFilter m_timedRegionFilter;

            // Saves statistical view data organized by group name -> region name -> row data
            GroupRegionMap m_groupRegionMap;

            // Saves pointers to objects in m_groupRegionMap, order reflects table ordering
            AZStd::vector<TableRow*> m_tableData;

            // Pause cpu profiling. The profiler will show the statistics of the last frame before pause
            bool m_paused = false;

            // Export the profiling data from a single frame to a local file
            bool m_captureToFile = false;

            // Toggle between the normal statistical view and the visual profiling view
            bool m_enableVisualizer = false;

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

            // Draw the vertical lines separating frames in the timeline
            void DrawFrameBoundaries();

            // Draw the ruler with frame time labels
            void DrawRuler();

            // Draw the frame time histogram 
            void DrawFrameTimeHistogram();

            // Converts raw ticks to a pixel value suitable to give to ImDrawList, handles window scrolling
            float ConvertTickToPixelSpace(AZStd::sys_time_t tick, AZStd::sys_time_t leftBound, AZStd::sys_time_t rightBound) const;

            AZStd::sys_time_t GetViewportTickWidth() const;

            // Gets the color for a block using the GroupRegionName as a key into the cache
            // Generates a random ImU32 if the block does not yet have a color
            ImU32 GetBlockColor(const TimeRegion& block);

            // System tick bus overrides
            virtual void OnSystemTick() override;

            // Visualizer state

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

            // Filter for highlighting regions on the visualizer
            ImGuiTextFilter m_visualizerHighlightFilter;
        };
    } // namespace Render
} // namespace AZ

#include "ImGuiCpuProfiler.inl"
