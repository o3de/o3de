/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/ProfilingCaptureBus.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/time.h>
#include <AzCore/std/containers/set.h>

namespace AZ
{
    namespace Render
    {
        namespace CpuProfilerImGuiHelper
        {
            // NOTE: Fix build error in case AZStd::thread_id is not of an arithmetic type, and instead a pointer
            template<typename ThreadId, typename AZStd::enable_if<AZStd::is_pointer<ThreadId>::value>::type* = nullptr>
            void TextThreadId(ThreadId threadId)
            {
                const AZStd::string threadIdText = AZStd::string::format("Thread: %p", threadId);
                ImGui::Text(threadIdText.c_str());
            }

            template<typename ThreadId, typename AZStd::enable_if<!AZStd::is_pointer<ThreadId>::value>::type* = nullptr>
            void TextThreadId(ThreadId threadId)
            {
                const AZStd::string threadIdText = AZStd::string::format("Thread: %zu", static_cast<size_t>(threadId));
                ImGui::Text(threadIdText.c_str());
            }
            inline float TicksToMs(AZStd::sys_time_t ticks)
            {
                // Note: converting to microseconds integer before converting to milliseconds float
                const AZStd::sys_time_t ticksPerSecond = AZStd::GetTimeTicksPerSecond();
                AZ_Assert(ticksPerSecond >= 1000, "Error in converting ticks to ms, expected ticksPerSecond >= 1000");
                return static_cast<float>((ticks * 1000) / (ticksPerSecond / 1000)) / 1000.0f;
            }
        }

        inline void ImGuiCpuProfiler::Draw(bool& keepDrawing, const AZ::RHI::CpuTimingStatistics& currentCpuTimingStatistics)
        {
            // Cache the value to detect if it was changed by ImGui(user pressed 'x')
            const bool cachedShowCpuProfiler = keepDrawing;

            const ImVec2 windowSize(640.0f, 480.0f);
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);
            bool captureToFile = false;
            if (ImGui::Begin("Cpu Profiler", &keepDrawing, ImGuiWindowFlags_None))
            {
                m_paused = !AZ::RHI::CpuProfiler::Get()->IsProfilerEnabled();
                if (ImGui::Button(m_paused ? "Resume" : "Pause"))
                {
                    m_paused = !m_paused;
                    AZ::RHI::CpuProfiler::Get()->SetProfilerEnabled(!m_paused);
                }

                // Update region map and cache the input cpu timing statistics when the profiling is not paused
                if (!m_paused)
                {
                    UpdateGroupRegionMap();
                    m_cpuTimingStatisticsWhenPause = currentCpuTimingStatistics;
                }

                if (ImGui::Button("Capture"))
                {
                    captureToFile = true;
                }

                if (!m_lastCapturedFilePath.empty())
                {
                    ImGui::SameLine();
                    ImGui::Text(m_lastCapturedFilePath.c_str());
                }

                const AZ::RHI::CpuTimingStatistics& cpuTimingStatistics = m_cpuTimingStatisticsWhenPause;

                const AZStd::sys_time_t ticksPerSecond = AZStd::GetTimeTicksPerSecond();

                const auto ShowTimeInMs = [ticksPerSecond](AZStd::sys_time_t duration)
                {
                    ImGui::Text("%.2f ms", CpuProfilerImGuiHelper::TicksToMs(duration));
                };

                const auto ShowRow = [ticksPerSecond, &ShowTimeInMs](const char* regionLabel, AZStd::sys_time_t duration)
                {
                    ImGui::Text(regionLabel);
                    ImGui::NextColumn();

                    ShowTimeInMs(duration);
                    ImGui::NextColumn();
                };

                const auto DrawRegionHoverMarker = [this, &ShowTimeInMs](AZStd::vector<ThreadRegionEntry>& entries)
                {
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 60.0f);

                        for (ThreadRegionEntry& entry : entries)
                        {
                            CpuProfilerImGuiHelper::TextThreadId(entry.m_threadId.m_id);

                            const AZStd::sys_time_t elapsed = entry.m_endTick - entry.m_startTick;
                            ShowTimeInMs(elapsed);
                            ImGui::Separator();
                        }

                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                };

                const auto ShowRegionRow = [ticksPerSecond, &DrawRegionHoverMarker, &ShowTimeInMs](const char* regionLabel,
                    AZStd::vector<ThreadRegionEntry> regions,
                    AZStd::sys_time_t duration)
                {
                    // Draw the region label
                    ImGui::Text(regionLabel);
                    ImGui::NextColumn();

                    // Draw the thread count label
                    AZStd::sys_time_t totalTime = 0;
                    AZStd::set<AZStd::thread_id> threads;
                    for (ThreadRegionEntry& entry : regions) // Find the thread count and total execution time for all threads
                    {
                        threads.insert(entry.m_threadId);
                        totalTime += entry.m_endTick - entry.m_startTick;
                    }
                    const AZStd::string threadLabel = AZStd::string::format("Threads: %u", static_cast<uint32_t>(threads.size()));
                    ImGui::Text(threadLabel.c_str());
                    DrawRegionHoverMarker(regions);
                    ImGui::NextColumn();

                    // Draw the overall invocation count
                    const AZStd::string invocationLabel = AZStd::string::format("Total calls: %u", static_cast<uint32_t>(regions.size()));
                    ImGui::Text(invocationLabel.c_str());
                    DrawRegionHoverMarker(regions);
                    ImGui::NextColumn();

                    // Draw the time labels (max and then total)
                    const AZStd::string timeLabel =
                        AZStd::string::format("%.2f ms max, %.2f ms total",
                        CpuProfilerImGuiHelper::TicksToMs(duration),
                        CpuProfilerImGuiHelper::TicksToMs(totalTime));
                    ImGui::Text(timeLabel.c_str());
                    ImGui::NextColumn();
                };

                // Set column settings.
                ImGui::Columns(2, "view", false);
                ImGui::SetColumnWidth(0, 660.0f);
                ImGui::SetColumnWidth(1, 100.0f);

                ShowRow("Frame to Frame Time", cpuTimingStatistics.m_frameToFrameTime);
                ShowRow("Present Time", cpuTimingStatistics.m_presentDuration);
                for (const auto& queueStatistics : cpuTimingStatistics.m_queueStatistics)
                {
                    ShowRow(queueStatistics.m_queueName.GetCStr(), queueStatistics.m_executeDuration);
                }

                ImGui::Separator();
                ImGui::Columns(1, "view", false);

                m_timedRegionFilter.Draw("TimedRegion Filter");

                // Draw the timed regions
                if (ImGui::BeginChild("TimedRegions"))
                {
                    for (auto& timeRegionMapEntry : m_groupRegionMap)
                    {
                        // Draw the regions
                        if (ImGui::TreeNodeEx(timeRegionMapEntry.first.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::Columns(4, "view", false);
                            ImGui::SetColumnWidth(0, 400.0f);
                            ImGui::SetColumnWidth(1, 100.0f);
                            ImGui::SetColumnWidth(2, 150.0f);
                            ImGui::SetColumnWidth(3, 240.0f);

                            for (auto& reigon : timeRegionMapEntry.second)
                            {
                                // Calculate the region with the longest execution time
                                AZStd::sys_time_t threadExecutionElapsed = 0;
                                for (ThreadRegionEntry& entry : reigon.second)
                                {
                                    const AZStd::sys_time_t elapsed = entry.m_endTick - entry.m_startTick;
                                    threadExecutionElapsed = AZStd::max(threadExecutionElapsed, elapsed);
                                }

                                // Only draw the TimedRegion rows when it passes the filter
                                if (m_timedRegionFilter.PassFilter(reigon.first.c_str()))
                                {
                                    ShowRegionRow(reigon.first.c_str(), reigon.second, threadExecutionElapsed);
                                }
                            }
                            ImGui::Columns(1, "view", false);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::EndChild();
                }
            }
            ImGui::End();

            if (captureToFile)
            {
                AZStd::sys_time_t timeNow = AZStd::GetTimeNowSecond();
                AZStd::string timeString;
                AZStd::to_string(timeString, timeNow);
                u64 currentTick = AZ::RPI::RPISystemInterface::Get()->GetCurrentTick();
                AZStd::string frameDataFilePath = AZStd::string::format("@user@/CpuProfiler/%s_%llu.json", timeString.c_str(), currentTick);
                char resolvedPath[AZ::IO::MaxPathLength];
                AZ::IO::FileIOBase::GetInstance()->ResolvePath(frameDataFilePath.c_str(), resolvedPath, AZ::IO::MaxPathLength);
                m_lastCapturedFilePath = resolvedPath;
                AZ::Render::ProfilingCaptureRequestBus::Broadcast(&AZ::Render::ProfilingCaptureRequestBus::Events::CaptureCpuProfilingStatistics,
                    frameDataFilePath);
            }

            // Toggle if the bool isn't the same as the cached value
            if (cachedShowCpuProfiler != keepDrawing)
            {
                AZ::RHI::CpuProfiler::Get()->SetProfilerEnabled(keepDrawing);
            }
        }

        inline void ImGuiCpuProfiler::UpdateGroupRegionMap()
        {
            // Clear the cached entries
            m_groupRegionMap.clear();

            // Get the latest TimeRegionMap
            const RHI::CpuProfiler::TimeRegionMap& timeRegionMap = RHI::CpuProfiler::Get()->GetTimeRegionMap();

            // Iterate through all the cached regions from all threads, and add the entries to this map
            for (auto& threadEntry : timeRegionMap)
            {
                for (auto& cachedRegionEntry : threadEntry.second)
                {
                    const AZStd::string& regionName = cachedRegionEntry.first;
                    for (auto& cachedRegion : cachedRegionEntry.second)
                    {
                        const AZStd::string& groupName = cachedRegion.m_groupRegionName->m_groupName;

                        m_groupRegionMap[groupName][regionName].push_back(
                            { threadEntry.first, cachedRegion.m_startTick, cachedRegion.m_endTick });
                    }
                }
            }
        }
    }
}
