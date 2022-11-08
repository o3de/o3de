/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/sort.h>

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/GpuQuery/GpuPassProfiler.h>


namespace AZ
{
    namespace RPI
    {
        ///////////////////////////////////////////////////////////////////////
        // --- PassEntry Start---
        GpuPassProfiler::PassEntry::PassEntry(const RPI::Pass* pass, GpuPassProfiler::PassEntry* parent)
        {
            m_name = pass->GetName();
            m_path = pass->GetPathName();
            m_parent = parent;
            m_enabled = pass->IsEnabled();
            m_timestampEnabled = pass->IsTimestampQueryEnabled();
            m_pipelineStatisticsEnabled = pass->IsPipelineStatisticsQueryEnabled();
            m_isParent = pass->AsParent() != nullptr;

            // [GFX TODO][ATOM-4001] Cache the timestamp and PipelineStatistics results.
            // Get the query results from the passes.
            m_timestampResult = pass->GetLatestTimestampResult();

            const RPI::PipelineStatisticsResult rps = pass->GetLatestPipelineStatisticsResult();
            m_pipelineStatistics = { rps.m_vertexCount, rps.m_primitiveCount, rps.m_vertexShaderInvocationCount,
                rps.m_rasterizedPrimitiveCount, rps.m_renderedPrimitiveCount, rps.m_pixelShaderInvocationCount, rps.m_computeShaderInvocationCount };

            // Disable the entry if it has a parent that is also not enabled.
            if (m_parent)
            {
                m_enabled = pass->IsEnabled() && m_parent->m_enabled;
            }
        }

        void GpuPassProfiler::PassEntry::LinkChild(PassEntry* childEntry)
        {
            m_children.push_back(childEntry);

            if (!m_linked && m_parent)
            {
                m_linked = true;

                // Recursively create parent->child references for entries that aren't linked to the root entry yet.
                // Effectively walking the tree backwards from the leaf to the root entry, and establishing parent->child references to
                // entries that aren't connected to the root entry yet.
                m_parent->LinkChild(this);
            }

            childEntry->m_linked = true;
        }

        bool GpuPassProfiler::PassEntry::IsTimestampEnabled() const
        {
            return m_enabled && m_timestampEnabled;
        }

        bool GpuPassProfiler::PassEntry::IsPipelineStatisticsEnabled() const
        {
            return m_enabled && m_pipelineStatisticsEnabled;
        }
        // --- PassEntry End ---
        ///////////////////////////////////////////////////////////////////////

        AZStd::unordered_map<Name, GpuPassProfiler::PassEntry> GpuPassProfiler::CreatePassEntriesDatabase(RHI::Ptr<RPI::ParentPass> rootPass)
        {
            AZStd::unordered_map<Name, GpuPassProfiler::PassEntry> passEntryDatabase;
            const auto addPassEntry = [&passEntryDatabase](const RPI::Pass* pass, GpuPassProfiler::PassEntry* parent) -> GpuPassProfiler::PassEntry*
            {
                // If parent a nullptr, it's assumed to be the rootpass.
                if (parent == nullptr)
                {
                    return &passEntryDatabase[pass->GetPathName()];
                }
                else
                {
                    GpuPassProfiler::PassEntry entry(pass, parent);

                    // Set the time stamp in the database.
                    [[maybe_unused]] const auto passEntry = passEntryDatabase.find(entry.m_path);
                    AZ_Assert(passEntry == passEntryDatabase.end(), "There already is an entry with the name \"%s\".", entry.m_path.GetCStr());

                    // Set the entry in the map.
                    GpuPassProfiler::PassEntry& entryRef = passEntryDatabase[entry.m_path] = entry;
                    return &entryRef;
                }
            };

            // NOTE: Write it all out, can't have recursive functions for lambdas.
            const AZStd::function<void(const RPI::Pass*, PassEntry*)> getPassEntryRecursive = [&addPassEntry, &getPassEntryRecursive](const RPI::Pass* pass, GpuPassProfiler::PassEntry* parent) -> void
            {
                const RPI::ParentPass* passAsParent = pass->AsParent();

                // Add new entry to the timestamp map.
                GpuPassProfiler::PassEntry* entry = addPassEntry(pass, parent);

                // Recur if it's a parent.
                if (passAsParent)
                {
                    for (const auto& childPass : passAsParent->GetChildren())
                    {
                        getPassEntryRecursive(childPass.get(), entry);
                    }
                }
            };

            // Set up the root entry.
            GpuPassProfiler::PassEntry rootEntry(static_cast<RPI::Pass*>(rootPass.get()), nullptr);
            passEntryDatabase[rootPass->GetPathName()] = rootEntry;

            // Create an intermediate structure from the passes.
            // Recursively create the timestamp entries tree.
            getPassEntryRecursive(static_cast<RPI::Pass*>(rootPass.get()), nullptr);

            // Interpolate the old values.
            const float lerpWeight = 0.2f;
            InterpolatePassEntries(passEntryDatabase, lerpWeight);

            return passEntryDatabase;
        }

        void GpuPassProfiler::InterpolatePassEntries(AZStd::unordered_map<Name, GpuPassProfiler::PassEntry>& passEntryDatabase, float weight) const
        {
            for (auto& entry : passEntryDatabase)
            {
                const auto oldEntryIt = passEntryDatabase.find(entry.second.m_path);
                if (oldEntryIt != passEntryDatabase.end())
                {
                    // Interpolate the timestamps.
                    const double interpolated = Lerp(static_cast<double>(oldEntryIt->second.m_interpolatedTimestampInNanoseconds),
                        static_cast<double>(entry.second.m_timestampResult.GetDurationInNanoseconds()),
                        static_cast<double>(weight));
                    entry.second.m_interpolatedTimestampInNanoseconds = static_cast<uint64_t>(interpolated);
                }
            }
        }

        AZStd::vector<GpuPassProfiler::PassEntry*> GpuPassProfiler::SortPassEntriesByTimestamps(AZStd::unordered_map<Name, PassEntry>& timestampEntryDatabase)
        {
            // pass entry grid based on its timestamp
            AZStd::vector<GpuPassProfiler::PassEntry*> sortedPassEntries;
            sortedPassEntries.reserve(timestampEntryDatabase.size());

            // Set the child of the parent, only if it passes the filter.
            for (auto& passEntryIt : timestampEntryDatabase)
            {
                PassEntry* passEntry = &passEntryIt.second;

                // Collect all pass entries with non-zero durations
                if (passEntry->m_timestampResult.GetDurationInTicks() > 0)
                {
                    sortedPassEntries.push_back(passEntry);
                }
            }

            // Sort the pass entries based on their starting time and duration
            AZStd::sort(sortedPassEntries.begin(), sortedPassEntries.end(), [](const PassEntry* passEntry1, const PassEntry* passEntry2) {
                if (passEntry1->m_timestampResult.GetTimestampBeginInTicks() == passEntry2->m_timestampResult.GetTimestampBeginInTicks())
                {
                    return passEntry1->m_timestampResult.GetDurationInTicks() < passEntry2->m_timestampResult.GetDurationInTicks();
                }
                return passEntry1->m_timestampResult.GetTimestampBeginInTicks() < passEntry2->m_timestampResult.GetTimestampBeginInTicks();
            });

            return sortedPassEntries;
        }


        uint64_t GpuPassProfiler::CalculateTotalGpuPassTime(const AZStd::vector<GpuPassProfiler::PassEntry*>& sortedPassEntries)
        {
            // calculate the total GPU duration.
            RPI::TimestampResult gpuTimestamp;
            if (sortedPassEntries.size() > 0)
            {
                gpuTimestamp = sortedPassEntries.front()->m_timestampResult;
                gpuTimestamp.Add(sortedPassEntries.back()->m_timestampResult);
            }

            return gpuTimestamp.GetDurationInNanoseconds();
        }

        uint64_t GpuPassProfiler::MeasureGpuTimeInNanoseconds(RHI::Ptr<RPI::ParentPass> rootPass)
        {
            if (m_measureGpuTime)
            {
                if (!rootPass->IsTimestampQueryEnabled())
                {
                    rootPass->SetTimestampQueryEnabled(true);
                }
            }
            else
            {
                if (rootPass->IsTimestampQueryEnabled())
                {
                    rootPass->SetTimestampQueryEnabled(false);
                }
                return 0;
            }

            // This would be the non-efficient way to measure GPU time per frame, but
            // it is what ImGuiGpuProfiler would need to do as it needs to show more detailed data.
            // If your FPS is at 300fps, running these three functions can make it drop to ~265fps.
            //auto passEntryDatabase = CreatePassEntriesDatabase(rootPass);
            //auto sortedPassEntries = SortPassEntriesByTimestamps(passEntryDatabase);
            //return CalculateTotalGpuPassTime(sortedPassEntries);

            AZ::RPI::TimestampResult resultBegin(AZStd::numeric_limits<uint64_t>::max(), AZStd::numeric_limits<uint64_t>::max(), RHI::HardwareQueueClass::Graphics);
            AZ::RPI::TimestampResult resultEnd;

            // NOTE: Write it all out, can't have recursive functions for lambdas.
            const AZStd::function<void(const RPI::Pass*)> calculateResultEndRecursive = [&resultBegin, &resultEnd, &calculateResultEndRecursive](const RPI::Pass* pass) -> void
            {
                const RPI::ParentPass* passAsParent = pass->AsParent();
                // Add new entry to the timestamp map.
                AZ::RPI::TimestampResult passTime = pass->GetLatestTimestampResult();

                if (passTime.GetDurationInTicks() > 0)
                {
                    const auto passBeginInTicks = passTime.GetTimestampBeginInTicks();
                    if (passBeginInTicks < resultBegin.GetTimestampBeginInTicks())
                    {
                        resultBegin = passTime;
                    }

                    if (resultEnd.GetTimestampBeginInTicks() == passBeginInTicks)
                    {
                        if (resultEnd.GetDurationInTicks() < passTime.GetDurationInTicks())
                        {
                            resultEnd = passTime;
                        }
                    }
                    else if (resultEnd.GetTimestampBeginInTicks() < passBeginInTicks)
                    {
                        resultEnd = passTime;
                    }
                }

                // Recur if it's a parent.
                if (passAsParent)
                {
                    for (const auto& childPass : passAsParent->GetChildren())
                    {
                        calculateResultEndRecursive(childPass.get());
                    }
                }
            };
            calculateResultEndRecursive(rootPass.get());

            if (resultBegin.GetTimestampBeginInTicks() >= resultEnd.GetTimestampBeginInTicks())
            {
                // Bogus data. This is normal for the first 3 frames.
                return 0;
            }

            // calculate the total GPU duration.
            resultBegin.Add(resultEnd);
            return resultBegin.GetDurationInNanoseconds();
        }

    }   // namespace RPI
}   // namespace AZ
