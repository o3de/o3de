/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/array.h>
#include <AzCore/Name/Name.h>

#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>

namespace AZ
{
    namespace RPI
    {
        class Pass;
        class ParentPass;

        //! This is a helper class that can be used to measure, per frame, the time it takes
        //! for the render pipeline to execute in the GPU from the first to the last pass.
        //! The core functionality is provided by the function MeasureRenderPipelineGpuTimeInNanoseconds(),
        //! but other functions are available for tools who need to report more details, like time spent at each pass, etc.
        //! REMARK: Use judiciously, as MeasureRenderPipelineGpuTimeInNanoseconds() can itself affect performance when called.
        //! For example, if a scene is running at 300fps, calling MeasureRenderPipelineGpuTimeInNanoseconds(), each frame, can reduce the FPS to
        //! ~265fps.
        class GpuPassProfiler final
        {
        public:

            //! Intermediate data that represents the structure of a Pass within the FrameGraph.
            //! A tree structure will be created from these entries that mimics the pass' structure.
            //! By default, all entries have a parent<-child reference, but only entries that pass the filter will also hold a parent->child reference.
            struct PassEntry
            {
                //! Total number of attributes collected per  PipelineStatistics.
                static const uint32_t PipelineStatisticsAttributeCount = 7u;

                using PipelineStatisticsArray = AZStd::array<uint64_t, PipelineStatisticsAttributeCount>;

                PassEntry() = default;
                PassEntry(const class AZ::RPI::Pass* pass, PassEntry* parent);

                ~PassEntry() = default;

                //! Links the child TimestampEntry to the parent's, and sets the dirty flag for both the parent and child entry.
                //! Calling this method will effectively add a parent->child reference for this instance, and all parent entries leading up to this
                //! entry from the root entry.
                void LinkChild(PassEntry* childEntry);

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

                //! Dirty flag to determine if this entry is linked to an parent entry.
                bool m_linked = false;

                //! Cache if the pass is a parent.
                bool m_isParent = false;
            };

            GpuPassProfiler() = default;
            ~GpuPassProfiler() = default;

            GpuPassProfiler(GpuPassProfiler&& other) = delete;
            GpuPassProfiler(const GpuPassProfiler& other) = delete;
            GpuPassProfiler& operator=(const GpuPassProfiler& other) = delete;

            void SetRenderPipelineGpuTimeMeasurementEnabled(bool enabled) { m_measureRenderPipelineGpuTime = enabled; }
            bool IsRenderPipelineGpuTimeMeasurementEnabled() { return m_measureRenderPipelineGpuTime; }

            // Measures the total time spent by the Render Pipeline inside the GPU.
            // It measures the time by using the functions CreatePassEntriesDatabase(), SortPassEntriesByTimestamps() & CalculateTotalGpuPassTime().
            // The above functions are public and exist in split form to faciliate integration in tools like the ImGuiGpuProfiler, etc.
            // if @m_measureRenderPipelineGpuTime is false this function returns 0.
            uint64_t MeasureRenderPipelineGpuTimeInNanoseconds(RHI::Ptr<RPI::ParentPass> rootPass);

            //////////////////////////////////////////////////////////////////
            // Support Functions START
            // The following functions provide all the details when measuring
            // the render pipeline performance.
            
            // Returns the pass entry database where the key is PathName -> value is PassEntry
            AZStd::unordered_map<Name, PassEntry> CreatePassEntriesDatabase(RHI::Ptr<RPI::ParentPass> rootPass);

            // Measures the time spent in the GPU for each pass.
            // Returns the total amount of nanoseconds spent in the GPU for the Root Parent Pass.
            AZStd::vector<GpuPassProfiler::PassEntry*> SortPassEntriesByTimestamps(AZStd::unordered_map<Name, PassEntry>& timestampEntryDatabase);

            // Returns the total amount spent in the GPU by the root pass. Assumes sortedPassEntries
            // is sorted by timestamp.
            uint64_t CalculateTotalGpuPassTime(const AZStd::vector<GpuPassProfiler::PassEntry*>& sortedPassEntries);

            // Support Functions END
            ///////////////////////////////////////////////////////////////////


        private:
            // Interpolates the values of the PassEntries from the previous frame.
            void InterpolatePassEntries(AZStd::unordered_map<Name, PassEntry>& passEntryDatabase, float weight) const;

            bool m_measureRenderPipelineGpuTime = false;
 
        };

    }   // namespace RPI
}   // namespace AZ
