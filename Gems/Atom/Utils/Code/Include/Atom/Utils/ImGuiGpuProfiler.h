/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <imgui/imgui.h>
#include <Atom/RHI.Reflect/MemoryStatistics.h>
#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>
#include <Atom/RPI.Public/Pass/Pass.h>

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/string/string.h>

#include <Profiler/ImGuiTreemap.h>

namespace AZ
{
    namespace Render
    {
        //! Intermediate resource that represents the structure of a Pass within the FrameGraph.
        //! A tree structure will be created from these entries that mimics the pass' structure.
        //! By default, all entries have a parent<-child reference, but only entries that pass the filter will also hold a parent->child reference.
        struct PassEntry
        {
            //! Total number of attribute columns to draw the PipelineStatistics.
            static const uint32_t PipelineStatisticsAttributeCount = 7u;

            using PipelineStatisticsArray = AZStd::array<uint64_t, PipelineStatisticsAttributeCount>;

            PassEntry() = default;
            PassEntry(const class AZ::RPI::Pass* pass, PassEntry* parent);

            ~PassEntry() = default;

            //! Links the child TimestampEntry to the parent's, and sets the dirty flag for both the parent and child entry.
            //! Calling this method will effectively add a parent->child reference for this instance, and all parent entries leading up to this
            //! entry from the root entry.
            void LinkChild(PassEntry* childEntry);

            //! Propagate deviceIndex to parents
            void PropagateDeviceIndex(int deviceIndex);

            //! Checks if timestamp queries are enabled for this PassEntry.
            bool IsTimestampEnabled() const;
            //! Checks if PipelineStatistics queries are enabled for this PassEntry.
            bool IsPipelineStatisticsEnabled() const;

            //! The name of the pass.
            Name m_name;
            //! Cache the path name of the Pass as a unique identifier.
            Name m_path;

            RPI::TimestampResult m_timestampResult;
            uint64_t m_interpolatedTimestampInNanoseconds = 0u;

            //! Convert the PipelineStatistics result to an array for easier access.
            PipelineStatisticsArray m_pipelineStatistics;

            //! Used as a double linked structure to reference the parent <-> child relationship.
            PassEntry* m_parent = nullptr;
            AZStd::vector<PassEntry*> m_children;

            //! Mirrors the enabled queries state of the pass.
            bool m_timestampEnabled = false;
            bool m_pipelineStatisticsEnabled = false;

            //! Mirrors the enabled/disabled state of the pass.
            bool m_enabled = false;
            int m_deviceIndex = RHI::MultiDevice::DefaultDeviceIndex;
            AZStd::unordered_set<int> m_childrenDeviceIndices;

            //! Dirty flag to determine if this entry is linked to an parent entry.
            bool m_linked = false;

            //! Cache if the pass is a parent.
            bool m_isParent = false;
        };

        class ImGuiPipelineStatisticsView
        {
            // Sorting types for the PipelineStatistics.
            enum class StatisticsSortType : uint32_t
            {
                Alphabetical,
                Numerical,

                Count
            };

            // Sort options per variant.
            const uint32_t SortVariantPerColumn = 2u;

        public:
            ImGuiPipelineStatisticsView();

            //! Draw the PipelineStatistics window.
            void DrawPipelineStatisticsWindow(bool& draw, const PassEntry* rootPassEntry,
                AZStd::unordered_map<AZ::Name, PassEntry>& m_timestampEntryDatabase,
                AZ::RHI::Ptr<AZ::RPI::ParentPass> rootPass);

            //! Total number of columns (Attribute columns + PassName column).
            static const uint32_t HeaderAttributeCount = PassEntry::PipelineStatisticsAttributeCount + 1u;

        private:
            // Creates a row entry within the attribute matrix.
            void CreateAttributeRow(const PassEntry* passEntry, const PassEntry* rootEntry);

            // Sort the view depending on the selected attribute.
            void SortView();

            // Returns the column index to sort on (i.e 0 = PasName, 1 = Attribute0, etc).
            uint32_t GetSortIndex() const;
            // Returns whether it should be sorting alphabetically or numerically.
            StatisticsSortType GetSortType() const;
            // Returns whether the sorting is normal or inverted.
            bool IsSortStateInverted() const;

            // SortIndex is used to store the state in what order to list the PassEntries.
            // The matrix is only able to draw one sorting state at a time. Each column has two
            // states: normal and inverted. The column's sorting index is interleaved 
            // (e.g ColumnIndex 0: 0 = normal, 1 = inverted; ColumnIndex 1: 2 = normal, 3 = inverted, etc).
            uint32_t m_sortIndex = 0u;

            // Array of PassEntries that will be sorted depending on the sorting index.
            AZStd::vector<const PassEntry*> m_passEntryReferences;

            // Width of the columns.
            const float m_headerColumnWidth[HeaderAttributeCount];

            // Whether the color-coding is enabled.
            bool m_enableColorCoding = true;
            // Whether the exclusion filter is enabled.
            bool m_excludeFilterEnabled = false;
            // Show the contribution of a PassEntry attribute to the total in percentages.
            bool m_showAttributeContribution = false;
            // Show pass' tree state.
            bool m_showPassTreeState = false;
            // Show the disabled passes for the PipelineStatistics window.
            bool m_showDisabledPasses = true;
            // Show parent passes.
            bool m_showParentPasses = true;

            // ImGui filter used to filter passes by the user's input.
            ImGuiTextFilter m_passFilter;
                        
            // Pause and showing the pipeline statistics result when it's paused.
            bool m_paused = false;
        };

        class ImGuiTimestampView
        {
            // Metric unit in which the timestamp entries are represented.
            enum class TimestampMetricUnit : int32_t
            {
                Milliseconds = 0,
                Nanoseconds,
                Count
            };

            // Workload views.
            enum class FrameWorkloadView : int32_t
            {
                FpsView30 = 0,
                FpsView60,
                Count
            };

            // Sorting types for the flat view.
            enum class ProfilerSortType : uint32_t
            {
                Alphabetical = 0u,
                AlphabeticalInverse,
                AlphabeticalCount = AlphabeticalInverse - Alphabetical + 1u,
                Timestamp,
                TimestampInverse,
                TimestampCount = TimestampInverse - Timestamp + 1u,

                Count = AlphabeticalCount + TimestampCount,
            };

            // Timestamp structure views.
            enum class ProfilerViewType : int32_t
            {
                Hierarchical = 0,
                Flat,
                Count
            };

            // Timestamp refresh type .
            enum class RefreshType : int32_t
            {
                Realtime = 0,
                OncePerSecond,
                Count
            };

        public:
            //! Draw the Timestamp window.
            void DrawTimestampWindow(bool& draw, const PassEntry* rootPassEntry,
                AZStd::unordered_map<Name, PassEntry>& m_timestampEntryDatabase,
                AZ::RHI::Ptr<AZ::RPI::ParentPass> rootPass);

        private:
            // Draw option for the hierarchical view of the passes.
            // Recursively iterates through the timestamp entries, and creates an hierarchical structure.
            void DrawHierarchicalView(const PassEntry* entry, int deviceIndex) const;
            // Draw option for the flat view of the passes.
            void DrawFlatView(int deviceIndex) const;

            // Sorts the entries array depending on the sorting type.
            void SortFlatView();

            // Draws a single instance of the frame workload bar.
            void DrawFrameWorkloadBar(double value) const;
            // Converts nano- to milliseconds.
            double NanoToMilliseconds(uint64_t nanoseconds) const;
            // When the user clicks a sorting category, depending on the state of the profiler,
            // two things can happen:
            // - If the state changes, the sorting type will change.
            // - If the state stays the same, the order of sorting will change.
            void ToggleOrSwitchSortType(ProfilerSortType start, ProfilerSortType count);
            // Normalizes the timestamp parameter to a 30FPS (33 ms) or 60 FPS (16 ms) metric.
            double NormalizeFrameWorkload(uint64_t timestamp) const;
            // Formats the timestamp label depending on the time metric.
            AZStd::string FormatTimestampLabel(uint64_t timestamp) const;

            // Used to set the timestamp metric unit option.
            TimestampMetricUnit m_timestampMetricUnit = TimestampMetricUnit::Milliseconds;
            // Used to set the frame load option.
            FrameWorkloadView m_frameWorkloadView = FrameWorkloadView::FpsView30;
            // Used to set the sorting type option.
            ProfilerSortType m_sortType = ProfilerSortType::Timestamp;
            // Used to set the view option.
            ProfilerViewType m_viewType = ProfilerViewType::Hierarchical;

            // Array that will be sorted for the flat view.
            static const uint32_t TimestampEntryCount = 1024u;
            AZStd::fixed_vector<PassEntry*, TimestampEntryCount> m_passEntryReferences;

            // ImGui filter used to filter passes.
            ImGuiTextFilter m_passFilter;

            // Pause and showing the timestamp result when it's paused.
            bool m_paused = false;

            // Hide non-parent passes which has 0 execution time.
            bool m_hideZeroPasses = false;

            // Show pass execution timeline
            bool m_showTimeline = false;
            float m_timelineOffset{ 0.f };
            float m_timelineWindowWidth{ 1.f };

            // Controls how often the timestamp data is refreshed
            RefreshType m_refreshType = RefreshType::Realtime;
            AZStd::sys_time_t m_lastUpdateTimeMicroSecond = 0;

            AZStd::unordered_map<int, AZStd::pair<uint64_t, uint64_t>> m_lastCalibratedTimestamps;
            AZStd::unordered_map<int, AZStd::pair<uint64_t, uint64_t>> m_calibratedTimestamps;
        };

        class ImGuiGpuMemoryView
        {
        public:
            // Draw the overall GPU memory profiling window.
            void DrawGpuMemoryWindow(bool& draw);
            ImGuiGpuMemoryView();
            ~ImGuiGpuMemoryView();

        private:
            // Collate data from RHI and update memory view tables and treemap
            void PerformCapture();

            // Draw the heap usage pie chart
            void DrawPieChart(const AZ::RHI::MemoryStatistics::Heap& heap);

            // Update allocations and pools in the device and heap treemap widgets.
            void UpdateTreemaps();

            // Update the saved pointers in m_tableRows according to new data/filters
            void UpdateTableRows();

            void DrawTables();

            // Sort the table according to the appropriate column.
            void SortPoolTable(ImGuiTableSortSpecs* sortSpecs);
            void SortResourceTable(ImGuiTableSortSpecs* sortSpecs);

            // Save and load data to and from CSV/JSON files
            void SaveToJSON();
            void LoadFromJSON(const AZStd::string& fileName);
            void LoadFromCSV(const AZStd::string& fileName);

            struct PoolTableRow
            {
                Name m_poolName;

                bool m_deviceHeap = false;
                size_t m_budgetBytes = 0;
                size_t m_allocatedBytes = 0;
                size_t m_usedBytes = 0;
                float m_fragmentation = 0.f;
                size_t m_uniqueBytes = 0;
            };

            struct ResourceTableRow
            {
                Name m_parentPoolName;
                Name m_bufImgName;
                size_t m_sizeInBytes = 0;
                float m_fragmentation = 0.f;
                AZStd::string m_bindFlags;
            };

            // Table settings
            bool m_includeBuffers = true;
            bool m_includeImages = true;
            bool m_includeTransientAttachments = true;
            bool m_hideEmptyBufferPools = true;

            ImGuiTextFilter m_nameFilter;

            AZStd::vector<PoolTableRow> m_poolTableRows;
            AZStd::vector<ResourceTableRow> m_resourceTableRows;
            AZStd::vector<AZ::RHI::MemoryStatistics::Pool> m_savedPools;
            AZStd::vector<AZ::RHI::MemoryStatistics::Heap> m_savedHeaps;

            Profiler::ImGuiTreemap* m_hostTreemap = nullptr;
            Profiler::ImGuiTreemap* m_deviceTreemap = nullptr;
            bool m_showHostTreemap = false;
            bool m_showDeviceTreemap = false;

            AZStd::string m_memoryCapturePath;
            AZStd::string m_loadedCapturePath;
            AZStd::string m_captureMessage;
            char m_captureInput[AZ::IO::MaxPathLength] = { '\0' };
            size_t m_captureSelection = 0;
        };

        class ImGuiGpuProfiler
        {
        public:
            ImGuiGpuProfiler() = default;
            ~ImGuiGpuProfiler() = default;

            ImGuiGpuProfiler(ImGuiGpuProfiler&& other) = delete;
            ImGuiGpuProfiler(const ImGuiGpuProfiler& other) = delete;
            ImGuiGpuProfiler& operator=(const ImGuiGpuProfiler& other) = delete;

            //! Draws the ImGuiProfiler window.
            void Draw(bool& draw, RHI::Ptr<RPI::ParentPass> rootPass);

        private:
            // Interpolates the values of the PassEntries from the previous frame.
            void InterpolatePassEntries(AZStd::unordered_map<Name, PassEntry>& passEntryDatabase, float weight) const;

            // Create the PassEntries, and returns the root entry.
            PassEntry* CreatePassEntries(RHI::Ptr<RPI::ParentPass> rootPass);

            // Holds a PathName -> PassEntry reference for the PassEntries. 
            AZStd::unordered_map<Name, PassEntry> m_passEntryDatabase;

            bool m_drawTimestampView = false;
            bool m_drawPipelineStatisticsView = false;
            bool m_drawGpuMemoryView = false;

            ImGuiTimestampView m_timestampView;
            ImGuiPipelineStatisticsView m_pipelineStatisticsView;
            ImGuiGpuMemoryView m_gpuMemoryView;
        };
    } //namespace Render
} // namespace AZ

