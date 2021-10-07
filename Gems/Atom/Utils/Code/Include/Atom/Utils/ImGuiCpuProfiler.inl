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
#include <Atom/RHI/CpuProfilerImpl.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/JSON/filereadstream.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/time.h>


namespace AZ
{
    namespace Render
    {
        namespace CpuProfilerImGuiHelper
        {
            inline float TicksToMs(AZStd::sys_time_t ticks)
            {
                // Note: converting to microseconds integer before converting to milliseconds float
                const AZStd::sys_time_t ticksPerSecond = AZStd::GetTimeTicksPerSecond();
                AZ_Assert(ticksPerSecond >= 1000, "Error in converting ticks to ms, expected ticksPerSecond >= 1000");
                return static_cast<float>((ticks * 1000) / (ticksPerSecond / 1000)) / 1000.0f;
            }

            using DeserializedCpuData = AZStd::vector<RHI::CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry>;
            inline Outcome<DeserializedCpuData, AZStd::string> LoadSavedCpuProfilingStatistics(const AZStd::string& capturePath)
            {
                auto* base = IO::FileIOBase::GetInstance();

                char resolvedPath[IO::MaxPathLength];
                if (!base->ResolvePath(capturePath.c_str(), resolvedPath, IO::MaxPathLength))
                {
                    return Failure(AZStd::string::format("Could not resolve the path to file %s, is the path correct?", resolvedPath));
                }

                u64 captureSizeBytes;
                const IO::Result fileSizeResult = base->Size(resolvedPath, captureSizeBytes);
                if (!fileSizeResult)
                {
                    return Failure(AZStd::string::format("Could not read the size of file %s, is the path correct?", resolvedPath));
                }

                // NOTE: this uses raw file pointers over the abstractions and utility functions provided by AZ::JsonSerializationUtils because
                // saved profiling captures can be upwards of 400 MB. This necessitates a buffered approach to avoid allocating huge chunks of memory.
                FILE* fp = nullptr;
                azfopen(&fp, resolvedPath, "rb");
                if (!fp)
                {
                    return Failure(AZStd::string::format("Could not fopen file %s, is the path correct?\n", resolvedPath));
                }

                constexpr AZStd::size_t MaxBufSize = 65536;
                const AZStd::size_t bufSize = AZStd::min(MaxBufSize, aznumeric_cast<AZStd::size_t>(captureSizeBytes));
                char* buf = reinterpret_cast<char*>(azmalloc(bufSize));

                rapidjson::Document document;
                rapidjson::FileReadStream inputStream(fp, buf, bufSize);
                document.ParseStream(inputStream);

                azfree(buf);
                fclose(fp);

                if (document.HasParseError())
                {
                    const auto pe = document.GetParseError();
                    return Failure(AZStd::string::format(
                        "Rapidjson could not parse the document with ParseErrorCode %u. See 3rdParty/rapidjson/error.h for definitions.\n", pe));
                }

                if (!document.IsObject() || !document.HasMember("ClassData"))
                {
                    return Failure(AZStd::string::format(
                        "Error in loading saved capture: top-level object does not have a ClassData field. Did the serialization format change recently?\n"));
                }

                AZ_TracePrintf("JsonUtils", "Successfully loaded JSON into memory.\n");

                const auto& root = document["ClassData"];
                RHI::CpuProfilingStatisticsSerializer serializer;
                const JsonSerializationResult::ResultCode deserializationResult = JsonSerialization::Load(serializer, root);
                if (deserializationResult.GetProcessing() == JsonSerializationResult::Processing::Halted
                    || serializer.m_cpuProfilingStatisticsSerializerEntries.empty())
                {
                    return Failure(AZStd::string::format("Error in deserializing document: %s\n", deserializationResult.ToString(capturePath.c_str()).c_str()));
                }

                AZ_TracePrintf("JsonUtils", "Successfully loaded CPU profiling data with %zu profiling entries.\n",
                     serializer.m_cpuProfilingStatisticsSerializerEntries.size());

                return Success(AZStd::move(serializer.m_cpuProfilingStatisticsSerializerEntries));
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

                if (m_showFilePicker)
                {
                    DrawFilePicker();
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
            if (!m_lastCapturedFilePath.empty())
            {
                ImGui::Text("Saved: %s", m_lastCapturedFilePath.c_str());
            }

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

            ImGui::SameLine();
            bool isInProgress = RHI::CpuProfiler::Get()->IsContinuousCaptureInProgress();
            if (ImGui::Button(isInProgress ? "End" : "Begin"))
            {
                if (isInProgress)
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
                        &AZ::Render::ProfilingCaptureRequestBus::Events::EndContinuousCpuProfilingCapture, frameDataFilePath);
                    m_paused = true;
                }

                else
                {
                    AZ::Render::ProfilingCaptureRequestBus::Broadcast(
                        &AZ::Render::ProfilingCaptureRequestBus::Events::BeginContinuousCpuProfilingCapture);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Load file"))
            {
                m_showFilePicker = true;

                // Only update the cached file list when opened so that we aren't making IO calls on every frame.
                auto* base = AZ::IO::FileIOBase::GetInstance();
                const AZStd::string defaultSavedCapturePath = "@user@/CpuProfiler";

                m_cachedCapturePaths.clear();
                base->FindFiles(
                    defaultSavedCapturePath.c_str(), "*.json",
                    [&paths = m_cachedCapturePaths](const char* path) -> bool
                    {
                        auto foundPath = IO::Path(path);
                        paths.push_back(foundPath);
                        return true;
                    });

                // Sort by decreasing modification time (most recent at the top)
                AZStd::sort(m_cachedCapturePaths.begin(), m_cachedCapturePaths.end(),
                    [&base](const IO::Path& lhs, const IO::Path& rhs)
                    {
                        return base->ModificationTime(lhs.c_str()) > base->ModificationTime(rhs.c_str());
                    });
            }
        }

        inline void ImGuiCpuProfiler::DrawTable()
        {
            const auto flags =
                ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
            if (ImGui::BeginTable("FunctionStatisticsTable", 6, flags))
            {
                // Table header setup
                ImGui::TableSetupColumn("Group");
                ImGui::TableSetupColumn("Region");
                ImGui::TableSetupColumn("MTPC (ms)");
                ImGui::TableSetupColumn("Max (ms)");
                ImGui::TableSetupColumn("Invocations");
                ImGui::TableSetupColumn("Total (ms)");
                ImGui::TableHeadersRow();
                ImGui::TableNextColumn();

                ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
                if (sortSpecs && sortSpecs->SpecsDirty)
                {
                    SortTable(sortSpecs);
                }

                // Draw all of the rows held in the GroupRegionMap
                for (const auto* statistics : m_tableData)
                {
                    if (!m_timedRegionFilter.PassFilter(statistics->m_groupName.c_str())
                        && !m_timedRegionFilter.PassFilter(statistics->m_regionName.c_str()))
                    {
                        continue;
                    }

                    ImGui::Text("%s", statistics->m_groupName.c_str());
                    const ImVec2 topLeftBound = ImGui::GetItemRectMin();
                    ImGui::TableNextColumn();

                    ImGui::Text("%s", statistics->m_regionName.c_str());
                    ImGui::TableNextColumn();

                    ImGui::Text("%.2f", CpuProfilerImGuiHelper::TicksToMs(statistics->m_runningAverageTicks));
                    ImGui::TableNextColumn();

                    ImGui::Text("%.2f", CpuProfilerImGuiHelper::TicksToMs(statistics->m_maxTicks));
                    ImGui::TableNextColumn();

                    ImGui::Text("%llu", statistics->m_invocationsLastFrame);
                    ImGui::TableNextColumn();

                    ImGui::Text("%.2f", CpuProfilerImGuiHelper::TicksToMs(statistics->m_lastFrameTotalTicks));
                    const ImVec2 botRightBound = ImGui::GetItemRectMax();
                    ImGui::TableNextColumn();

                    // NOTE: we are manually checking the bounds rather than using ImGui::IsItemHovered + Begin/EndGroup because
                    // ImGui reports incorrect bounds when using Begin/End group in the Tables API.
                    if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(topLeftBound, botRightBound, false))
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", statistics->GetExecutingThreadsLabel().c_str());
                        ImGui::EndTooltip();
                    }
                }
            }
            ImGui::EndTable();
        }

        inline void ImGuiCpuProfiler::SortTable(ImGuiTableSortSpecs* sortSpecs)
        {
            const bool ascending = sortSpecs->Specs->SortDirection == ImGuiSortDirection_Ascending;
            const ImS16 columnToSort = sortSpecs->Specs->ColumnIndex;

            switch (columnToSort)
            {
            case (0): // Sort by group name
                AZStd::sort(m_tableData.begin(), m_tableData.end(), TableRow::TableRowCompareFunctor(&TableRow::m_groupName, ascending));
                break;
            case (1): // Sort by region name
                AZStd::sort(m_tableData.begin(), m_tableData.end(), TableRow::TableRowCompareFunctor(&TableRow::m_regionName, ascending));
                break;
            case (2): // Sort by average time
                AZStd::sort(m_tableData.begin(), m_tableData.end(), TableRow::TableRowCompareFunctor(&TableRow::m_runningAverageTicks, ascending));
                break;
            case (3): // Sort by max time
                AZStd::sort(m_tableData.begin(), m_tableData.end(), TableRow::TableRowCompareFunctor(&TableRow::m_maxTicks, ascending));
                break;
            case (4): // Sort by invocations
                AZStd::sort(m_tableData.begin(), m_tableData.end(), TableRow::TableRowCompareFunctor(&TableRow::m_invocationsLastFrame, ascending));
                break;
            case (5): // Sort by total time
                AZStd::sort(m_tableData.begin(), m_tableData.end(), TableRow::TableRowCompareFunctor(&TableRow::m_lastFrameTotalTicks, ascending));
                break;
            }
            sortSpecs->SpecsDirty = false;
        }

        inline void ImGuiCpuProfiler::DrawStatisticsView()
        {
            DrawCommonHeader();

            const AZ::RHI::CpuTimingStatistics& cpuTimingStatistics = m_cpuTimingStatisticsWhenPause;

            const auto ShowTimeInMs = [](AZStd::sys_time_t duration)
            {
                ImGui::Text("%.2f ms", CpuProfilerImGuiHelper::TicksToMs(duration));
            };

            const auto ShowRow = [&ShowTimeInMs](const char* regionLabel, AZStd::sys_time_t duration)
            {
                ImGui::Text("%s", regionLabel);
                ImGui::NextColumn();

                ShowTimeInMs(duration);
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

                m_timedRegionFilter.Draw("Filter");
                ImGui::SameLine();
                if (ImGui::Button("Clear Filter"))
                {
                    m_timedRegionFilter.Clear();
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Table"))
                {
                    m_tableData.clear();
                    m_groupRegionMap.clear();
                }

                DrawTable();
            }
        }

        inline void ImGuiCpuProfiler::DrawFilePicker()
        {
            ImGui::SetNextWindowSize({ 500, 200 }, ImGuiCond_Once);
            if (ImGui::Begin("File Picker", &m_showFilePicker))
            {
                if (ImGui::Button("Load selected"))
                {
                    LoadFile();
                }

                auto getter = [](void* vectorPointer, int idx, const char** out_text) -> bool
                {
                    const auto& pathVec = *static_cast<AZStd::vector<IO::Path>*>(vectorPointer);
                    if (idx < 0 || idx >= pathVec.size())
                    {
                        return false;
                    }
                    *out_text = pathVec[idx].c_str();
                    return true;
                };

                ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth());
                ImGui::ListBox("", &m_currentFileIndex, getter, &m_cachedCapturePaths, aznumeric_cast<int>(m_cachedCapturePaths.size()));
            }
            ImGui::End();
        }

        inline void ImGuiCpuProfiler::LoadFile()
        {
            const IO::Path& pathToLoad = m_cachedCapturePaths[m_currentFileIndex];
            auto loadResult = CpuProfilerImGuiHelper::LoadSavedCpuProfilingStatistics(pathToLoad.String());
            if (!loadResult.IsSuccess())
            {
                AZ_TracePrintf("ImGuiCpuProfiler", "%s", loadResult.GetError().c_str());
                return;
            }

            AZStd::vector<RHI::CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry> deserializedData = loadResult.TakeValue();

            // Clear visualizer and statistics view state
            m_savedRegionCount = deserializedData.size();
            m_savedData.clear();
            m_paused = true;
            AZ::RHI::CpuProfiler::Get()->SetProfilerEnabled(false);
            m_frameEndTicks.clear();

            m_tableData.clear();
            m_groupRegionMap.clear();

            for (const auto& entry : deserializedData)
            {
                const auto [groupNameItr, wasGroupNameInserted] = m_deserializedStringPool.emplace(entry.m_groupName.GetCStr());
                const auto [regionNameItr, wasRegionNameInserted] = m_deserializedStringPool.emplace(entry.m_regionName.GetCStr());
                const auto [groupRegionNameItr, wasGroupRegionNameInserted] =
                    m_deserializedGroupRegionNamePool.emplace(groupNameItr->c_str(), regionNameItr->c_str());

                const RHI::CachedTimeRegion newRegion(*groupRegionNameItr, entry.m_stackDepth, entry.m_startTick, entry.m_endTick);
                m_savedData[entry.m_threadId].push_back(newRegion);

                // Since we don't serialize the frame boundaries, we need to use the RPI's OnSystemTick event as a heuristic.
                const static Name frameBoundaryName = Name("RPISystem: OnSystemTick");
                if (entry.m_regionName == frameBoundaryName)
                {
                    m_frameEndTicks.push_back(entry.m_endTick);
                }

                // Update running statistics
                if (!m_groupRegionMap[*groupNameItr].contains(*regionNameItr))
                {
                    m_groupRegionMap[*groupNameItr][*regionNameItr].m_groupName = *groupNameItr;
                    m_groupRegionMap[*groupNameItr][*regionNameItr].m_regionName = *regionNameItr;
                    m_tableData.push_back(&m_groupRegionMap[*groupNameItr][*regionNameItr]);
                }
                m_groupRegionMap[*groupNameItr][*regionNameItr].RecordRegion(newRegion, entry.m_threadId);
            }

            // Update viewport bounds with some added UX fudge factor
            m_viewportStartTick = deserializedData.back().m_startTick - 1000;
            m_viewportEndTick = deserializedData.back().m_endTick + 1000;

            // Invariant: each vector in m_savedData must be sorted so that we can efficiently cull region data.
            for (auto& [threadId, singleThreadData] : m_savedData)
            {
                AZStd::sort(singleThreadData.begin(), singleThreadData.end(),
                [](const TimeRegion& lhs, const TimeRegion& rhs)
                {
                    return lhs.m_startTick < rhs.m_startTick;
                });
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
                ImGui::SliderInt("Saved Frames", &m_framesToCollect, 10, 20000, "%d", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
                m_visualizerHighlightFilter.Draw("Find Region");

                ImGui::NextColumn();

                ImGui::Text("Viewport width: %.3f ms", CpuProfilerImGuiHelper::TicksToMs(GetViewportTickWidth()));
                ImGui::Text("Ticks [%lld , %lld]", m_viewportStartTick, m_viewportEndTick);
                ImGui::Text("Recording %zu threads", m_savedData.size());
                ImGui::Text("%llu profiling events saved", m_savedRegionCount);

                ImGui::NextColumn();

                ImGui::TextWrapped(
                    "Hold the right mouse button to move around. Zoom by scrolling the mouse wheel while holding <ctrl>.");
            }

            ImGui::Columns(1, "FrameTimeColumn", true);

            if (ImGui::BeginChild("FrameTimeHistogram", { 0, 50 }, true, ImGuiWindowFlags_NoScrollbar))
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

                    if (regionItr == singleThreadData.end())
                    {
                        continue;
                    }

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

            // Get the latest TimeRegionMap
            const RHI::CpuProfiler::TimeRegionMap& timeRegionMap = RHI::CpuProfiler::Get()->GetTimeRegionMap();

            m_viewportStartTick = AZStd::numeric_limits<s64>::max();
            m_viewportEndTick = AZStd::numeric_limits<s64>::lowest();

            // Iterate through the entire TimeRegionMap and copy the data since it will get deleted on the next frame
            for (const auto& [threadId, singleThreadRegionMap] : timeRegionMap)
            {
                const size_t threadIdHashed = AZStd::hash<AZStd::thread_id>{}(threadId);
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
                        const AZStd::string& groupName = region.m_groupRegionName.m_groupName;

                        if (!m_groupRegionMap[groupName].contains(regionName))
                        {
                            m_groupRegionMap[groupName][regionName].m_groupName = groupName;
                            m_groupRegionMap[groupName][regionName].m_regionName = regionName;
                            m_tableData.push_back(&m_groupRegionMap[groupName][regionName]);
                        }

                        m_groupRegionMap[groupName][regionName].RecordRegion(region, threadIdHashed);
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
                AZStd::vector<TimeRegion>& savedDataVec = m_savedData[threadIdHashed];
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

                // Early out to avoid the linear erase_if call
                if (savedRegions.size() >= 1 && savedRegions.at(0).m_startTick > deleteBeforeTick)
                {
                    continue;
                }

                // Use erase_if over plain upper_bound + erase to avoid repeated shifts. erase requires a shift of all elements to the right
                // for each element that is erased, while erase_if squashes all removes into a single shift which significantly improves perf.
                AZStd::erase_if(
                    savedRegions,
                    [deleteBeforeTick](const TimeRegion& region)
                    {
                        return region.m_startTick < deleteBeforeTick;
                    });

                m_savedRegionCount -= sizeBeforeRemove - savedRegions.size();
            }

            // Remove any threads from the top-level map that no longer hold data
            AZStd::erase_if(
                m_savedData,
                [](const auto& singleThreadDataEntry)
                {
                    return singleThreadDataEntry.second.empty();
                });
        }

        inline void ImGuiCpuProfiler::DrawBlock(const TimeRegion& block, u64 targetRow)
        {
            // Don't draw anything if the user is searching for regions and this block doesn't pass the filter
            if (!m_visualizerHighlightFilter.PassFilter(block.m_groupRegionName.m_regionName))
            {
                return;
            }

            float wy = ImGui::GetWindowPos().y - ImGui::GetScrollY();

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            const float startPixel = ConvertTickToPixelSpace(block.m_startTick, m_viewportStartTick, m_viewportEndTick);
            const float endPixel = ConvertTickToPixelSpace(block.m_endTick, m_viewportStartTick, m_viewportEndTick);

            if (endPixel - startPixel < 0.5f)
            {
                return;
            }

            const ImVec2 startPoint = { startPixel, wy + targetRow * RowHeight + 1};
            const ImVec2 endPoint = { endPixel, wy + (targetRow + 1) * RowHeight };

            const ImU32 blockColor = GetBlockColor(block);

            drawList->AddRectFilled(startPoint, endPoint, blockColor, 0);
            drawList->AddLine(startPoint, { endPixel, startPoint.y }, IM_COL32_BLACK, 0.5f);
            drawList->AddLine({ startPixel, endPoint.y }, endPoint, IM_COL32_BLACK, 0.5f);

            // Draw the region name if possible
            // If the block's current width is too small, we skip drawing the label.
            const float regionPixelWidth = endPixel - startPixel;
            const float maxCharWidth = ImGui::CalcTextSize("M").x; // M is usually the largest character in most fonts (see CSS em)
            if (regionPixelWidth > maxCharWidth) // We can draw at least one character
            {
                const AZStd::string label =
                    AZStd::string::format("%s/ %s", block.m_groupRegionName.m_groupName, block.m_groupRegionName.m_regionName);
                const float textWidth = ImGui::CalcTextSize(label.c_str()).x;

                if (regionPixelWidth < textWidth) // Not enough space in the block to draw the whole name, draw clipped text.
                {
                    const ImVec4 clipRect = { startPoint.x, startPoint.y, endPoint.x - maxCharWidth, endPoint.y };

                    // NOTE: RenderText calls do not automatically account for the global scale (which is modified at high DPI)
                    // so we must adjust for the scale manually.
                    const float scaleFactor = ImGui::GetIO().FontGlobalScale;
                    const float fontSize = ImGui::GetFont()->FontSize * scaleFactor;

                    ImGui::GetFont()->RenderText(drawList, fontSize, startPoint, IM_COL32_WHITE, clipRect, label.c_str(), 0);
                }
                else // We have enough space to draw the entire label, draw and center text.
                {
                    const float remainingWidth = regionPixelWidth - textWidth;
                    const float offset = remainingWidth * .5f;

                    drawList->AddText({ startPoint.x + offset, startPoint.y }, IM_COL32_WHITE, label.c_str());
                }
            }

            // Tooltip and block highlighting
            if (ImGui::IsMouseHoveringRect(startPoint, endPoint) && ImGui::IsWindowHovered())
            {
                // Go to the statistics view when a region is clicked
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    m_enableVisualizer = false;
                    const auto newFilter = AZStd::string(block.m_groupRegionName.m_regionName);
                    m_timedRegionFilter = ImGuiTextFilter(newFilter.c_str());
                    m_timedRegionFilter.Build();
                }
                // Hovering outline
                drawList->AddRect(startPoint, endPoint, ImGui::GetColorU32({ 1, 1, 1, 1 }), 0.0, 0, 1.5);

                ImGui::BeginTooltip();
                ImGui::Text("%s::%s", block.m_groupRegionName.m_groupName, block.m_groupRegionName.m_regionName);
                ImGui::Text("Execution time: %.3f ms", CpuProfilerImGuiHelper::TicksToMs(block.m_endTick - block.m_startTick));
                ImGui::Text("Ticks %lld => %lld", block.m_startTick, block.m_endTick);
                ImGui::EndTooltip();
            }
        }

        inline ImU32 ImGuiCpuProfiler::GetBlockColor(const TimeRegion& block)
        {
            // Use the GroupRegionName pointer a key into the cache, equal regions will have equal pointers
            const GroupRegionName& key = block.m_groupRegionName;
            if (auto iter = m_regionColorMap.find(key); iter != m_regionColorMap.end()) // Cache hit
            {
                return ImGui::GetColorU32(iter->second);
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
            const float boundaryY = wy + (baseRow + maxDepth + 1) * RowHeight;

            ImGui::GetWindowDrawList()->AddLine({ wx, boundaryY }, { wx + windowWidth, boundaryY }, red, 1.0f);
        }

        inline void ImGuiCpuProfiler::DrawThreadLabel(u64 baseRow, size_t threadId)
        {
            auto [wx, wy] = ImGui::GetWindowPos();
            wy -= ImGui::GetScrollY();
            const AZStd::string threadIdText = AZStd::string::format("Thread: %zu", threadId);

            ImGui::GetWindowDrawList()->AddText({ wx + 10, wy + baseRow * RowHeight}, IM_COL32_WHITE, threadIdText.c_str());
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
                const float horizontalPixel = ConvertTickToPixelSpace(*endTickItr, m_viewportStartTick, m_viewportEndTick);
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

                const float lastFrameBoundaryPixel = ConvertTickToPixelSpace(lastFrameBoundaryTick, m_viewportStartTick, m_viewportEndTick);
                const float nextFrameBoundaryPixel = ConvertTickToPixelSpace(nextFrameBoundaryTick, m_viewportStartTick, m_viewportEndTick);

                const AZStd::string label =
                    AZStd::string::format("%.2f ms", CpuProfilerImGuiHelper::TicksToMs(nextFrameBoundaryTick - lastFrameBoundaryTick));
                const float labelWidth = ImGui::CalcTextSize(label.c_str()).x;

                // The label can fit between the two boundaries, center it and draw
                if (labelWidth <= nextFrameBoundaryPixel - lastFrameBoundaryPixel) 
                {
                    const float offset = (nextFrameBoundaryPixel - lastFrameBoundaryPixel - labelWidth) /2;
                    const float textBeginPixel = lastFrameBoundaryPixel + offset;
                    const float textEndPixel = textBeginPixel + labelWidth;

                    const float verticalOffset = (ImGui::GetWindowHeight() - ImGui::GetFontSize()) / 2;

                    // Execution time label
                    drawList->AddText({ textBeginPixel, wy + verticalOffset }, IM_COL32_WHITE, label.c_str());

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
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const auto [wx, wy] = ImGui::GetWindowPos();
            const ImU32 orange = ImGui::GetColorU32({ 1, .7, 0, 1 });
            const ImU32 red = ImGui::GetColorU32({ 1, 0, 0, 1 });

            const AZStd::sys_time_t ticksPerSecond = AZStd::GetTimeTicksPerSecond();
            const AZStd::sys_time_t viewportCenter = m_viewportEndTick - (m_viewportEndTick - m_viewportStartTick) / 2;
            const AZStd::sys_time_t leftHistogramBound = viewportCenter - ticksPerSecond;
            const AZStd::sys_time_t rightHistogramBound = viewportCenter + ticksPerSecond;

            // Draw frame limit lines 
            drawList->AddLine(
                { wx, wy + ImGui::GetWindowHeight() - MediumFrameTimeLimit },
                { wx + ImGui::GetWindowWidth(), wy + ImGui::GetWindowHeight() - MediumFrameTimeLimit },
                orange);

            drawList->AddLine(
                { wx, wy + ImGui::GetWindowHeight() - HighFrameTimeLimit },
                { wx + ImGui::GetWindowWidth(), wy + ImGui::GetWindowHeight() - HighFrameTimeLimit },
                red);


            // Draw viewport bound rectangle
            const float leftViewportPixel = ConvertTickToPixelSpace(m_viewportStartTick, leftHistogramBound, rightHistogramBound);
            const float rightViewportPixel = ConvertTickToPixelSpace(m_viewportEndTick, leftHistogramBound, rightHistogramBound);
            const ImVec2 topLeftPos = { leftViewportPixel, wy };
            const ImVec2 botRightPos = { rightViewportPixel, wy + ImGui::GetWindowHeight() };
            const ImU32 gray = ImGui::GetColorU32({ 1, 1, 1, .3 });
            drawList->AddRectFilled(topLeftPos, botRightPos, gray);

            // Find the first onscreen frame execution time 
            auto frameEndTickItr = AZStd::lower_bound(m_frameEndTicks.begin(), m_frameEndTicks.end(), leftHistogramBound);
            if (frameEndTickItr != m_frameEndTicks.begin())
            {
                --frameEndTickItr;
            }

            // Since we only store the frame end ticks, we must calculate the execution times on the fly by comparing pairs of elements. 
            AZStd::sys_time_t lastFrameEndTick = *frameEndTickItr;
            while (*frameEndTickItr < rightHistogramBound && ++frameEndTickItr != m_frameEndTicks.end())
            {
                const AZStd::sys_time_t frameEndTick = *frameEndTickItr;

                const float framePixelPos = ConvertTickToPixelSpace(frameEndTick, leftHistogramBound, rightHistogramBound);
                const float frameTimeMs = CpuProfilerImGuiHelper::TicksToMs(frameEndTick - lastFrameEndTick);

                const ImVec2 lineBottom = { framePixelPos, ImGui::GetWindowHeight() + wy };
                const ImVec2 lineTop = { framePixelPos, ImGui::GetWindowHeight() + wy - frameTimeMs };

                ImU32 lineColor = ImGui::GetColorU32({ .3, .3, .3, 1 }); // Gray
                if (frameTimeMs > HighFrameTimeLimit)
                {
                    lineColor = ImGui::GetColorU32({1, 0, 0, 1}); // Red
                }
                else if (frameTimeMs > MediumFrameTimeLimit)
                {
                    lineColor = ImGui::GetColorU32({1, .7, 0, 1}); // Orange
                }

                drawList->AddLine(lineBottom, lineTop, lineColor, 3.0);

                lastFrameEndTick = frameEndTick;
            }

            // Handle input
            ImGui::InvisibleButton("HistogramInputCapture", { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() });
            ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                const float mousePixelX = io.MousePos.x;
                const float percentWindow = (mousePixelX - wx) / ImGui::GetWindowWidth();
                const AZStd::sys_time_t newViewportCenterTick = leftHistogramBound +
                    aznumeric_cast<AZStd::sys_time_t>((rightHistogramBound - leftHistogramBound) * percentWindow);

                const AZStd::sys_time_t viewportWidth = GetViewportTickWidth();
                m_viewportEndTick = newViewportCenterTick + viewportWidth / 2;
                m_viewportStartTick = newViewportCenterTick - viewportWidth / 2;
            }
        }

        inline AZStd::sys_time_t ImGuiCpuProfiler::GetViewportTickWidth() const
        {
            return m_viewportEndTick - m_viewportStartTick;
        }

        inline float ImGuiCpuProfiler::ConvertTickToPixelSpace(AZStd::sys_time_t tick, AZStd::sys_time_t leftBound, AZStd::sys_time_t rightBound) const
        {
            const float wx = ImGui::GetWindowPos().x;
            const float tickSpaceShifted = aznumeric_cast<float>(tick - leftBound); // This will be close to zero, so FP inaccuracy should not be too bad
            const float tickSpaceNormalized = tickSpaceShifted / (rightBound - leftBound);
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
                m_frameEndTicks.push_back(AZStd::GetTimeNowTicks());

                for (auto& [groupName, regionMap] : m_groupRegionMap)
                {
                    for (auto& [regionName, row] : regionMap)
                    {
                        row.ResetPerFrameStatistics();
                    }
                }
            }
        }

        // ---- TableRow impl ----

        inline void TableRow::RecordRegion(const AZ::RHI::CachedTimeRegion& region, size_t threadId)
        {
            const AZStd::sys_time_t deltaTime = region.m_endTick - region.m_startTick;

            // Update per frame statistics
            ++m_invocationsLastFrame;
            m_executingThreads.insert(threadId);
            m_lastFrameTotalTicks += deltaTime;
            m_maxTicks = AZStd::max(m_maxTicks, deltaTime);

            // Update aggregate statistics
            m_runningAverageTicks =
                aznumeric_cast<AZStd::sys_time_t>((1.0 * (deltaTime + m_invocationsTotal * m_runningAverageTicks)) / (m_invocationsTotal + 1));
            ++m_invocationsTotal;
        }

        inline void TableRow::ResetPerFrameStatistics()
        {
            m_invocationsLastFrame = 0;
            m_executingThreads.clear();
            m_lastFrameTotalTicks = 0;
            m_maxTicks = 0;
        }

        inline AZStd::string TableRow::GetExecutingThreadsLabel() const 
        {
            auto threadString = AZStd::string::format("Executed in %zu threads\n", m_executingThreads.size());
            for (const auto& threadId : m_executingThreads)
            {
                threadString.append(AZStd::string::format("Thread: %zu\n", threadId));
            }
            return threadString;
        }
    } // namespace Render
} // namespace AZ
