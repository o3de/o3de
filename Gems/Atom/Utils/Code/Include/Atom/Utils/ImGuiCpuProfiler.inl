/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/ProfilingCaptureBus.h>
#include <Atom/RHI.Reflect/CpuTimingStatistics.h>
#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/time.h>
#include "../../../Gems/ImGui/External/ImGui/v1.82/imgui/imgui.h"


namespace AZ
{
    namespace Render
    {
        namespace CpuProfilerImGuiHelper
        {
            // NOTE: Fix build error in case AZStd::thread_id is not of an arithmetic type, and instead a pointer
            template<typename ThreadId, typename AZStd::enable_if<AZStd::is_pointer<ThreadId>::value>::type* = nullptr>
            AZStd::string TextThreadId(ThreadId threadId)
            {
                return AZStd::string::format("Thread: %p", threadId);
            }

            template<typename ThreadId, typename AZStd::enable_if<!AZStd::is_pointer<ThreadId>::value>::type* = nullptr>
            AZStd::string TextThreadId(ThreadId threadId)
            {
                return AZStd::string::format("Thread: %zu", static_cast<size_t>(threadId));
            }
            inline float TicksToMs(AZStd::sys_time_t ticks)
            {
                // Note: converting to microseconds integer before converting to milliseconds float
                const AZStd::sys_time_t ticksPerSecond = AZStd::GetTimeTicksPerSecond();
                AZ_Assert(ticksPerSecond >= 1000, "Error in converting ticks to ms, expected ticksPerSecond >= 1000");
                return static_cast<float>((ticks * 1000) / (ticksPerSecond / 1000)) / 1000.0f;
            }
        } // namespace CpuProfilerImGuiHelper

        inline void ImGuiCpuProfiler::Draw(bool& keepDrawing, const AZ::RHI::CpuTimingStatistics& currentCpuTimingStatistics)
        {
            // Cache the value to detect if it was changed by ImGui(user pressed 'x')
            const bool cachedShowCpuProfiler = keepDrawing;

            const ImVec2 windowSize(900.0f, 600.0f);
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);
            if (ImGui::Begin("CPU Profiler", &keepDrawing, ImGuiWindowFlags_None))
            {
                // Collect the last frame's profiling data
                if (!m_paused)
                {
                    // Update region map and cache the input cpu timing statistics when the profiling is not paused
                    m_cpuTimingStatisticsWhenPause = currentCpuTimingStatistics;

                    CollectFrameData();
                    CullFrameData(currentCpuTimingStatistics); 

                    // Only listen to system ticks when the profiler is active
                    if (!SystemTickBus::Handler::BusIsConnected())
                    {
                        SystemTickBus::Handler::BusConnect();
                    }
                }

                if (m_enableVisualizer)
                {
                    DrawVisualizer();
                }
                else
                {
                    DrawStatisticsView();
                }
            }
            ImGui::End();
         
            if (m_captureToFile)
            {
                AZStd::sys_time_t timeNow = AZStd::GetTimeNowSecond();
                AZStd::string timeString;
                AZStd::to_string(timeString, timeNow);
                u64 currentTick = AZ::RPI::RPISystemInterface::Get()->GetCurrentTick();
                const AZStd::string frameDataFilePath = AZStd::string::format(
                    "@user@/CpuProfiler/%s_%llu.json",
                    timeString.c_str(),
                    currentTick);
                char resolvedPath[AZ::IO::MaxPathLength];
                AZ::IO::FileIOBase::GetInstance()->ResolvePath(frameDataFilePath.c_str(), resolvedPath, AZ::IO::MaxPathLength);
                m_lastCapturedFilePath = resolvedPath;
                AZ::Render::ProfilingCaptureRequestBus::Broadcast(
                    &AZ::Render::ProfilingCaptureRequestBus::Events::CaptureCpuProfilingStatistics, frameDataFilePath);
            }
            m_captureToFile = false;

            // Toggle if the bool isn't the same as the cached value
            if (cachedShowCpuProfiler != keepDrawing)
            {
                AZ::RHI::CpuProfiler::Get()->SetProfilerEnabled(keepDrawing);
            }
        }

        inline void ImGuiCpuProfiler::DrawCommonHeader()
        {
            if (ImGui::Button(m_enableVisualizer ? "Swap to statistics" : "Swap to visualizer"))
            {
                m_enableVisualizer = !m_enableVisualizer;
            }

            ImGui::SameLine();
            m_paused = !AZ::RHI::CpuProfiler::Get()->IsProfilerEnabled();
            if (ImGui::Button(m_paused ? "Resume" : "Pause"))
            {
                m_paused = !m_paused;
                AZ::RHI::CpuProfiler::Get()->SetProfilerEnabled(!m_paused);
            }

            ImGui::SameLine();
            if (ImGui::Button("Capture"))
            {
                m_captureToFile = true;
            }

            if (!m_lastCapturedFilePath.empty())
            {
                ImGui::SameLine();
                ImGui::Text("Saved: %s", m_lastCapturedFilePath.c_str());
            }
        }

        inline void ImGuiCpuProfiler::DrawStatisticsView()
        {
            DrawCommonHeader();

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
                        ImGui::Text(CpuProfilerImGuiHelper::TextThreadId(entry.m_threadId.m_id).c_str());

                        const AZStd::sys_time_t elapsed = entry.m_endTick - entry.m_startTick;
                        ShowTimeInMs(elapsed);
                        ImGui::Separator();
                    }

                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            };

            const auto ShowRegionRow =
                [ticksPerSecond, &DrawRegionHoverMarker,
                 &ShowTimeInMs](const char* regionLabel, AZStd::vector<ThreadRegionEntry> regions, AZStd::sys_time_t duration)
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
                const AZStd::string timeLabel = AZStd::string::format(
                    "%.2f ms max, %.2f ms total", CpuProfilerImGuiHelper::TicksToMs(duration),
                    CpuProfilerImGuiHelper::TicksToMs(totalTime));
                ImGui::Text(timeLabel.c_str());
                ImGui::NextColumn();
            };

            if (ImGui::BeginChild("Statistics View", { 0, 0 }, true))
            {
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

                            for (auto& region : timeRegionMapEntry.second)
                            {
                                // Calculate the region with the longest execution time
                                AZStd::sys_time_t threadExecutionElapsed = 0;
                                for (ThreadRegionEntry& entry : region.second)
                                {
                                    const AZStd::sys_time_t elapsed = entry.m_endTick - entry.m_startTick;
                                    threadExecutionElapsed = AZStd::max(threadExecutionElapsed, elapsed);
                                }

                                // Only draw the TimedRegion rows when it passes the filter
                                if (m_timedRegionFilter.PassFilter(region.first.c_str()))
                                {
                                    ShowRegionRow(region.first.c_str(), region.second, threadExecutionElapsed);
                                }
                            }
                            ImGui::Columns(1, "view", false);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::EndChild();
                }
            } 
        }

        // -- CPU Visualizer --
        inline void ImGuiCpuProfiler::DrawVisualizer()
        {
            DrawCommonHeader();

            // Options & Statistics
            if (ImGui::BeginChild("Options and Statistics", { 0, 0 }, true))
            {
                ImGui::Columns(3, "Options", true);
                ImGui::Text("Frames To Collect:");
                ImGui::SliderInt("", &m_framesToCollect, 10, 10000, "%d", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);

                ImGui::NextColumn();

                ImGui::Text("Viewport width: %.3f ms", CpuProfilerImGuiHelper::TicksToMs(GetViewportTickWidth()));
                ImGui::Text("Ticks [%lld , %lld]", m_viewportStartTick, m_viewportEndTick);
                ImGui::Text("Recording %ld threads", RHI::CpuProfiler::Get()->GetTimeRegionMap().size());
                ImGui::Text("%llu profiling events saved", m_savedRegionCount);

                ImGui::NextColumn();

                ImGui::TextWrapped(
                    "Hold the right mouse button to move around. Zoom by scrolling the mouse wheel while holding <ctrl>.");
            }

            ImGui::Columns(1, "FrameTimeColumn", true);

            if (ImGui::BeginChild("FrameTimeHistogram", { 0, 50 }, true))
            {
                DrawFrameTimeHistogram();
            }
            ImGui::EndChild();

            ImGui::Columns(1, "RulerColumn", true);

            // Ruler
            if (ImGui::BeginChild("Ruler", { 0, 30 }, true, ImGuiWindowFlags_NoNavFocus))
            {
                DrawRuler();
            }
            ImGui::EndChild();


            ImGui::Columns(1, "TimelineColumn", true);

            // Timeline
            if (ImGui::BeginChild(
                    "Timeline", { 0, 0 }, true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
            {
                // Find the next frame boundary after the viewport's right bound and draw until that tick
                auto nextFrameBoundaryItr = AZStd::lower_bound(m_frameEndTicks.begin(), m_frameEndTicks.end(), m_viewportEndTick);
                if (nextFrameBoundaryItr == m_frameEndTicks.end() && m_frameEndTicks.size() != 0)
                {
                    --nextFrameBoundaryItr;
                }
                const AZStd::sys_time_t nextFrameBoundary = *nextFrameBoundaryItr;

                // Find the start tick of the leftmost frame, which may be offscreen.
                auto startTickItr = AZStd::lower_bound(m_frameEndTicks.begin(), m_frameEndTicks.end(), m_viewportStartTick);
                if (startTickItr != m_frameEndTicks.begin())
                {
                    --startTickItr;
                }

                // Main draw loop
                u64 baseRow = 0;
                for (const auto& [currentThreadId, singleThreadData] : m_savedData)
                {
                    // Find the first TimeRegion that we should draw
                    auto regionItr = AZStd::lower_bound(
                        singleThreadData.begin(), singleThreadData.end(), *startTickItr,
                        [](const TimeRegion& wrapper, AZStd::sys_time_t target)
                        {
                            return wrapper.m_startTick < target;
                        });

                    // Draw all of the blocks for a given thread/row
                    u64 maxDepth = 0;
                    while (regionItr != singleThreadData.end())
                    {
                        const TimeRegion& region = *regionItr;

                        // Early out if we have drawn all the onscreen regions
                        if (region.m_startTick > nextFrameBoundary)
                        {
                            break;
                        }
                        u64 targetRow = region.m_stackDepth + baseRow;
                        maxDepth = AZStd::max(aznumeric_cast<u64>(region.m_stackDepth), maxDepth);

                        DrawBlock(region, targetRow);

                        ++regionItr;
                    }

                    // Draw UI details
                    DrawThreadLabel(baseRow, currentThreadId);
                    DrawThreadSeparator(baseRow, maxDepth);

                    baseRow += maxDepth + 1; // Next draw loop should start one row down
                }

                DrawRegionStatistics();
                DrawFrameBoundaries();

                // Draw an invisible button to capture inputs
                ImGui::InvisibleButton("Timeline Input", { ImGui::GetWindowContentRegionWidth(), baseRow * RowHeight });

                // Controls
                ImGuiIO& io = ImGui::GetIO();
                if (ImGui::IsWindowFocused() && ImGui::IsItemHovered())
                {
                    io.WantCaptureMouse = true;
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) // Scrolling
                    {
                        const auto [deltaX, deltaY] = io.MouseDelta;
                        if (deltaX != 0 || deltaY != 0)
                        {
                            // We want to maintain uniformity in scrolling (a click and drag should leave the cursor at the same spot
                            // relative to the objects on screen)
                            const float pixelDeltaNormalized = deltaX / ImGui::GetWindowWidth();
                            auto tickDelta = aznumeric_cast<AZStd::sys_time_t>(-1 * pixelDeltaNormalized * GetViewportTickWidth());
                            m_viewportStartTick += tickDelta;
                            m_viewportEndTick += tickDelta;

                            ImGui::SetScrollY(ImGui::GetScrollY() + deltaY * -1);
                        }
                    }
                    else if (io.MouseWheel != 0 && io.KeyCtrl) // Zooming
                    {
                        // We want zooming to be relative to the mouse's current position
                        const float mouseVel = io.MouseWheel;
                        const float mouseX = ImGui::GetMousePos().x;

                        // Find the normalized position of the cursor relative to the window
                        const float percentWindow = (mouseX - ImGui::GetWindowPos().x) / ImGui::GetWindowWidth();

                        const auto overallTickDelta = aznumeric_cast<AZStd::sys_time_t>(0.05 * io.MouseWheel * GetViewportTickWidth());

                        // Split the overall delta between the two bounds depending on mouse pos
                        const auto newStartTick = m_viewportStartTick + aznumeric_cast<AZStd::sys_time_t>(percentWindow * overallTickDelta);
                        const auto newEndTick = m_viewportEndTick - aznumeric_cast<AZStd::sys_time_t>((1-percentWindow) * overallTickDelta);

                        // Avoid zooming too much, start tick should always be less than end tick
                        if (newStartTick < newEndTick)
                        {
                            m_viewportStartTick = newStartTick;
                            m_viewportEndTick = newEndTick;
                        }
                    }
                }
            }
            ImGui::EndChild();
        }

        inline void ImGuiCpuProfiler::CollectFrameData()
        {
            // We maintain separate datastores for the visualizer and the statistical view because they require different
            // data formats - one grouped by thread ID versus the other organized by group + region. Since the statistical
            // view is only holding data from the last frame, the memory overhead is minimal and gives us a faster redraw
            // compared to if we needed to transform the visualizer's data into the statistical format every frame.

            // Clear the statistical view's cached entries
            m_groupRegionMap.clear();

            // Get the latest TimeRegionMap
            const RHI::CpuProfiler::TimeRegionMap& timeRegionMap = RHI::CpuProfiler::Get()->GetTimeRegionMap();

            m_viewportStartTick = INT64_MAX;
            m_viewportEndTick = INT64_MIN;

            // Iterate through the entire TimeRegionMap and copy the data since it will get deleted on the next frame
            for (const auto& [threadId, singleThreadRegionMap] : timeRegionMap)
            {
                // The profiler can sometime return threads without any profiling events when dropping threads, FIXME(ATOM-15949)
                if (singleThreadRegionMap.size() == 0)
                {
                    continue;
                }

                // Now focus on just the data for the current thread
                AZStd::vector<TimeRegion> newVisualizerData;
                newVisualizerData.reserve(singleThreadRegionMap.size()); // Avoids reallocation in the normal case when each region only has one invocation
                for (const auto& [regionName, regionVec] : singleThreadRegionMap)
                {
                    for (const TimeRegion& region : regionVec)
                    {
                        newVisualizerData.push_back(region); // Copies

                        // Also update the statistical view's data
                        const AZStd::string& groupName = region.m_groupRegionName->m_groupName;
                        m_groupRegionMap[groupName][regionName].push_back(
                            { threadId, region.m_startTick, region.m_endTick });

                        // Update running statistics if we want to record this region's data
                        if (m_regionStatisticsMap[region.m_groupRegionName].m_record)
                        {
                            m_regionStatisticsMap[region.m_groupRegionName].RecordRegion(region);
                        }
                    }
                }

                // Sorting by start tick allows us to speed up some other processes (ex. finding the first block to draw)
                // since we can binary search by start tick.
                AZStd::sort(
                    newVisualizerData.begin(), newVisualizerData.end(),
                    [](const TimeRegion& lhs, const TimeRegion& rhs)
                    {
                        return lhs.m_startTick < rhs.m_startTick;
                    });

                // Use the latest frame's data as the new bounds of the viewport
                m_viewportStartTick = AZStd::min(newVisualizerData.front().m_startTick, m_viewportStartTick);
                m_viewportEndTick = AZStd::max(newVisualizerData.back().m_endTick, m_viewportEndTick);

                m_savedRegionCount += newVisualizerData.size();

                // Move onto the end of the current thread's saved data, sorted order maintained
                AZStd::vector<TimeRegion>& savedDataVec = m_savedData[threadId];
                savedDataVec.insert(
                    savedDataVec.end(), AZStd::make_move_iterator(newVisualizerData.begin()), AZStd::make_move_iterator(newVisualizerData.end()));
            }
        }

        inline void ImGuiCpuProfiler::CullFrameData(const AZ::RHI::CpuTimingStatistics& currentCpuTimingStatistics)
        {
            const AZStd::sys_time_t frameToFrameTime = currentCpuTimingStatistics.m_frameToFrameTime;
            const AZStd::sys_time_t deleteBeforeTick = AZStd::GetTimeNowTicks() - frameToFrameTime * m_framesToCollect;

            // Remove old frame boundary data
            auto firstBoundaryToKeepItr = AZStd::upper_bound(m_frameEndTicks.begin(), m_frameEndTicks.end(), deleteBeforeTick);
            m_frameEndTicks.erase(m_frameEndTicks.begin(), firstBoundaryToKeepItr);

            // Remove old region data for each thread
            for (auto& [threadId, savedRegions] : m_savedData)
            {
                AZStd::size_t sizeBeforeRemove = savedRegions.size();

                auto firstRegionToKeep = AZStd::lower_bound(
                    savedRegions.begin(), savedRegions.end(), deleteBeforeTick,
                    [](const TimeRegion& region, AZStd::sys_time_t target)
                    {
                        return region.m_startTick < target;
                    });
                savedRegions.erase(savedRegions.begin(), firstRegionToKeep);

                m_savedRegionCount -= sizeBeforeRemove - savedRegions.size();
            }
        }

        inline void ImGuiCpuProfiler::DrawBlock(const TimeRegion& block, u64 targetRow)
        {
            float wy = ImGui::GetWindowPos().y - ImGui::GetScrollY();

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            const float startPixel = ConvertTickToPixelSpace(block.m_startTick);
            const float endPixel = ConvertTickToPixelSpace(block.m_endTick);

            const ImVec2 startPoint = { startPixel, wy + targetRow * RowHeight };
            const ImVec2 endPoint = { endPixel, wy + targetRow * RowHeight + 40 };

            const ImU32 blockColor = GetBlockColor(block);

            drawList->AddRectFilled(startPoint, endPoint, blockColor, 0);

            // Draw the region name if possible
            // If the block's current width is too small, we skip drawing the label.
            const float regionPixelWidth = endPixel - startPixel;
            const float maxCharWidth = ImGui::CalcTextSize("M").x; // M is usually the largest character in most fonts (see CSS em)
            if (regionPixelWidth > maxCharWidth) // We can draw at least one character
            {
                const AZStd::string label =
                    AZStd::string::format("%s/ %s", block.m_groupRegionName->m_groupName, block.m_groupRegionName->m_regionName);
                const float textWidth = ImGui::CalcTextSize(label.c_str()).x;

                if (regionPixelWidth < textWidth) // Not enough space in the block to draw the whole name, draw clipped text.
                {
                    // clipRect appears to only clip when a character is fully outside of its bounds which can lead to overflow
                    // for now subtract the width of a character
                    const ImVec4 clipRect = { startPoint.x, startPoint.y, endPoint.x - maxCharWidth, endPoint.y };
                    const float fontSize = ImGui::GetFont()->FontSize;

                    ImGui::GetFont()->RenderText(drawList, fontSize, startPoint, IM_COL32_WHITE, clipRect, label.c_str(), 0);
                }
                else // We have enough space to draw the entire label, draw and center text.
                {
                    const float remainingWidth = regionPixelWidth - textWidth;
                    const float offset = remainingWidth * .5;

                    drawList->AddText({ startPoint.x + offset, startPoint.y }, IM_COL32_WHITE, label.c_str());
                }
            }

            // Tooltip and block highlighting
            if (ImGui::IsMouseHoveringRect(startPoint, endPoint) && ImGui::IsWindowHovered())
            {
                // Open function statistics map on click
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    const GroupRegionName* key = block.m_groupRegionName;
                    m_regionStatisticsMap[key].m_draw = true;
                }

                // Hovering outline
                drawList->AddRect(startPoint, endPoint, ImGui::GetColorU32({ 1, 1, 1, 1 }), 0.0, 0, 1.5);

                ImGui::BeginTooltip();
                ImGui::Text("%s::%s", block.m_groupRegionName->m_groupName, block.m_groupRegionName->m_regionName);
                ImGui::Text("Execution time: %.3f ms", CpuProfilerImGuiHelper::TicksToMs(block.m_endTick - block.m_startTick));
                ImGui::Text("Ticks %lld => %lld", block.m_startTick, block.m_endTick);
                ImGui::EndTooltip();
            }
        }

        inline ImU32 ImGuiCpuProfiler::GetBlockColor(const TimeRegion& block)
        {
            // Use the GroupRegionName pointer a key into the cache, equal regions will have equal pointers
            const GroupRegionName* key = block.m_groupRegionName;
            if (m_regionColorMap.contains(key)) // Cache hit
            {
                return ImGui::GetColorU32(m_regionColorMap[key]);
            }

            // Cache miss, generate a new random color
            AZ::SimpleLcgRandom rand(aznumeric_cast<u64>(AZStd::GetTimeNowTicks()));
            const float r = AZStd::clamp(rand.GetRandomFloat(), .1f, .9f);
            const float g = AZStd::clamp(rand.GetRandomFloat(), .1f, .9f);
            const float b = AZStd::clamp(rand.GetRandomFloat(), .1f, .9f);
            const ImVec4 randomColor = {r, g, b, .8};
            m_regionColorMap.emplace(key, randomColor);
            return ImGui::GetColorU32(randomColor);
        }

        inline void ImGuiCpuProfiler::DrawThreadSeparator(u64 baseRow, u64 maxDepth)
        {
            const ImU32 red = ImGui::GetColorU32({ 1, 0, 0, 1 });

            auto [wx, wy] = ImGui::GetWindowPos();
            wy -= ImGui::GetScrollY();
            const float windowWidth = ImGui::GetWindowWidth();
            const float boundaryY = wy + (baseRow + maxDepth + 1) * RowHeight - 5;

            ImGui::GetWindowDrawList()->AddLine({ wx, boundaryY }, { wx + windowWidth, boundaryY }, red, 2.0f);
        }

        inline void ImGuiCpuProfiler::DrawThreadLabel(u64 baseRow, AZStd::thread_id threadId)
        {
            auto [wx, wy] = ImGui::GetWindowPos();
            wy -= ImGui::GetScrollY();
            const AZStd::string threadIdText =  CpuProfilerImGuiHelper::TextThreadId(threadId.m_id);

            ImGui::GetWindowDrawList()->AddText({ wx + 10, wy + baseRow * RowHeight + 5 }, IM_COL32_WHITE, threadIdText.c_str());
        }

        inline void ImGuiCpuProfiler::DrawRegionStatistics()
        {
            for (auto& [groupRegionName, stat] : m_regionStatisticsMap)
            {
                if (stat.m_draw)
                {
                    ImGui::SetNextWindowSize({300, 340}, ImGuiCond_FirstUseEver);
                    ImGui::Begin(groupRegionName->m_regionName, &stat.m_draw, 0);

                    if (ImGui::Button(stat.m_record ? "Pause" : "Resume"))
                    {
                        stat.m_record = !stat.m_record;
                    }

                    ImGui::Text("Invocations: %llu", stat.m_invocations);
                    ImGui::Text("Average time: %.3f ms", stat.CalcAverageTimeMs());

                    ImGui::Separator();

                    ImGui::ColorPicker4("Region color", &m_regionColorMap[groupRegionName].x); 
                    ImGui::End();
                }
            }
        }

        inline void ImGuiCpuProfiler::DrawFrameBoundaries()
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            const float wy = ImGui::GetWindowPos().y;
            const float windowHeight = ImGui::GetWindowHeight();
            const ImU32 red = ImGui::GetColorU32({ 1, 0, 0, 1 });

            // End ticks are sorted in increasing order, find the first frame bound to draw
            auto endTickItr = AZStd::lower_bound(m_frameEndTicks.begin(), m_frameEndTicks.end(), m_viewportStartTick);

            while (endTickItr != m_frameEndTicks.end() && *endTickItr < m_viewportEndTick)
            {
                const float horizontalPixel = ConvertTickToPixelSpace(*endTickItr);
                drawList->AddLine({ horizontalPixel, wy }, { horizontalPixel, wy + windowHeight }, red);
                ++endTickItr;
            }
        }

        inline void ImGuiCpuProfiler::DrawRuler()
        {
            // Use a pair of iterators to go through all saved frame boundaries and draw ruler lines
            auto lastFrameBoundaryItr = AZStd::lower_bound(m_frameEndTicks.begin(), m_frameEndTicks.end(), m_viewportStartTick);
            auto nextFrameBoundaryItr = lastFrameBoundaryItr;
            if (lastFrameBoundaryItr != m_frameEndTicks.begin()) 
            {
                --lastFrameBoundaryItr;
            }

            const auto [wx, wy] = ImGui::GetWindowPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            while (nextFrameBoundaryItr != m_frameEndTicks.end() && *lastFrameBoundaryItr <= m_viewportEndTick)
            {
                const AZStd::sys_time_t lastFrameBoundaryTick = *lastFrameBoundaryItr;
                const AZStd::sys_time_t nextFrameBoundaryTick = *nextFrameBoundaryItr;
                if (lastFrameBoundaryTick > m_viewportEndTick)
                {
                    break;
                }

                const float lastFrameBoundaryPixel = ConvertTickToPixelSpace(lastFrameBoundaryTick);
                const float nextFrameBoundaryPixel = ConvertTickToPixelSpace(nextFrameBoundaryTick);

                const AZStd::string label =
                    AZStd::string::format("%.2f ms", CpuProfilerImGuiHelper::TicksToMs(nextFrameBoundaryTick - lastFrameBoundaryTick));
                const float labelWidth = ImGui::CalcTextSize(label.c_str()).x;

                // The label can fit between the two boundaries, center it and draw
                if (labelWidth <= nextFrameBoundaryPixel - lastFrameBoundaryPixel) 
                {
                    const float offset = (nextFrameBoundaryPixel - lastFrameBoundaryPixel - labelWidth) /2;
                    const float textBeginPixel = lastFrameBoundaryPixel + offset;
                    const float textEndPixel = textBeginPixel + labelWidth;

                    // Execution time label
                    drawList->AddText({ textBeginPixel, wy + ImGui::GetWindowHeight() / 4 }, IM_COL32_WHITE, label.c_str());

                    // Left side
                    drawList->AddLine(
                        { lastFrameBoundaryPixel, wy + ImGui::GetWindowHeight() / 2 },
                        { textBeginPixel - 5, wy + ImGui::GetWindowHeight() / 2},
                        IM_COL32_WHITE);

                    // Right side
                    drawList->AddLine(
                        { textEndPixel, wy + ImGui::GetWindowHeight()/2 },
                        { nextFrameBoundaryPixel,  wy + ImGui::GetWindowHeight()/2 },
                        IM_COL32_WHITE);
                }
                else // Cannot fit inside, just draw a line between the two boundaries
                {
                    drawList->AddLine(
                        { lastFrameBoundaryPixel, wy + ImGui::GetWindowHeight() / 2 },
                        { nextFrameBoundaryPixel, wy + ImGui::GetWindowHeight() / 2 },
                        IM_COL32_WHITE);
                }

                // Left bound
                drawList->AddLine(
                    { lastFrameBoundaryPixel, wy },
                    { lastFrameBoundaryPixel, wy + ImGui::GetWindowHeight() },
                    IM_COL32_WHITE);

                // Right bound
                drawList->AddLine(
                    { nextFrameBoundaryPixel, wy },
                    { nextFrameBoundaryPixel, wy + ImGui::GetWindowHeight() },
                    IM_COL32_WHITE);

                lastFrameBoundaryItr = nextFrameBoundaryItr;
                ++nextFrameBoundaryItr;
            }
        }

        inline void ImGuiCpuProfiler::DrawFrameTimeHistogram()
        {
            // TODO draw 60fps 30fps lines at the start
            ImGui::BeginTooltip();
            ImGui::Text("%f, %f", ImGui::GetMousePos().x, ImGui::GetMousePos().y);
            ImGui::EndTooltip();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const auto [wx, wy] = ImGui::GetWindowPos();
            const auto tickDelta = AZStd::GetTimeTicksPerSecond();
            const auto viewportCenter = m_viewportEndTick - (m_viewportEndTick - m_viewportStartTick) / 2;
            const auto leftHistogramBound = viewportCenter - tickDelta;
            const auto rightHistogramBound = viewportCenter + tickDelta;

            // Find the first onscreen frame execution time 
            auto frameItr = AZStd::lower_bound(
                m_frameTimes.begin(), m_frameTimes.end(), leftHistogramBound,
                [](const FrameHistogramEntry& entry, AZStd::sys_time_t target)
                {
                    return entry.m_endTick < target;
                });
            if (frameItr != m_frameTimes.begin())
            {
                --frameItr;
            }

            auto convert = [leftHistogramBound, rightHistogramBound](AZStd::sys_time_t tick)
            {
                const float wx = ImGui::GetWindowPos().x;
                const float tickSpaceShifted = aznumeric_cast<float>(tick - leftHistogramBound); // This will be close to zero, so FP inaccuracy should not be too bad
                const float tickSpaceNormalized = tickSpaceShifted / (rightHistogramBound - leftHistogramBound);
                const float pixelSpace = tickSpaceNormalized * ImGui::GetWindowWidth() + wx;
                return pixelSpace;
            };

            while (frameItr->m_endTick < rightHistogramBound && ++frameItr != m_frameTimes.end())
            {
                const auto tick = frameItr->m_endTick;
                const auto et = CpuProfilerImGuiHelper::TicksToMs(frameItr->m_frameTime);

                const float pixelX = convert(tick);
                const auto height = et;

                const ImVec2 startpos = { pixelX, ImGui::GetWindowHeight() + wy };
                const ImVec2 endpos = { pixelX, ImGui::GetWindowHeight() + wy - height };
                drawList->AddLine(startpos, endpos, IM_COL32_WHITE, 3.0);
                ImGui::BeginTooltip();
                ImGui::Text("Base: %f, %f", startpos.x, startpos.y);
                ImGui::EndTooltip();
            }
        }

        inline AZStd::sys_time_t ImGuiCpuProfiler::GetViewportTickWidth() const
        {
            return m_viewportEndTick - m_viewportStartTick;
        }

        inline float ImGuiCpuProfiler::ConvertTickToPixelSpace(AZStd::sys_time_t tick) const
        {
            const float wx = ImGui::GetWindowPos().x;
            const float tickSpaceShifted = aznumeric_cast<float>(tick - m_viewportStartTick); // This will be close to zero, so FP inaccuracy should not be too bad
            const float tickSpaceNormalized = tickSpaceShifted / GetViewportTickWidth();
            const float pixelSpace = tickSpaceNormalized * ImGui::GetWindowWidth() + wx;
            return pixelSpace;
        }

        // System tick bus overrides
        inline void ImGuiCpuProfiler::OnSystemTick()
        {
            if (m_paused)
            {
                SystemTickBus::Handler::BusDisconnect();
            }
            else
            {
                const AZStd::sys_time_t lastFrameTime = AZStd::GetTimeNowTicks() - m_frameEndTicks.back();
                m_frameTimes.push_back({ m_frameEndTicks.back(), lastFrameTime });
                m_frameEndTicks.push_back(AZStd::GetTimeNowTicks());
            }
        }

        // ----- RegionStatistics implementation ----- 
        
        inline float RegionStatistics::CalcAverageTimeMs() const
        {
            if (m_invocations == 0)
            {
                return 0.0;
            }
            const double averageTicks = aznumeric_cast<double>(m_totalTicks) / m_invocations;
            return CpuProfilerImGuiHelper::TicksToMs(aznumeric_cast<AZStd::sys_time_t>(averageTicks));
        }

        inline void RegionStatistics::RecordRegion(const AZ::RHI::CachedTimeRegion& region)
        {
            m_invocations++;
            m_totalTicks += region.m_endTick - region.m_startTick;
        }
    } // namespace Render
} // namespace AZ
