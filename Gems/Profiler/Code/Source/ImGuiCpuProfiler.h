/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(IMGUI_ENABLED)

#include <CpuProfiler.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>

#include <imgui/imgui.h>

namespace Profiler
{
    //! Stores all the data associated with a row in the table.
    struct TableRow
    {
        template <typename T>
        struct TableRowCompareFunctor
        {
            TableRowCompareFunctor(T memberPointer, bool isAscending) : m_memberPointer(memberPointer), m_ascending(isAscending){};

            bool operator()(const TableRow* lhs, const TableRow* rhs)
            {
                return m_ascending ? lhs->*m_memberPointer < rhs->*m_memberPointer : lhs->*m_memberPointer > rhs->*m_memberPointer;
            }

            T m_memberPointer;
            bool m_ascending;
        };

        // Update running statistics with new region data
        void RecordRegion(const CachedTimeRegion& region, size_t threadId);

        void ResetPerFrameStatistics();

        // Get a string of all threads that this region executed in during the last frame
        AZStd::string GetExecutingThreadsLabel() const;

        AZStd::string m_groupName;
        AZStd::string m_regionName;

        // --- Per frame statistics ---

        AZ::u64 m_invocationsLastFrame = 0;

        // NOTE: set over unordered_set so the threads can be shown in increasing order in tooltip.
        AZStd::set<size_t> m_executingThreads;

        AZStd::sys_time_t m_lastFrameTotalTicks = 0;

        // Maximum execution time of a region in the last frame.
        AZStd::sys_time_t m_maxTicks = 0;

        // --- Aggregate statistics ---

        AZ::u64 m_invocationsTotal = 0;

        // Running average of Mean Time Per Call
        AZStd::sys_time_t m_runningAverageTicks = 0;
    };

    //! ImGui widget for examining CPU Profiling instrumentation.
    //! Offers both a statistical view (with sorting and searching capability) and a visualizer
    //! similar to other profiling tools.
    class ImGuiCpuProfiler
        : public AZ::SystemTickBus::Handler
    {
        // Region Name -> statistical view row data
        using RegionRowMap = AZStd::map<AZStd::string, TableRow>;
        // Group Name -> RegionRowMap
        using GroupRegionMap = AZStd::map<AZStd::string, RegionRowMap>;

        using TimeRegion = CachedTimeRegion;
        using GroupRegionName = CachedTimeRegion::GroupRegionName;

    public:
        struct CpuTimingEntry
        {
            const AZStd::string& m_name;
            double m_executeDuration;
        };

        ImGuiCpuProfiler() = default;
        ~ImGuiCpuProfiler() = default;

        //! Draws the overall CPU profiling window, defaults to the statistical view
        void Draw(bool& keepDrawing);

    private:
        static constexpr float RowHeight = 35.0f;
        static constexpr int DefaultFramesToCollect = 50;
        static constexpr float MediumFrameTimeLimit = 16.6f; // 60 fps
        static constexpr float HighFrameTimeLimit = 33.3f; // 30 fps

        //! Draws the statistical view of the CPU profiling data.
        void DrawStatisticsView();

        //! Generates the full output timestamped file path based on nameHint
        AZStd::string GenerateOutputFile(const char* nameHint);

        //! Callback invoked when the "Load File" button is pressed in the file picker.
        void LoadFile();

        //! Draws the file picker window.
        void DrawFilePicker();

        //! Draws the CPU profiling visualizer.
        void DrawVisualizer();

        // Draw the shared header between the two windows.
        void DrawCommonHeader();

        // Draw the region statistics table in the order specified by the pointers in m_tableData.
        void DrawTable();

        // Sort the table by a given column, rearranges the pointers in m_tableData.
        void SortTable(ImGuiTableSortSpecs* sortSpecs);

        // gather the latest timing statistics
        void CacheCpuTimingStatistics();

        // Get the profiling data from the last frame, only called when the profiler is not paused.
        void CollectFrameData();

        // Cull old data from internal storage, only called when profiler is not paused.
        void CullFrameData();

        // Draws a single block onto the timeline into the specified row
        void DrawBlock(const TimeRegion& block, AZ::u64 targetRow);

        // Draw horizontal lines between threads in the timeline
        void DrawThreadSeparator(AZ::u64 threadBoundary, AZ::u64 maxDepth);

        // Draw the "Thread XXXXX" label onto the viewport
        void DrawThreadLabel(AZ::u64 baseRow, size_t threadId);

        // Draw the vertical lines separating frames in the timeline
        void DrawFrameBoundaries();

        // Draw the ruler with frame time labels
        void DrawRuler();

        // Draw the frame time histogram
        void DrawFrameTimeHistogram();

        // Converts raw ticks to a pixel value suitable to give to ImDrawList, handles window scrolling
        float ConvertTickToPixelSpace(AZStd::sys_time_t tick, AZStd::sys_time_t leftBound, AZStd::sys_time_t rightBound) const;

        AZStd::sys_time_t GetViewportTickWidth() const;

        // Gets the color for a block using the GroupRegionName as a key into the cache.
        // Generates a random ImU32 if the block does not yet have a color.
        ImU32 GetBlockColor(const TimeRegion& block);

        // System tick bus overrides
        void OnSystemTick() override;

        //  --- Visualizer Members ---

        int m_framesToCollect = DefaultFramesToCollect;

        // Tally of the number of saved profiling events so far
        AZ::u64 m_savedRegionCount = 0;

        // Viewport tick bounds, these are used to convert tick space -> screen space and cull so we only draw onscreen objects
        AZStd::sys_time_t m_viewportStartTick;
        AZStd::sys_time_t m_viewportEndTick;

        // Map to store each thread's TimeRegions, individual vectors are sorted by start tick
        // note: we use size_t as a proxy for thread_id because native_thread_id_type differs differs from
        // platform to platform, which causes problems when deserializing saved captures.
        AZStd::unordered_map<size_t, AZStd::vector<TimeRegion>> m_savedData;

        // Region color cache
        AZStd::unordered_map<GroupRegionName, ImVec4, CachedTimeRegion::GroupRegionName::Hash> m_regionColorMap;

        // Tracks the frame boundaries
        AZStd::vector<AZStd::sys_time_t> m_frameEndTicks = { INT64_MIN };

        // Filter for highlighting regions on the visualizer
        ImGuiTextFilter m_visualizerHighlightFilter;

        // --- Tabular view members ---

        // ImGui filter used to filter TimedRegions.
        ImGuiTextFilter m_timedRegionFilter;

        // Saves statistical view data organized by group name -> region name -> row data
        GroupRegionMap m_groupRegionMap;

        // Saves pointers to objects in m_groupRegionMap, order reflects table ordering.
        // Non-owning, will be cleared when m_groupRegionMap is cleared.
        AZStd::vector<TableRow*> m_tableData;

        // Pause cpu profiling. The profiler will show the statistics of the last frame before pause.
        bool m_paused = false;

        // Export the profiling data from a single frame to a local file.
        bool m_captureToFile = false;

        // Toggle between the normal statistical view and the visual profiling view.
        bool m_enableVisualizer = false;

        // Last captured CPU timing statistics
        AZStd::vector<CpuTimingEntry> m_cpuTimingStatisticsWhenPause;
        AZStd::sys_time_t m_frameToFrameTime{};

        AZ::IO::FixedMaxPath m_lastCapturedFilePath;

        bool m_showFilePicker = false;

        // Cached file paths to previous traces on disk, sorted with the most recent trace at the front.
        AZStd::vector<AZ::IO::Path> m_cachedCapturePaths;

        // Index into the file picker, used to determine which file to load when "Load File" is pressed.
        int m_currentFileIndex = 0;


        // --- Loading capture state ---
        AZStd::unordered_set<AZStd::string> m_deserializedStringPool;
        AZStd::unordered_set<CachedTimeRegion::GroupRegionName, CachedTimeRegion::GroupRegionName::Hash> m_deserializedGroupRegionNamePool;
    };
} // namespace Profiler

#endif // defined(IMGUI_ENABLED)
