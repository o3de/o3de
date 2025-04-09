/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/ImGuiGpuProfiler.h>

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <Atom/RHI.Reflect/MemoryStatistics.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

#include <Profiler/ImGuiTreemap.h>
#include <imgui/imgui_internal.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/time.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

#include <inttypes.h>

namespace AZ
{
    namespace Render
    {
        namespace GpuProfilerImGuiHelper
        {
            template<typename T>
            static void TreeNode(const char* label, ImGuiTreeNodeFlags flags, T&& functor)
            {
                const bool unrolledTreeNode = ImGui::TreeNodeEx(label, flags);
                functor(unrolledTreeNode);

                if (unrolledTreeNode)
                {
                    ImGui::TreePop();
                }
            }

            template <typename Functor>
            static void Begin(const char* name, bool* open, ImGuiWindowFlags flags, Functor&& functor)
            {
                if (ImGui::Begin(name, open, flags))
                {
                    functor();
                }
                ImGui::End();
            }

            template <typename Functor>
            static void BeginChild(const char* text, const ImVec2& size, bool border, ImGuiWindowFlags flags, Functor&& functor)
            {
                if (ImGui::BeginChild(text, size, border, flags))
                {
                    functor();
                }
                ImGui::EndChild();
            }

            static void HoverMarker(const char* text)
            {
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(text);
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }

            template <typename Functor>
            static void PushStyleColor(ImGuiCol idx, const ImVec4& color, Functor&& functor)
            {
                ImGui::PushStyleColor(idx, color);
                functor();
                ImGui::PopStyleColor();
            }

            template <typename Functor>
            static void WrappableSelectable(const char* text, ImVec2 size, bool selected, ImGuiSelectableFlags flags, Functor&& functor)
            {
                ImFont* font = ImGui::GetFont();
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                const ImVec2 pos = ImGui::GetCursorScreenPos();

                const AZStd::string label = AZStd::string::format("%s%s", "##hidden", text);
                if (ImGui::Selectable(label.c_str(), selected, flags, size))
                {
                    functor();
                }
                drawList->AddText(font, font->FontSize, pos, ImGui::GetColorU32(ImGuiCol_Text), text, nullptr, size.x);
            }

            static AZStd::string GetImageBindStrings(AZ::RHI::ImageBindFlags imageBindFlags)
            {
                AZStd::string imageBindStrings;
                for (const auto& flag : AZ::RHI::ImageBindFlagsMembers)
                {
                    if (flag.m_value != AZ::RHI::ImageBindFlags::None && AZ::RHI::CheckBitsAll(imageBindFlags, flag.m_value))
                    {
                        imageBindStrings.append(flag.m_string);
                        imageBindStrings.append(", ");
                    }
                }
                return imageBindStrings;
            }

            static AZStd::string GetBufferBindStrings(AZ::RHI::BufferBindFlags bufferBindFlags)
            {
                AZStd::string bufferBindStrings;
                for (const auto& flag : AZ::RHI::BufferBindFlagsMembers)
                {
                    if (flag.m_value != AZ::RHI::BufferBindFlags::None && AZ::RHI::CheckBitsAll(bufferBindFlags, flag.m_value))
                    {
                        bufferBindStrings.append(flag.m_string);
                        bufferBindStrings.append(", ");
                    }
                }
                return bufferBindStrings;
            }

            static constexpr u64 KB = 1024;
            static constexpr u64 MB = 1024 * KB;
        } // namespace GpuProfilerImGuiHelper 

        // --- PassEntry ---

        PassEntry::PassEntry(const RPI::Pass* pass, PassEntry* parent)
        {
            m_name = pass->GetName();
            m_path = pass->GetPathName();
            m_parent = parent;
            m_enabled = pass->IsEnabled();
            m_deviceIndex = pass->GetDeviceIndex() == -1 ? RHI::MultiDevice::DefaultDeviceIndex : pass->GetDeviceIndex();
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

        void PassEntry::LinkChild(PassEntry* childEntry)
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

        void PassEntry::PropagateDeviceIndex(int deviceIndex)
        {
            m_childrenDeviceIndices.insert(deviceIndex);
            if (m_parent)
            {
                m_parent->PropagateDeviceIndex(deviceIndex);
            }
        }

        bool PassEntry::IsTimestampEnabled() const
        {
            return m_enabled && m_timestampEnabled;
        }

        bool PassEntry::IsPipelineStatisticsEnabled() const
        {
            return m_enabled && m_pipelineStatisticsEnabled;
        }

        // --- ImGuiPipelineStatisticsView ---

        ImGuiPipelineStatisticsView::ImGuiPipelineStatisticsView() :
            m_headerColumnWidth{ 204.0f, 104.0f, 104.0f, 104.0f, 104.0f, 104.0f, 104.0f, 104.0f }
        {

        }

        void ImGuiPipelineStatisticsView::DrawPipelineStatisticsWindow(bool& draw,
            const PassEntry* rootPassEntry, AZStd::unordered_map<Name, PassEntry>& passEntryDatabase,
            AZ::RHI::Ptr<RPI::ParentPass> rootPass)
        {
            // Early out if nothing is supposed to be drawn
            if (!draw)
            {
                return;
            }

            AZ_Assert(rootPassEntry, "RootPassEntry is invalid.");

            // The PipelineStatistics attribute names.
            static const char* PipelineStatisticsAttributeHeader[HeaderAttributeCount] = {
                "Pass Name", "Vertex Count", "Primitive Count", "Vertex Shader Invocation Count", "Rasterized Primitive Count",
                "Rendered Primitive Count", "Pixel Shader Invocation Count", "Compute Shader Invocation Count"
            };

            // Additional filter to exclude passes from the list.
            static const AZStd::array<AZStd::string, 2> ExcludeFilter = { "Root", "MainPipeline" };

            // Clear the references array from the previous frame.
            m_passEntryReferences.clear();

            // Filter the PassEntries.
            {
                m_passEntryReferences.reserve(passEntryDatabase.size());
                for (auto& passEntryIt : passEntryDatabase)
                {
                    const PassEntry& passEntry = passEntryIt.second;

                    // Filter depending on the user input.
                    if (!m_passFilter.PassFilter(passEntry.m_name.GetCStr()))
                    {
                        continue;
                    }

                    // Filter out parent passes if necessary.
                    if (!m_showParentPasses && passEntry.m_isParent)
                    {
                        continue;
                    }

                    // Filter with the ExcludeFilter.
                    if (m_excludeFilterEnabled)
                    {
                        const auto filterIt = AZStd::find_if(ExcludeFilter.begin(), ExcludeFilter.end(), [&passEntry](const AZStd::string& passName)
                        {
                            return passName == passEntry.m_name.GetStringView();
                        });

                        if (filterIt != ExcludeFilter.end())
                        {
                            continue;
                        }
                    }

                    // Add the PassEntry if it passes both filters.
                    m_passEntryReferences.push_back(&passEntry);
                }
            }


            // Sort the PassEntries.
            SortView();

            // Set the window size.
            const ImVec2 windowSize(964.0f, 510.0f);
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

            // Start drawing the PipelineStatistics window.
            if (ImGui::Begin("PipelineStatistics Window", &draw, ImGuiWindowFlags_None))
            {
                // Pause/unpause the profiling
                if (ImGui::Button(m_paused ? "Resume" : "Pause"))
                {
                    m_paused = !m_paused;
                    rootPass->SetPipelineStatisticsQueryEnabled(!m_paused);
                }

                ImGui::Columns(2, "HeaderColumns");

                // Draw the statistics of the RootPass.
                {
                    ImGui::Text("Information");
                    ImGui::Spacing();

                    // General information.
                    {
                        // Display total pass count.
                        const AZStd::string totalPassCountLabel = AZStd::string::format("%s: %u",
                            "Total Pass Count",
                            static_cast<uint32_t>(passEntryDatabase.size()));
                        ImGui::Text("%s", totalPassCountLabel.c_str());

                        // Display listed pass count.
                        const AZStd::string listedPassCountLabel = AZStd::string::format("%s: %u",
                            "Listed Pass Count",
                            static_cast<uint32_t>(m_passEntryReferences.size()));
                        ImGui::Text("%s", listedPassCountLabel.c_str());
                    }
                }

                ImGui::NextColumn();

                // Options
                GpuProfilerImGuiHelper::TreeNode("Options", ImGuiTreeNodeFlags_None, [this](bool unrolled)
                {
                    if (unrolled)
                    {
                        // Draw the advanced Options node.
                        ImGui::Checkbox("Enable color-coding", &m_enableColorCoding);
                        ImGui::Checkbox("Remove RootPasses from the list", &m_excludeFilterEnabled);
                        ImGui::Checkbox("Show attribute contribution", &m_showAttributeContribution);
                        ImGui::Checkbox("Show pass' tree state", &m_showPassTreeState);
                        ImGui::Checkbox("Show disabled passes", &m_showDisabledPasses);
                        ImGui::Checkbox("Show parent passes", &m_showParentPasses);
                    }
                });

                ImGui::Columns(1, "HeaderColumns");

                ImGui::Separator();

                // Draw the filter.
                m_passFilter.Draw("Pass Name Filter");

                // Draw the attribute matrix header.
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
                    ImGui::Columns(HeaderAttributeCount, "PipelineStatisticsHeader", false);

                    // Calculate the text which requires the most height.
                    float maxColumnHeight = 0.0f;
                    for (uint32_t headerIdx = 0u; headerIdx < HeaderAttributeCount; headerIdx++)
                    {
                        ImGui::SetColumnWidth(static_cast<int32_t>(headerIdx), m_headerColumnWidth[headerIdx]);

                        const char* text = PipelineStatisticsAttributeHeader[headerIdx];
                        const ImVec2 textSize = ImGui::CalcTextSize(text, nullptr, false, m_headerColumnWidth[headerIdx]);
                        maxColumnHeight = AZStd::max(textSize.y, maxColumnHeight);
                    }

                    // Create the header text.
                    for (uint32_t headerIdx = 0u; headerIdx < HeaderAttributeCount; headerIdx++)
                    {
                        const char* text = PipelineStatisticsAttributeHeader[headerIdx];
                        const ImVec2 selectableSize = { m_headerColumnWidth[headerIdx], maxColumnHeight };

                        // Sort when the selectable is clicked.
                        bool columnSelected = (headerIdx == GetSortIndex());
                        GpuProfilerImGuiHelper::WrappableSelectable(text, selectableSize, columnSelected, ImGuiSelectableFlags_None, [&, this]()
                        {
                            // Sort depending on the column index.
                            const uint32_t sortIndex = GetSortIndex();
                            // When the sort index is equal to the header index, it means that the same column has been selected, which
                            // results in sorting the items in a inverted manner depending on the column's attribute.
                            if (columnSelected)
                            {
                                const uint32_t baseSortIndex = sortIndex * SortVariantPerColumn;
                                m_sortIndex = baseSortIndex + ((m_sortIndex + 1u) % SortVariantPerColumn);
                            }
                            else
                            {
                                // When the current header index and sort index are different, it means that a different column has been selected,
                                // which results in sorting the items depending on the most recently selected column's attribute.
                                m_sortIndex = headerIdx * SortVariantPerColumn;
                            }
                        });

                        ImGui::NextColumn();
                    }

                    // Draw the RootPass' attribute row.
                    CreateAttributeRow(rootPassEntry, nullptr);

                    ImGui::Columns(1);
                    ImGui::PopStyleVar();
                }

                // Draw the child window, consisting of the body of the matrix.
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
                    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar;

                    GpuProfilerImGuiHelper::BeginChild("AttributeMatrix", ImVec2(ImGui::GetWindowContentRegionWidth(), 320), false, window_flags, [&, this]()
                    {
                        ImGui::Columns(HeaderAttributeCount, "PipelineStatsisticsBody", false);

                        for (const auto passEntry : m_passEntryReferences)
                        {
                            CreateAttributeRow(passEntry, rootPassEntry);
                        }

                        ImGui::Columns(1, "PipelineStatsisticsBody");
                    });
                    ImGui::PopStyleVar();
                }
            }
            ImGui::End();
        }

        void ImGuiPipelineStatisticsView::CreateAttributeRow(const PassEntry* passEntry, const PassEntry* rootEntry)
        {
            [[maybe_unused]] const uint32_t columnCount = static_cast<uint32_t>(ImGui::GetColumnsCount());
            AZ_Assert(columnCount == ImGuiPipelineStatisticsView::HeaderAttributeCount, "The column count needs to match HeaderAttributeCount.");

            ImGui::Separator();

            // Draw the pass name.
            {
                AZStd::string passName(passEntry->m_name.GetCStr());
                if (m_showPassTreeState)
                {
                    const char* passTreeState = passEntry->m_isParent ? "Parent" : "Child";
                    passName = AZStd::string::format("%s (%s)", passName.c_str(), passTreeState);
                }

                ImGui::Text("%s", passName.c_str());

                // Show a HoverMarker if the text is bigger than the column.
                const ImVec2 textSize = ImGui::CalcTextSize(passName.c_str());
                const uint32_t passNameIndex = 0u;

                // Set the column width.
                ImGui::SetColumnWidth(passNameIndex, m_headerColumnWidth[passNameIndex]);

                // Create a hover marker when the pass name exceeds the column width.
                if (textSize.x > m_headerColumnWidth[passNameIndex])
                {
                    GpuProfilerImGuiHelper::HoverMarker(passName.c_str());
                }
            }

            ImGui::NextColumn();

            // Change the value(hsv) according to the normalized value.
            for (int32_t attributeIdx = 0; attributeIdx < PassEntry::PipelineStatisticsAttributeCount; attributeIdx++)
            {
                // Set the width of the column depending on the header column.
                const int32_t attributeHeaderIndex = attributeIdx + 1;
                ImGui::SetColumnWidth(attributeHeaderIndex, m_headerColumnWidth[attributeHeaderIndex]);

                // Calculate the normalized value if the RootEntry is valid.
                float normalized = 0.0f;
                if (rootEntry)
                {
                    const double attributeLimit = static_cast<double>(rootEntry->m_pipelineStatistics[attributeIdx]);
                    const double attribute = static_cast<double>(passEntry->m_pipelineStatistics[attributeIdx]);
                    normalized = static_cast<float>(attribute / attributeLimit);
                }

                // Color code the cell depending on the contribution of the attribute to the attribute limit.
                ImVec4 rgb = { 0.0f, 0.0f, 0.0f, 1.0f };
                if (m_enableColorCoding)
                {
                    // Interpolate in HSV, then convert hsv to rgb.
                    const ImVec4 hsv = { 161.0f, 95.0f, normalized * 80.0f, 0.0f };
                    ImGui::ColorConvertHSVtoRGB(hsv.x / 360.0f, hsv.y / 100.0f, hsv.z / 100.0f, rgb.x, rgb.y, rgb.z);
                }

                // Draw the attribute cell.
                GpuProfilerImGuiHelper::PushStyleColor(ImGuiCol_Header, rgb, [&, this]()
                {
                    // Threshold to determine if a text needs to change to black.
                    const float changeTextColorThreshold = 0.9f;

                    // Make the text black if the cell becomes too bright.
                    const bool textColorChanged = m_enableColorCoding && normalized > changeTextColorThreshold;
                    if (textColorChanged)
                    {
                        const ImVec4 black = { 0.0f, 0.0f, 0.0f, 1.0f };
                        ImGui::PushStyleColor(ImGuiCol_Text, black);
                    }

                    AZStd::string label;
                    if (rootEntry && m_showAttributeContribution)
                    {
                        label = AZStd::string::format("%llu (%u%%)",
                            static_cast<AZ::u64>(passEntry->m_pipelineStatistics[attributeIdx]),
                            static_cast<uint32_t>(normalized * 100.0f));
                    }
                    else
                    {
                        label = AZStd::string::format("%llu",
                            static_cast<AZ::u64>(passEntry->m_pipelineStatistics[attributeIdx]));
                    }

                    if (rootEntry)
                    {
                        ImGui::Selectable(label.c_str(), true);
                    }
                    else
                    {
                        ImGui::Text("%s", label.c_str());
                    }

                    if (textColorChanged)
                    {
                        ImGui::PopStyleColor();
                    }
                });

                ImGui::NextColumn();
            }
        }

        void ImGuiPipelineStatisticsView::SortView()
        {
            const StatisticsSortType sortType = GetSortType();

            if (sortType == StatisticsSortType::Alphabetical)
            {
                // Sort depending on the PassEntry's names.
                AZStd::sort(m_passEntryReferences.begin(), m_passEntryReferences.end(), [this](const PassEntry* left, const PassEntry* right)
                {
                    if (IsSortStateInverted())
                    {
                        AZStd::swap(left, right);
                    }

                    return left->m_name.GetStringView() < right->m_name.GetStringView();
                });
            }
            else if (sortType == StatisticsSortType::Numerical)
            {
                // Sort depending on a numerical attribute.
                AZStd::sort(m_passEntryReferences.begin(), m_passEntryReferences.end(), [this](const PassEntry* left, const PassEntry* right)
                {
                    if (IsSortStateInverted())
                    {
                        AZStd::swap(left, right);
                    }
                    const uint32_t sortingIndex = GetSortIndex();
                    AZ_Assert(sortingIndex != 0u, "Trying to sort on name");

                    return left->m_pipelineStatistics[sortingIndex - 1u] > right->m_pipelineStatistics[sortingIndex - 1u];
                });
            }
        }

        uint32_t ImGuiPipelineStatisticsView::GetSortIndex() const
        {
            return m_sortIndex / SortVariantPerColumn;
        }

        ImGuiPipelineStatisticsView::StatisticsSortType ImGuiPipelineStatisticsView::GetSortType() const
        {
            // The first column (Pass Name) is the only column that requires the items to be sorted in an alphabetic manner.
            if (GetSortIndex() == 0u)
            {
                return StatisticsSortType::Alphabetical;
            }
            else
            {
                return StatisticsSortType::Numerical;
            }
        }

        bool ImGuiPipelineStatisticsView::IsSortStateInverted() const
        {
            return m_sortIndex % SortVariantPerColumn;
        }

        // --- ImGuiTimestampView ---

        void ImGuiTimestampView::DrawTimestampWindow(
            bool& draw, const PassEntry* rootPassEntry, AZStd::unordered_map<Name, PassEntry>& timestampEntryDatabase,
            AZ::RHI::Ptr<RPI::ParentPass> rootPass)
        {
            // Early out if nothing is supposed to be drawn
            if (!draw)
            {
                return;
            }

            auto* rhiSystem = AZ::RHI::RHISystemInterface::Get();

            // Clear the references from the previous frame.
            m_passEntryReferences.clear();

            struct PerDevicePassData
            {
                // pass entry grid based on its timestamp
                AZStd::vector<PassEntry*> sortedPassEntries;
                AZStd::vector<AZStd::vector<PassEntry*>> sortedPassGrid;
                RPI::TimestampResult gpuTimestamp;
                int64_t deviceReferenceDuration{};
                int64_t hostReferenceDuration{};
            };

            AZStd::map<int, PerDevicePassData> passEntriesMap;

            // Set the child of the parent, only if it passes the filter.
            for (auto& [passName, passEntry] : timestampEntryDatabase)
            {
                // Collect all pass entries with non-zero durations
                if (passEntry.m_timestampResult.GetDurationInTicks() > 0)
                {
                    auto it{ passEntriesMap.find(passEntry.m_deviceIndex) };
                    if (it == passEntriesMap.end())
                    {
                        it = passEntriesMap.insert({ passEntry.m_deviceIndex, PerDevicePassData{} }).first;
                    }
                    it->second.sortedPassEntries.push_back(&passEntry);
                }

                // Skip the pass if the pass' timestamp duration is 0
                if (m_hideZeroPasses && (!passEntry.m_isParent) && passEntry.m_timestampResult.GetDurationInTicks() == 0)
                {
                    continue;
                }

                // Only add pass if it pass the filter.
                if (m_passFilter.PassFilter(passEntry.m_name.GetCStr()))
                {
                    if (passEntry.m_parent && !passEntry.m_linked)
                    {
                        passEntry.m_parent->LinkChild(&passEntry);
                        passEntry.m_parent->PropagateDeviceIndex(passEntry.m_deviceIndex);
                    }

                    AZ_Assert(
                        m_passEntryReferences.size() < TimestampEntryCount,
                        "Too many PassEntry references. Increase the size of the array.");
                    m_passEntryReferences.push_back(&passEntry);
                }
            }

            /*
                In order to compare timestamps recorded on multiple devices to one another, they need to be related to a common reference
               frame. To this end, calibrated timestamps are recorded for each device, which represent a "simultaneous" timestamp on both
               CPU and a specified GPU. With these timestamps at hand at hand, device timestamps are first related to the common CPU time
               reference frame to allow for proper positioning and scaling of the resulting timestamp bars. The final values are again
               displayed as device timestamps in the end.
            */
            int64_t minimumHostTime{ INT64_MAX };
            int64_t maximumHostTime{ INT64_MIN };

            for (auto& [deviceIndex, passEntries] : passEntriesMap)
            {
                // Only calibrate when taking new measurements to prevent flickering
                if (!m_paused || m_calibratedTimestamps[deviceIndex] == m_lastCalibratedTimestamps[deviceIndex])
                {
                    m_calibratedTimestamps[deviceIndex] = rhiSystem->GetDevice(deviceIndex)->GetCalibratedTimestamp();
                }

                if (m_lastCalibratedTimestamps.find(deviceIndex) == m_lastCalibratedTimestamps.end())
                {
                    m_lastCalibratedTimestamps[deviceIndex] = { 0, 0 };
                }

                auto& [calibratedTimestampDevice, calibratedTimestampHost] = m_calibratedTimestamps[deviceIndex];
                auto& [lastCalibratedTimestampDevice, lastCalibratedTimestampHost] = m_lastCalibratedTimestamps[deviceIndex];

                // Calculate the scaling factor to go from a host to a device timestamp
                passEntries.deviceReferenceDuration = calibratedTimestampDevice - lastCalibratedTimestampDevice;
                passEntries.hostReferenceDuration = calibratedTimestampHost - lastCalibratedTimestampHost;

                // Sort the pass entries based on their starting time and duration
                AZStd::sort(
                    passEntries.sortedPassEntries.begin(),
                    passEntries.sortedPassEntries.end(),
                    [](const PassEntry* passEntry1, const PassEntry* passEntry2)
                    {
                        if (passEntry1->m_timestampResult.GetTimestampBeginInTicks() ==
                            passEntry2->m_timestampResult.GetTimestampBeginInTicks())
                        {
                            return passEntry1->m_timestampResult.GetDurationInTicks() < passEntry2->m_timestampResult.GetDurationInTicks();
                        }
                        return passEntry1->m_timestampResult.GetTimestampBeginInTicks() <
                            passEntry2->m_timestampResult.GetTimestampBeginInTicks();
                    });

                uint64_t lastTimestamp{ 0 };
                PassEntry* lastPassEntry{ nullptr };

                // find the maximum length, since the pass that starts last could end earlier than another pass, so the sorting doesn't help
                for (const auto& passEntry : passEntries.sortedPassEntries)
                {
                    uint64_t endTimestamp{ passEntry->m_timestampResult.GetTimestampBeginInTicks() +
                                           passEntry->m_timestampResult.GetDurationInTicks() };

                    if (endTimestamp > lastTimestamp)
                    {
                        lastPassEntry = passEntry;
                        lastTimestamp = endTimestamp;
                    }
                }

                // calculate the total GPU duration.
                if (passEntries.sortedPassEntries.size() > 0)
                {
                    passEntries.gpuTimestamp = passEntries.sortedPassEntries.front()->m_timestampResult;
                    passEntries.gpuTimestamp.Add(lastPassEntry->m_timestampResult);
                }

                // Convert a device timestamp to a host timestamp so that all timestamps are in one reference frame and hence comparable
                auto convertToHostTime{ [lastDeviceTimestamp = lastCalibratedTimestampDevice,
                                         lastHostTimeStamp = lastCalibratedTimestampHost,
                                         deviceReferenceDuration = passEntries.deviceReferenceDuration,
                                         hostReferenceDuration = passEntries.hostReferenceDuration](int64_t timestamp)
                                        {
                                            return (((timestamp - int64_t(lastDeviceTimestamp)) * hostReferenceDuration) /
                                                    deviceReferenceDuration) +
                                                int64_t(lastHostTimeStamp);
                                        } };

                int64_t hostStartTime{ convertToHostTime(passEntries.gpuTimestamp.GetTimestampBeginInTicks()) };
                int64_t hostEndTime{ convertToHostTime(lastTimestamp) };

                if (hostStartTime < minimumHostTime)
                {
                    minimumHostTime = hostStartTime;
                }

                if (hostEndTime > maximumHostTime)
                {
                    maximumHostTime = hostEndTime;
                }

                // Add a pass to the pass grid which none of the pass's timestamp range won't overlap each other.
                // Search each row until the pass can be added to the end of row without overlap the previous one.
                for (auto& passEntry : passEntries.sortedPassEntries)
                {
                    auto row = passEntries.sortedPassGrid.begin();
                    for (; row != passEntries.sortedPassGrid.end(); row++)
                    {
                        if (row->empty())
                        {
                            break;
                        }
                        auto last = (*row).back();
                        if (passEntry->m_timestampResult.GetTimestampBeginInTicks() >=
                            last->m_timestampResult.GetTimestampBeginInTicks() + last->m_timestampResult.GetDurationInTicks())
                        {
                            row->push_back(passEntry);
                            break;
                        }
                    }
                    if (row == passEntries.sortedPassGrid.end())
                    {
                        passEntries.sortedPassGrid.emplace_back().push_back(passEntry);
                    }
                }
            }

            auto hostDuration{ maximumHostTime - minimumHostTime };

            // Refresh timestamp query
            bool needEnable = false;
            if (!m_paused)
            {
                if (m_refreshType == RefreshType::OncePerSecond)
                {
                    auto now = AZStd::GetTimeNowMicroSecond();
                    if (now - m_lastUpdateTimeMicroSecond > 1000000)
                    {
                        needEnable = true;
                        m_lastUpdateTimeMicroSecond = now;
                    }
                }
                else if (m_refreshType == RefreshType::Realtime)
                {
                    needEnable = true;
                }
            }

            if (rootPass->IsTimestampQueryEnabled() != needEnable)
            {
                rootPass->SetTimestampQueryEnabled(needEnable);
            }

            const ImVec2 windowSize(1240.0f, 620.0f);
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);
            if (ImGui::Begin("Timestamp View", &draw, ImGuiWindowFlags_None))
            {
                // Draw the header.
                {
                    // Pause/unpause the profiling
                    if (ImGui::Button(m_paused? "Resume":"Pause"))
                    {
                        m_paused = !m_paused;
                    }

                    // Draw the frame time (GPU).
                    for (auto& [deviceIndex, passEntries] : passEntriesMap)
                    {
                        const AZStd::string formattedTimestamp =
                            FormatTimestampLabel(passEntriesMap[deviceIndex].gpuTimestamp.GetDurationInNanoseconds());
                        const AZStd::string headerFrameTime =
                            AZStd::string::format("Total frame duration (GPU %d): %s", deviceIndex, formattedTimestamp.c_str());
                        ImGui::Text("%s", headerFrameTime.c_str());
                    }

                    // Draw the viewing option.
                    ImGui::RadioButton("Hierarchical", reinterpret_cast<int32_t*>(&m_viewType), static_cast<int32_t>(ProfilerViewType::Hierarchical));
                    ImGui::SameLine();
                    ImGui::RadioButton("Flat", reinterpret_cast<int32_t*>(&m_viewType), static_cast<int32_t>(ProfilerViewType::Flat));

                    // Draw the refresh option
                    ImGui::RadioButton("Realtime", reinterpret_cast<int32_t*>(&m_refreshType), static_cast<int32_t>(RefreshType::Realtime));
                    ImGui::SameLine();
                    ImGui::RadioButton("Once Per Second", reinterpret_cast<int32_t*>(&m_refreshType), static_cast<int32_t>(RefreshType::OncePerSecond));

                    // Show/hide non-parent passes which have zero execution time
                    ImGui::Checkbox("Hide Zero Cost Passes", &m_hideZeroPasses);

                    // Show/hide the timeline bar of all the passes which has non-zero execution time
                    ImGui::Checkbox("Show Timeline", &m_showTimeline);

                    // Draw advanced options.
                    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
                    GpuProfilerImGuiHelper::TreeNode("Advanced options", flags, [this](bool unrolled)
                    {
                        if (unrolled)
                        {
                            // Draw the timestamp metric unit option.
                            ImGui::RadioButton("Timestamp in ms", reinterpret_cast<int32_t*>(&m_timestampMetricUnit), static_cast<int32_t>(TimestampMetricUnit::Milliseconds));
                            ImGui::SameLine();
                            ImGui::RadioButton("Timestamp in ns", reinterpret_cast<int32_t*>(&m_timestampMetricUnit), static_cast<int32_t>(TimestampMetricUnit::Nanoseconds));

                            // Draw the frame load view option.
                            ImGui::RadioButton("Frame load in 30 FPS", reinterpret_cast<int32_t*>(&m_frameWorkloadView), static_cast<int32_t>(FrameWorkloadView::FpsView30));
                            ImGui::SameLine();
                            ImGui::RadioButton("Frame load in 60 FPS", reinterpret_cast<int32_t*>(&m_frameWorkloadView), static_cast<int32_t>(FrameWorkloadView::FpsView60));
                        }
                    });
                }

                ImGui::Separator();

                // Draw the pass entry grid
                for (auto& [deviceIndex, passEntries] : passEntriesMap)
                {
                    if (!passEntries.sortedPassEntries.empty() && m_showTimeline)
                    {
                        const float passBarHeight = 20.f;
                        const float passBarSpace = 3.f;
                        float areaWidth = ImGui::GetContentRegionAvail().x - 20.f;

                        auto& [lastCalibratedTimestampDevice, lastCalibratedTimestampHost] = m_lastCalibratedTimestamps[deviceIndex];

                        ImGui::Text("GPU %d", deviceIndex);
                        AZStd::string childID{ "Timeline" + AZStd::to_string(deviceIndex) };
                        if (ImGui::BeginChild(
                                childID.c_str(),
                                ImVec2(areaWidth, (passBarHeight + passBarSpace) * passEntries.sortedPassGrid.size()),
                                false))
                        {
                            // To compute the correct minimum time per device, shift the minimum host time to the start of its
                            // host time and compute the start tick and end tick for the area for device measurements
                            auto shiftedHostTime = minimumHostTime - int64_t(lastCalibratedTimestampHost);
                            auto areaStartTick =
                                ((shiftedHostTime * passEntries.deviceReferenceDuration) / passEntries.hostReferenceDuration) +
                                int64_t(lastCalibratedTimestampDevice);
                            auto areaDurationInTicks =
                                (hostDuration * passEntries.deviceReferenceDuration) / passEntries.hostReferenceDuration;

                            auto offset = static_cast<int64_t>(static_cast<double>(areaDurationInTicks) * m_timelineOffset);
                            areaStartTick += offset;
                            auto scaledAreaDurationInTicks =
                                static_cast<int64_t>(static_cast<double>(areaDurationInTicks) * m_timelineWindowWidth);

                            float rowStartY = 0.f;
                            for (auto& row : passEntries.sortedPassGrid)
                            {
                                // row start y
                                for (auto passEntry : row)
                                {
                                    // button start and width
                                    float buttonStartX =
                                        (int64_t(passEntry->m_timestampResult.GetTimestampBeginInTicks()) - areaStartTick) * areaWidth /
                                        scaledAreaDurationInTicks;
                                    float buttonWidth =
                                        passEntry->m_timestampResult.GetDurationInTicks() * areaWidth / scaledAreaDurationInTicks;

                                    // If pass duration is too small, it is not visible in the timeline
                                    // Increase the size to at least 1.5f and color them to denote this change
                                    bool tooNarrow{ false };
                                    if (buttonWidth < 1.5f)
                                    {
                                        buttonWidth = 1.5f;
                                        tooNarrow = true;
                                    }

                                    ImGui::SetCursorPosX(buttonStartX);
                                    ImGui::SetCursorPosY(rowStartY);

                                    // If the size or position needed to be modified, color it red to make this clear
                                    if (tooNarrow)
                                    {
                                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f });
                                    }

                                    // Adds a button and the hover colors.
                                    ImGui::Button(passEntry->m_name.GetCStr(), ImVec2(buttonWidth, passBarHeight));

                                    if (tooNarrow)
                                    {
                                        ImGui::PopStyleColor(1);
                                    }

                                    if (ImGui::IsItemHovered())
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::Text("Name: %s", passEntry->m_name.GetCStr());
                                        ImGui::Text("Path: %s", passEntry->m_path.GetCStr());
                                        ImGui::Text(
                                            "Duration in ticks: %llu",
                                            static_cast<AZ::u64>(passEntry->m_timestampResult.GetDurationInTicks()));
                                        ImGui::Text(
                                            "Duration in microsecond: %.3f us",
                                            passEntry->m_timestampResult.GetDurationInNanoseconds() / 1000.f);
                                        ImGui::Text("Relative starting position (0-1): %.3f", buttonStartX / areaWidth);
                                        ImGui::EndTooltip();
                                    }
                                }

                                rowStartY += passBarHeight + passBarSpace;
                            }
                        }
                        ImGui::EndChild();

                        // Control the timeline offset and scale
                        ImGuiIO& io = ImGui::GetIO();
                        if (ImGui::IsWindowFocused() && ImGui::IsItemHovered())
                        {
                            io.WantCaptureMouse = true;
                            static constexpr float STEP_SIZE{ 0.1f };
                            auto timelineXOffsetScale{ (ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x) / areaWidth };
                            if (io.MouseWheel)
                            {
                                auto stepSize{ STEP_SIZE * m_timelineWindowWidth };
                                if (io.MouseWheel > 0.f)
                                {
                                    m_timelineWindowWidth = AZStd::max(m_timelineWindowWidth - stepSize, 0.f);
                                    m_timelineOffset = AZStd::min(m_timelineOffset + (stepSize * timelineXOffsetScale), 1.f);
                                }
                                else
                                {
                                    m_timelineWindowWidth = AZStd::min(m_timelineWindowWidth + stepSize, 1.f);
                                    m_timelineOffset = AZStd::max(m_timelineOffset - (stepSize * timelineXOffsetScale), 0.f);
                                }
                            }
                        }

                        ImGui::Separator();
                    }
                }

                // Reset m_lastCalibratedTimestamps every frame if not paused
                if (!m_paused)
                {
                    for (auto& [deviceIndex, timestamps] : m_calibratedTimestamps)
                    {
                        m_lastCalibratedTimestamps[deviceIndex] = timestamps;
                    }
                }

                // Draw the timestamp view.
                {
                    static const AZStd::array<const char*, static_cast<int32_t>(TimestampMetricUnit::Count)> MetricUnitText =
                    {
                        {
                            "ms",
                            "ns",
                        }
                    };

                    static const AZStd::array<const char*, static_cast<int32_t>(FrameWorkloadView::Count)> FrameWorkloadUnit =
                    {
                        {
                            "30",
                            "60",
                        }
                    };

                    m_passFilter.Draw("Pass Name Filter");

                    float areaWidth = ImGui::GetContentRegionAvail().x / passEntriesMap.size();
                    for (auto& [deviceIndex, passEntries] : passEntriesMap)
                    {
                        AZStd::string childID{ "Passes" + AZStd::to_string(deviceIndex) };

                        if (ImGui::BeginChild(childID.c_str(), ImVec2(areaWidth, 0)))
                        {
                            // Set column settings.
                            ImGui::Columns(3, "view", false);
                            ImGui::SetColumnWidth(0, 340.0f);
                            ImGui::SetColumnWidth(1, 100.0f);

                            if (m_viewType == ProfilerViewType::Hierarchical)
                            {
                                // Set the tab header.
                                {
                                    ImGui::Text("Pass Names");
                                    ImGui::NextColumn();

                                    // Render the text depending on the metric unit.
                                    {
                                        const int32_t timestampMetricUnitNumeric = static_cast<int32_t>(m_timestampMetricUnit);
                                        const AZStd::string metricUnitText =
                                            AZStd::string::format("Time in %s", MetricUnitText[timestampMetricUnitNumeric]);
                                        ImGui::Text("%s", metricUnitText.c_str());
                                        ImGui::NextColumn();
                                    }

                                    // Render the text depending on the metric unit.
                                    {
                                        const int32_t frameWorkloadViewNumeric = static_cast<int32_t>(m_frameWorkloadView);
                                        const AZStd::string frameWorkloadViewText =
                                            AZStd::string::format("Frame workload in %s FPS", FrameWorkloadUnit[frameWorkloadViewNumeric]);
                                        ImGui::Text("%s", frameWorkloadViewText.c_str());
                                        ImGui::NextColumn();
                                    }

                                    ImGui::Separator();
                                }

                                // Draw the hierarchical view.
                                DrawHierarchicalView(rootPassEntry, deviceIndex);
                            }
                            else if (m_viewType == ProfilerViewType::Flat)
                            {
                                // Set the tab header.
                                {
                                    // Check whether it should be sorted by name.
                                    const uint32_t sortType = static_cast<uint32_t>(m_sortType);
                                    AZ_PUSH_DISABLE_WARNING(4296, "-Wunknown-warning-option")
                                    bool sortByName =
                                        (sortType >= static_cast<uint32_t>(ProfilerSortType::Alphabetical) &&
                                         (sortType < static_cast<uint32_t>(ProfilerSortType::AlphabeticalCount)));
                                    AZ_POP_DISABLE_WARNING

                                    if (ImGui::Selectable("Pass Names", sortByName))
                                    {
                                        ToggleOrSwitchSortType(ProfilerSortType::Alphabetical, ProfilerSortType::AlphabeticalCount);
                                    }
                                    ImGui::NextColumn();

                                    if (ImGui::Selectable("Time in ms", !sortByName))
                                    {
                                        ToggleOrSwitchSortType(ProfilerSortType::Timestamp, ProfilerSortType::TimestampCount);
                                    }
                                    ImGui::NextColumn();

                                    const int32_t frameWorkloadViewNumeric = static_cast<int32_t>(m_frameWorkloadView);
                                    const AZStd::string frameWorkloadViewText =
                                        AZStd::string::format("Frame workload in %s FPS", FrameWorkloadUnit[frameWorkloadViewNumeric]);
                                    ImGui::Text("%s", frameWorkloadViewText.c_str());
                                    ImGui::NextColumn();
                                }

                                ImGui::Separator();

                                // Create the sorting buttons.
                                SortFlatView();
                                DrawFlatView(deviceIndex);
                            }
                            else
                            {
                                AZ_Assert(false, "Invalid ViewType.");
                            }

                            // Set back to default.
                            ImGui::Columns(1, "view", false);
                        }
                        ImGui::EndChild();
                        ImGui::SameLine();
                    }
                }
            }
            ImGui::End();

        }

        void ImGuiTimestampView::DrawFrameWorkloadBar(double value) const
        {
            // Interpolate the color of the bar depending on the load.
            const float fvalue = AZStd::clamp(static_cast<float>(value), 0.0f, 1.0f);

            static const Vector3 lowHSV(161.0f / 360.0f, 95.0f / 100.0f, 80.0f / 100.0f);
            static const Vector3 highHSV(1.0f / 360.0f, 68.0f / 100.0f, 80.0f / 100.0f);
            const Vector3 colorHSV = lowHSV + (highHSV - lowHSV) * fvalue;

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, static_cast<ImVec4>(ImColor::HSV(colorHSV.GetX(), colorHSV.GetY(), colorHSV.GetZ())));
            ImGui::ProgressBar(fvalue);
            ImGui::PopStyleColor(1);
        }

        void ImGuiTimestampView::DrawHierarchicalView(const PassEntry* entry, int deviceIndex) const
        {
            const AZStd::string entryTime = FormatTimestampLabel(entry->m_interpolatedTimestampInNanoseconds);

            const auto drawWorkloadBar = [this](const AZStd::string& entryTime, const PassEntry* entry)
            {
                ImGui::NextColumn();
                if (entry->m_isParent)
                {
                    ImGui::NextColumn();
                    ImGui::NextColumn();
                }
                else
                {
                    ImGui::Text("%s", entryTime.c_str());
                    ImGui::NextColumn();
                    DrawFrameWorkloadBar(NormalizeFrameWorkload(entry->m_interpolatedTimestampInNanoseconds));
                    ImGui::NextColumn();
                }
            };

            static const auto createHoverMarker = [](const char* text)
            {
                const ImVec2 textSize = ImGui::CalcTextSize(text);
                const int32_t passNameColumnIndex = 0;
                if (textSize.x + ImGui::GetCursorPosX() > ImGui::GetColumnWidth(passNameColumnIndex))
                {
                    GpuProfilerImGuiHelper::HoverMarker(text);
                }
            };

            if (entry->m_children.empty() && entry->m_deviceIndex == deviceIndex)
            {
                // Draw the workload bar when it doesn't have children.
                ImGui::Text("%s", entry->m_name.GetCStr());
                // Show a HoverMarker if the text is bigger than the column.
                createHoverMarker(entry->m_name.GetCStr());

                drawWorkloadBar(entryTime, entry);
            }
            else if (entry->m_childrenDeviceIndices.contains(deviceIndex))
            {
                // Recursively create another tree node.
                const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
                GpuProfilerImGuiHelper::TreeNode(
                    entry->m_name.GetCStr(),
                    flags,
                    [&drawWorkloadBar, &entryTime, entry, this, deviceIndex](bool unrolled)
                    {
                        // Show a HoverMarker if the text is bigger than the column.
                        createHoverMarker(entry->m_name.GetCStr());

                        drawWorkloadBar(entryTime, entry);

                        if (unrolled)
                        {
                            for (const PassEntry* child : entry->m_children)
                            {
                                DrawHierarchicalView(child, deviceIndex);
                            }
                        }
                    });
            }
        }

        void ImGuiTimestampView::SortFlatView()
        {
            const uint32_t ProfilerSortTypeCount = static_cast<uint32_t>(ProfilerSortType::Count);

            using SortTypeAndFunctionPair = AZStd::pair<ProfilerSortType, AZStd::function<bool(PassEntry*, PassEntry*)>>;
            static const AZStd::array<SortTypeAndFunctionPair, ProfilerSortTypeCount> profilerSortMap =
            {
                {
                    AZStd::make_pair(ProfilerSortType::Alphabetical,        [](PassEntry* left, PassEntry* right) {return left->m_name.GetStringView() < right->m_name.GetStringView(); }),
                    AZStd::make_pair(ProfilerSortType::AlphabeticalInverse, [](PassEntry* left, PassEntry* right) {return left->m_name.GetStringView() > right->m_name.GetStringView(); }),
                    AZStd::make_pair(ProfilerSortType::Timestamp,           [](PassEntry* left, PassEntry* right) {return left->m_interpolatedTimestampInNanoseconds > right->m_interpolatedTimestampInNanoseconds; }),
                    AZStd::make_pair(ProfilerSortType::TimestampInverse,    [](PassEntry* left, PassEntry* right) {return left->m_interpolatedTimestampInNanoseconds < right->m_interpolatedTimestampInNanoseconds; })
                }
            };

            auto it = AZStd::find_if(profilerSortMap.begin(), profilerSortMap.end(), [this](const SortTypeAndFunctionPair& sortTypeAndFunctionPair)
            {
                return sortTypeAndFunctionPair.first == m_sortType;
            });

            AZ_Assert(it != profilerSortMap.end(), "The functor associated with the SortType doesn't exist");

            AZStd::sort(m_passEntryReferences.begin(), m_passEntryReferences.end(), it->second);
        }

        void ImGuiTimestampView::DrawFlatView(int deviceIndex) const
        {
            // Draw the flat view.
            for (const PassEntry* entry : m_passEntryReferences)
            {
                if (entry->m_isParent || entry->m_deviceIndex != deviceIndex)
                {
                    continue;
                }
                const AZStd::string entryTime = FormatTimestampLabel(entry->m_interpolatedTimestampInNanoseconds);

                ImGui::Text("%s", entry->m_name.GetCStr());
                ImGui::NextColumn();
                ImGui::Text("%s", entryTime.c_str());
                ImGui::NextColumn();
                DrawFrameWorkloadBar(NormalizeFrameWorkload(entry->m_interpolatedTimestampInNanoseconds));
                ImGui::NextColumn();
            }
        }

        double ImGuiTimestampView::NanoToMilliseconds(uint64_t nanoseconds) const
        {
            // Nanoseconds to Milliseconds inverse multiplier (1 / 1000000)
            const double inverseMultiplier = 0.000001;
            return static_cast<double>(nanoseconds) * inverseMultiplier;
        }

        void ImGuiTimestampView::ToggleOrSwitchSortType(ProfilerSortType start, ProfilerSortType count)
        {
            const uint32_t startNumerical = static_cast<uint32_t>(start);
            const uint32_t countNumerical = static_cast<uint32_t>(count);
            const uint32_t offset = static_cast<uint32_t>(m_sortType) - startNumerical;

            if (offset < countNumerical)
            {
                // Change the sorting order.
                m_sortType = static_cast<ProfilerSortType>(((offset + 1u) % countNumerical) + startNumerical);
            }
            else
            {
                // Change the sorting type.
                m_sortType = start;
            }
        }

        double ImGuiTimestampView::NormalizeFrameWorkload(uint64_t timestamp) const
        {
            static const AZStd::array<double, static_cast<int32_t>(FrameWorkloadView::Count)> TimestampToViewMap =
            {
                {
                    33000000.0,
                    16000000.0,
                }
            };

            const int32_t frameWorkloadViewNumeric = static_cast<int32_t>(m_frameWorkloadView);
            AZ_Assert(frameWorkloadViewNumeric <= TimestampToViewMap.size(), "The frame workload view is invalid.");

            return static_cast<double>(timestamp) / TimestampToViewMap[frameWorkloadViewNumeric];
        }

        AZStd::string ImGuiTimestampView::FormatTimestampLabel(uint64_t timestamp) const
        {
            if (m_timestampMetricUnit == TimestampMetricUnit::Milliseconds)
            {
                const char* timeFormat = "%.4f %s";
                const char* timeType = "ms";
                const double timestampInMs = NanoToMilliseconds(timestamp);
                return AZStd::string::format(timeFormat, timestampInMs, timeType);
            }
            else if (m_timestampMetricUnit == TimestampMetricUnit::Nanoseconds)
            {
                const char* timeFormat = "%llu %s";
                const char* timeType = "ns";
                return AZStd::string::format(timeFormat, timestamp, timeType);
            }
            else
            {
                return AZStd::string("Invalid");
            }
        }

        // --- ImGuiGpuMemoryView ---

        ImGuiGpuMemoryView::ImGuiGpuMemoryView()
        {
            AZ::IO::Path path = AZ::Utils::GetO3deLogsDirectory().c_str();

            path /= "MemoryCaptures";

            AZ::IO::SystemFile::CreateDir(path.c_str());

            m_memoryCapturePath = path.c_str();
        }

        ImGuiGpuMemoryView::~ImGuiGpuMemoryView()
        {
            if (m_hostTreemap)
            {
                if (auto treemapFactory = Profiler::ImGuiTreemapFactory::Interface::Get())
                {
                    treemapFactory->Destroy(m_hostTreemap);
                    treemapFactory->Destroy(m_deviceTreemap);
                }
            }
        }

        void ImGuiGpuMemoryView::SortPoolTable(ImGuiTableSortSpecs* sortSpecs)
        {
            const bool ascending = sortSpecs->Specs->SortDirection == ImGuiSortDirection_Ascending;
            const ImS16 columnToSort = sortSpecs->Specs->ColumnIndex;

            // Sort by the appropriate column in the table
            switch (columnToSort)
            {
            case (0): // Sort by pool name
                AZStd::sort(m_poolTableRows.begin(), m_poolTableRows.end(),
                    [ascending](const PoolTableRow& lhs, const PoolTableRow& rhs)
                    {
                        const auto lhsParentPool = lhs.m_poolName.GetStringView();
                        const auto rhsParentPool = rhs.m_poolName.GetStringView();
                        return ascending ? lhsParentPool < rhsParentPool : lhsParentPool > rhsParentPool;
                    });
                break;
            case (1): // Sort by pool type
                AZStd::sort(m_poolTableRows.begin(), m_poolTableRows.end(),
                    [ascending](const PoolTableRow& lhs, const PoolTableRow& rhs)
                    {
                        const auto lhsHeapType = lhs.m_deviceHeap ? 0 : 1;
                        const auto rhsHeapType = rhs.m_deviceHeap ? 0 : 1;
                        return ascending ? lhsHeapType < rhsHeapType : lhsHeapType > rhsHeapType;
                    });
                break;
            case (2): // Sort by budget
                AZStd::sort(m_poolTableRows.begin(), m_poolTableRows.end(),
                    [ascending](const PoolTableRow& lhs, const PoolTableRow& rhs)
                    {
                        const float lhsBudget = static_cast<float>(lhs.m_budgetBytes);
                        const float rhsBudget = static_cast<float>(rhs.m_budgetBytes);
                        return ascending ? lhsBudget < rhsBudget : lhsBudget > rhsBudget;
                    });
                break;
            case (3): // Sort by reservation
                AZStd::sort(m_poolTableRows.begin(), m_poolTableRows.end(),
                    [ascending](const PoolTableRow& lhs, const PoolTableRow& rhs)
                    {
                        const float lhsReservation = static_cast<float>(lhs.m_allocatedBytes);
                        const float rhsReservation = static_cast<float>(rhs.m_allocatedBytes);
                        return ascending ? lhsReservation < rhsReservation : lhsReservation > rhsReservation;
                    });
                break;
            case (4): // Sort by residency
                AZStd::sort(m_poolTableRows.begin(), m_poolTableRows.end(),
                    [ascending](const PoolTableRow& lhs, const PoolTableRow& rhs)
                    {
                        const float lhsResidency = static_cast<float>(lhs.m_usedBytes);
                        const float rhsResidency = static_cast<float>(rhs.m_usedBytes);
                        return ascending ? lhsResidency < rhsResidency : lhsResidency > rhsResidency;
                    });
                break;
            case (5): // Sort by fragmentation
                AZStd::sort(m_poolTableRows.begin(), m_poolTableRows.end(),
                    [ascending](const PoolTableRow& lhs, const PoolTableRow& rhs)
                    {
                        const float lhsSize = static_cast<float>(lhs.m_fragmentation);
                        const float rhsSize = static_cast<float>(rhs.m_fragmentation);
                        return ascending ? lhsSize < rhsSize : lhsSize > rhsSize;
                    });
                break;
            }
            sortSpecs->SpecsDirty = false;
        }

        void ImGuiGpuMemoryView::SortResourceTable(ImGuiTableSortSpecs* sortSpecs)
        {
            const bool ascending = sortSpecs->Specs->SortDirection == ImGuiSortDirection_Ascending;
            const ImS16 columnToSort = sortSpecs->Specs->ColumnIndex;

            // Sort by the appropriate column in the table
            switch (columnToSort)
            {
            case (0): // Sorting by parent pool name
                AZStd::sort(m_resourceTableRows.begin(), m_resourceTableRows.end(),
                    [ascending](const ResourceTableRow& lhs, const ResourceTableRow& rhs)
                    {
                        const auto lhsParentPool = lhs.m_parentPoolName.GetStringView();
                        const auto rhsParentPool = rhs.m_parentPoolName.GetStringView();
                        return ascending ? lhsParentPool < rhsParentPool : lhsParentPool > rhsParentPool;
                    });
                break;
            case (1): // Sort by buffer/image name
                AZStd::sort(m_resourceTableRows.begin(), m_resourceTableRows.end(),
                    [ascending](const ResourceTableRow& lhs, const ResourceTableRow& rhs)
                    {
                        const auto lhsName = lhs.m_bufImgName.GetStringView();
                        const auto rhsName = rhs.m_bufImgName.GetStringView();
                        return ascending ? lhsName < rhsName : lhsName > rhsName;
                    });
                break;
            case (2): // Sort by memory usage
                AZStd::sort(m_resourceTableRows.begin(), m_resourceTableRows.end(),
                    [ascending](const ResourceTableRow& lhs, const ResourceTableRow& rhs)
                    {
                        const float lhsSize = static_cast<float>(lhs.m_sizeInBytes);
                        const float rhsSize = static_cast<float>(rhs.m_sizeInBytes);
                        return ascending ? lhsSize < rhsSize : lhsSize > rhsSize;
                    });
                break;
            case (3): // Sort by fragmentation
                AZStd::sort(m_resourceTableRows.begin(), m_resourceTableRows.end(),
                    [ascending](const ResourceTableRow& lhs, const ResourceTableRow& rhs)
                    {
                        const float lhsSize = static_cast<float>(lhs.m_fragmentation);
                        const float rhsSize = static_cast<float>(rhs.m_fragmentation);
                        return ascending ? lhsSize < rhsSize : lhsSize > rhsSize;
                    });
                break;
            }
            sortSpecs->SpecsDirty = false;
        }

        void ImGuiGpuMemoryView::DrawTables()
        {
            if (m_poolTableRows.empty())
            {
                return;
            }

            if (ImGui::CollapsingHeader("Buffer Pools", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                if (ImGui::BeginTable("PoolTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable))
                {
                    ImGui::TableSetupColumn("Pool");
                    ImGui::TableSetupColumn("Heap Type");
                    ImGui::TableSetupColumn("Budget (MB)");
                    ImGui::TableSetupColumn("Allocated (MB)");
                    ImGui::TableSetupColumn("Used (MB)");
                    ImGui::TableSetupColumn("Fragmentation (%)");
                    ImGui::TableSetupColumn("Unique (MB)");
                    ImGui::TableHeadersRow();
                    ImGui::TableNextColumn();

                    ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
                    if (sortSpecs && sortSpecs->SpecsDirty)
                    {
                        SortPoolTable(sortSpecs);
                    }

                    for (const auto& tableRow : m_poolTableRows)
                    {
                        ImGui::Text("%s", tableRow.m_poolName.GetCStr());
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", tableRow.m_deviceHeap ? "Device" : "Host");
                        ImGui::TableNextColumn();
                        ImGui::Text("%.4f", 1.0f * tableRow.m_budgetBytes / GpuProfilerImGuiHelper::MB);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.4f", 1.0f * tableRow.m_allocatedBytes / GpuProfilerImGuiHelper::MB);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.4f", 1.0f * tableRow.m_usedBytes / GpuProfilerImGuiHelper::MB);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.4f", tableRow.m_fragmentation);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.4f", 1.0f * tableRow.m_uniqueBytes / GpuProfilerImGuiHelper::MB);
                        ImGui::TableNextColumn();
                    }
                }
                ImGui::EndTable();
            }

            if (ImGui::CollapsingHeader("Allocations", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                if (ImGui::BeginTable("Table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable))
                {
                    ImGui::TableSetupColumn("Parent pool");
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Size (MB)");
                    ImGui::TableSetupColumn("Fragmentation (%)");
                    ImGui::TableSetupColumn("BindFlags", ImGuiTableColumnFlags_NoSort);
                    ImGui::TableHeadersRow();
                    ImGui::TableNextColumn();

                    ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
                    if (sortSpecs && sortSpecs->SpecsDirty)
                    {
                        SortResourceTable(sortSpecs);
                    }

                    // Draw each row in the table
                    for (const auto& tableRow : m_resourceTableRows)
                    {
                        // Don't draw the row if none of the row's text fields pass the filter
                        if (!m_nameFilter.PassFilter(tableRow.m_parentPoolName.GetCStr())
                            && !m_nameFilter.PassFilter(tableRow.m_bufImgName.GetCStr())
                            && !m_nameFilter.PassFilter(tableRow.m_bindFlags.c_str()))
                        {
                            continue;
                        }

                        ImGui::Text("%s", tableRow.m_parentPoolName.GetCStr());
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", tableRow.m_bufImgName.GetCStr());
                        ImGui::TableNextColumn();
                        ImGui::Text("%.4f", 1.0f * tableRow.m_sizeInBytes / GpuProfilerImGuiHelper::MB);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.4f", tableRow.m_fragmentation);
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", tableRow.m_bindFlags.c_str());
                        ImGui::TableNextColumn();
                    }
                }
                ImGui::EndTable();
            }
        }

        void ImGuiGpuMemoryView::UpdateTableRows()
        {
            // Update the table according to the latest filters applied
            m_poolTableRows.clear();
            m_resourceTableRows.clear();
            for (const auto& pool : m_savedPools)
            {
                Name poolName = pool.m_name.IsEmpty() ? Name("Unnamed pool") : pool.m_name;
                auto& deviceHeapUsage = pool.m_memoryUsage.GetHeapMemoryUsage(AZ::RHI::HeapMemoryLevel::Device);
                auto& hostHeapUsage = pool.m_memoryUsage.GetHeapMemoryUsage(AZ::RHI::HeapMemoryLevel::Host);

                if ((!m_hideEmptyBufferPools || deviceHeapUsage.m_totalResidentInBytes > 0) && deviceHeapUsage.m_totalResidentInBytes < static_cast<size_t>(-1))
                {
                    m_poolTableRows.push_back({ poolName, true, deviceHeapUsage.m_budgetInBytes, deviceHeapUsage.m_totalResidentInBytes,
                                                deviceHeapUsage.m_usedResidentInBytes, deviceHeapUsage.m_fragmentation, deviceHeapUsage.m_uniqueAllocationBytes });
                }
                if ((!m_hideEmptyBufferPools || hostHeapUsage.m_totalResidentInBytes > 0) && hostHeapUsage.m_totalResidentInBytes < static_cast<size_t>(-1))
                {
                    m_poolTableRows.push_back({ poolName, false, hostHeapUsage.m_budgetInBytes, hostHeapUsage.m_totalResidentInBytes,
                                                hostHeapUsage.m_usedResidentInBytes, hostHeapUsage.m_fragmentation, hostHeapUsage.m_uniqueAllocationBytes });
                }

                // Ignore transient pools
                if (!m_includeTransientAttachments && pool.m_name.GetStringView().contains("Transient"))
                {
                    continue;
                }

                if (m_includeBuffers)
                {
                    for (const auto& buf : pool.m_buffers)
                    {
                        const Name bufName = buf.m_name.IsEmpty() ? Name("Unnamed Buffer") : buf.m_name;
                        const AZStd::string flags = GpuProfilerImGuiHelper::GetBufferBindStrings(buf.m_bindFlags);
                        m_resourceTableRows.push_back({ poolName, bufName, buf.m_sizeInBytes, buf.m_fragmentation, flags });
                    }
                }

                if (m_includeImages)
                {
                    for (const auto& img : pool.m_images)
                    {
                        const Name imgName = img.m_name.IsEmpty() ? Name("Unnamed Image") : img.m_name;
                        const AZStd::string flags = GpuProfilerImGuiHelper::GetImageBindStrings(img.m_bindFlags);
                        m_resourceTableRows.push_back({ poolName, imgName, img.m_sizeInBytes, 0.f, flags });
                    }
                }
            }
        }

        void ImGuiGpuMemoryView::DrawPieChart(const AZ::RHI::MemoryStatistics::Heap& heap)
        {
            if (ImGui::BeginChild("PieChart", {150, 150}, true))
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                const auto [wx, wy] = ImGui::GetWindowPos();
                const auto [windowWidth, windowHeight] = ImGui::GetWindowSize();
                const ImVec2 center = { wx + windowWidth / 2, wy + windowHeight / 2 };
                const float radius = windowWidth / 2 - 10;

                // Draw the pie chart
                drawList->AddCircleFilled(center, radius, ImGui::GetColorU32({.3, .3, .3, 1}));
                const float usagePercent = 1.0f * heap.m_memoryUsage.m_totalResidentInBytes / heap.m_memoryUsage.m_budgetInBytes;
                drawList->PathArcTo(center, radius, 0, AZ::Constants::TwoPi * usagePercent); // Clockwise starting from rightmost point
                drawList->PathArcTo(center, 0, 0, 0); // To center
                drawList->PathArcTo(center, radius, 0, 0); // Back to starting position
                drawList->PathFillConvex(ImGui::GetColorU32({ .039, .8, 0.556, 1 }));
                ImGui::Text("%.2f%%", usagePercent * 100);
            }
            ImGui::EndChild();
        }

        void ImGuiGpuMemoryView::PerformCapture()
        {
            // Collect and save new GPU memory usage data
            RHI::RHIMemoryStatisticsInterface* rhiMemStats = RHI::RHIMemoryStatisticsInterface::Get();
            const auto* memoryStatistics = rhiMemStats->GetMemoryStatistics();
            if (memoryStatistics)
            {
                m_savedPools = memoryStatistics->m_pools;
                m_savedHeaps = memoryStatistics->m_heaps;

                // Collect the data into TableRows, ignoring depending on flags
                UpdateTableRows();

                UpdateTreemaps();
            }
        }

        void ImGuiGpuMemoryView::DrawGpuMemoryWindow(bool& draw)
        {
            // Enable GPU memory instrumentation while the window is open. Called every draw frame, but just a bitwise operation so overhead should be low.
            auto* rhiSystem = AZ::RHI::RHISystemInterface::Get();
            AZ_Assert(rhiSystem != nullptr, "Error in drawing GPU memory window: RHI System Interface was nullptr");
            rhiSystem->ModifyFrameSchedulerStatisticsFlags(AZ::RHI::FrameSchedulerStatisticsFlags::GatherMemoryStatistics, draw);

            if (!draw)
            {
                return;
            }
             
            ImGui::SetNextWindowSize({ 600, 600 }, ImGuiCond_Once);
            if (ImGui::Begin("Gpu Memory Profiler", &draw, ImGuiViewportFlags_None))
            {
                if (ImGui::Button("Capture"))
                {
                    m_captureMessage.clear();
                    m_loadedCapturePath.clear();
                    PerformCapture();
                }

                ImGui::SameLine();

                if (ImGui::Button("Save"))
                {
                    if (m_savedPools.empty())
                    {
                        m_captureMessage.clear();
                        PerformCapture();
                    }

                    SaveToJSON();
                }
                ImGui::SameLine();
                constexpr static const char* LoadMemoryCaptureTitle = "Select or input memory capture csv file";
                if (ImGui::Button("Load"))
                {
                    m_captureInput[0] = '\0';
                    m_captureSelection = 0;
                    ImGui::OpenPopup(LoadMemoryCaptureTitle);
                }

                // Always center this window when appearing
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                if (ImGui::BeginPopupModal(LoadMemoryCaptureTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    AZStd::vector<AZ::IO::Path> captures;

                    // Enumerate files in the capture folder
                    auto* base = AZ::IO::FileIOBase::GetInstance();
                    base->FindFiles(
                        m_memoryCapturePath.c_str(), "*.csv",
                        [&captures](const char* path)
                        {
                            captures.emplace_back(path);
                            return true;
                        });
                    base->FindFiles(
                        m_memoryCapturePath.c_str(), "*.json",
                        [&captures](const char* path)
                        {
                            captures.emplace_back(path);
                            return true;
                        });

                    if (captures.empty())
                    {
                        ImGui::Text("No captures found in %s", m_memoryCapturePath.c_str());
                    }
                    else
                    {
                        ImGui::Text("Displaying %zu captures found in %s", captures.size(), m_memoryCapturePath.c_str());

                        // Sort captures in reverse-chronological order
                        AZStd::sort(
                            captures.begin(), captures.end(),
                            [base](const AZ::IO::Path& lhs, const AZ::IO::Path& rhs)
                            {
                                return base->ModificationTime(rhs.c_str()) < base->ModificationTime(lhs.c_str());
                            });

                        // Display 10 entries in a scrolling list box
                        if (ImGui::BeginListBox(
                                "Memory Captures",
                                ImVec2{ ImGui::GetMainViewport()->Size.x * 0.8f, 10 * ImGui::GetTextLineHeightWithSpacing() }))
                        {
                            for (size_t i = 0; i != captures.size(); ++i)
                            {
                                bool selected = i == m_captureSelection;
                                if (ImGui::Selectable(captures[i].c_str(), selected))
                                {
                                    m_captureSelection = i;
                                }

                                if (selected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndListBox();
                        }

                        if (ImGui::Button("Open"))
                        {
                            if (captures[m_captureSelection].Extension() == ".csv")
                            {
                                LoadFromCSV(captures[m_captureSelection].c_str());
                            }
                            else if (captures[m_captureSelection].Extension() == ".json")
                            {
                                LoadFromJSON(captures[m_captureSelection].c_str());
                            }
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    // In addition to the directory selection above, provide a means to input a path directly
                    ImGui::InputText("File Path", m_captureInput, AZ::IO::MaxPathLength);
                    AZStd::string manualInput{ m_captureInput };
                    if (manualInput.empty())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    }

                    if (ImGui::Button("Open File"))
                    {
                        LoadFromCSV(manualInput);
                        ImGui::CloseCurrentPopup();
                    }

                    if (manualInput.empty())
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }

                    if (ImGui::Button("Cancel"))
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                if (!m_loadedCapturePath.empty())
                {
                    ImGui::Text("Viewing data loaded from %s", m_loadedCapturePath.c_str());
                }

                if (!m_captureMessage.empty())
                {
                    ImGui::Text("%s", m_captureMessage.c_str());
                }

                if (m_hostTreemap)
                {
                    ImGui::Checkbox("Show host memory treemap", &m_showHostTreemap);
                    ImGui::SameLine();
                    ImGui::Checkbox("Show device memory treemap", &m_showDeviceTreemap);

                    if (m_showHostTreemap)
                    {
                        m_hostTreemap->Render(20, 40, 800, 600);
                    }

                    if (m_showDeviceTreemap)
                    {
                        m_deviceTreemap->Render(40, 80, 800, 600);
                    }
                }

                if (ImGui::Checkbox("Show buffers", &m_includeBuffers)
                    || ImGui::Checkbox("Show images", &m_includeImages)
                    || ImGui::Checkbox("Show transient attachments", &m_includeTransientAttachments)
                    || ImGui::Checkbox("Hide empty pools", &m_hideEmptyBufferPools))
                {
                    UpdateTableRows(); 
                }

                ImGui::Text("Overall heap usage:");
                const float columnOffset = ImGui::GetWindowWidth() / m_savedHeaps.size();
                float currentX = columnOffset;
                for (const auto& savedHeap : m_savedHeaps)
                {
                    if (ImGui::BeginChild(savedHeap.m_name.GetCStr(), { ImGui::GetWindowWidth() / m_savedHeaps.size(), 250 }), ImGuiWindowFlags_NoScrollbar)
                    {
                        ImGui::Text("%s", savedHeap.m_name.GetCStr());
                        ImGui::Columns(2, "HeapData", true);

                        ImGui::Text("%s", "Used (MB): ");
                        ImGui::NextColumn();
                        ImGui::Text("%.2f", 1.0 * savedHeap.m_memoryUsage.m_usedResidentInBytes.load() / GpuProfilerImGuiHelper::MB);
                        ImGui::NextColumn();

                        ImGui::Text("%s", "Allocated (MB): ");
                        ImGui::NextColumn();
                        ImGui::Text("%.2f", 1.0 * savedHeap.m_memoryUsage.m_totalResidentInBytes.load() / GpuProfilerImGuiHelper::MB);
                        ImGui::NextColumn();

                        ImGui::Text("%s", "Budget (MB): ");
                        ImGui::NextColumn();
                        ImGui::Text("%.2f", 1.0 * savedHeap.m_memoryUsage.m_budgetInBytes / GpuProfilerImGuiHelper::MB);

                        ImGui::Columns(1, "PieChartColumn");
                        DrawPieChart(savedHeap);
                    }
                    ImGui::EndChild();
                    ImGui::SameLine(currentX);
                    currentX += columnOffset;
                }
                ImGui::NewLine();
                ImGui::Separator();

                m_nameFilter.Draw("Search");
                DrawTables();
            }
            ImGui::End();
        }

        void ImGuiGpuMemoryView::UpdateTreemaps()
        {
            if (!m_hostTreemap)
            {
                if (auto treemapFactory = Profiler::ImGuiTreemapFactory::Interface::Get())
                {
                    m_hostTreemap = &treemapFactory->Create(AZ::Name{ "Atom Host Memory Treemap" }, "MiB");
                    m_hostTreemap->AddMask("Hide Unused", 0);
                    m_deviceTreemap = &treemapFactory->Create(AZ::Name{ "Atom Device Memory Treemap" }, "MiB");
                    m_deviceTreemap->AddMask("Hide Unused", 0);
                }
            }

            if (m_hostTreemap)
            {
                using Profiler::TreemapNode;
                AZStd::vector<TreemapNode> hostNodes;
                AZStd::vector<TreemapNode> deviceNodes;

                for (auto& pool : m_savedPools)
                {
                    size_t hostBytes = pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Host).m_totalResidentInBytes;
                    size_t hostResidentBytes = pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Host).m_usedResidentInBytes;
                    size_t deviceBytes = pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device).m_totalResidentInBytes;
                    size_t deviceResidentBytes = pool.m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device).m_usedResidentInBytes;

                    TreemapNode* poolNode = nullptr;

                    // Resource pools are each associated with either a device-local heap, or a host heap. Identify the association and
                    // add constiuent buffers and textures as sub-nodes in the corresponding treemap.
                    if (hostBytes > 0)
                    {
                        poolNode = &hostNodes.emplace_back();
                        poolNode->m_name = pool.m_name;
                    }
                    else if (deviceBytes > 0)
                    {
                        poolNode = &deviceNodes.emplace_back();
                        poolNode->m_name = pool.m_name;
                    }
                    else
                    {
                        continue;
                    }

                    const AZ::Name unusedGroup{ "Unused" };
                    TreemapNode& unusedNode = poolNode->m_children.emplace_back();
                    unusedNode.m_name = "Unused";
                    unusedNode.m_group = unusedGroup;
                    if (hostBytes > 0)
                    {
                        unusedNode.m_weight = static_cast<float>(hostBytes - hostResidentBytes) / GpuProfilerImGuiHelper::MB;
                    }
                    else
                    {
                        unusedNode.m_weight = static_cast<float>(deviceBytes - deviceResidentBytes) / GpuProfilerImGuiHelper::MB;
                    }
                    unusedNode.m_tag = 1;

                    if (pool.m_buffers.empty() && pool.m_images.empty())
                    {
                        continue;
                    }

                    const AZ::Name bufferGroup{ "Buffer" };
                    const AZ::Name textureGroup{ "Texture" };

                    for (auto& buffer : pool.m_buffers)
                    {
                        TreemapNode& child = poolNode->m_children.emplace_back();
                        child.m_name = buffer.m_name;
                        child.m_weight = static_cast<float>(buffer.m_sizeInBytes) / GpuProfilerImGuiHelper::MB;
                        child.m_group = bufferGroup;
                    }

                    for (auto& image : pool.m_images)
                    {
                        TreemapNode& child = poolNode->m_children.emplace_back();
                        child.m_name = image.m_name;
                        child.m_weight = static_cast<float>(image.m_sizeInBytes) / GpuProfilerImGuiHelper::MB;
                        child.m_group = textureGroup;
                    }
                }

                m_hostTreemap->SetRoots(AZStd::move(hostNodes));
                m_deviceTreemap->SetRoots(AZStd::move(deviceNodes));
            }
        }

        void ImGuiGpuMemoryView::SaveToJSON()
        {
            time_t ltime;
            time(&ltime);
            tm today;
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
            localtime_s(&today, &ltime);
#else
            today = *localtime(&ltime);
#endif
            char sTemp[128];
            strftime(sTemp, sizeof(sTemp), "%Y%m%d.%H%M%S", &today);
            AZStd::string filename = AZStd::string::format("%s/GpuMemoryCapture_%s.json", m_memoryCapturePath.c_str(), sTemp);

            AZ::IO::SystemFile outputFile;
            if (!outputFile.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
            {
                m_captureMessage = AZStd::string::format("Failed to open file %s for writing", filename.c_str());
                AZ_Error("ImGuiGpuMemoryView", false, m_captureMessage.c_str());
                return;
            }

            rapidjson::Document doc;

            AZ::RHI::RHIMemoryStatisticsInterface::Get()->WriteResourcePoolInfoToJson(m_savedPools, doc);

            rapidjson::StringBuffer jsonStringBuffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(jsonStringBuffer);
            doc.Accept(writer);

            outputFile.Write(jsonStringBuffer.GetString(), jsonStringBuffer.GetSize());
            outputFile.Close();

            m_captureMessage = AZStd::string::format("Wrote memory capture to %s", filename.c_str());
        }

        void ImGuiGpuMemoryView::LoadFromJSON(const AZStd::string& fileName)
        {
            m_loadedCapturePath.clear();

            auto serializeOutcome = JsonSerializationUtils::ReadJsonFile(fileName);

            if (!serializeOutcome.IsSuccess())
            {
                m_captureMessage = AZStd::string::format("Failed to load memory data from %s, error message = \"%s\"", 
                                                        fileName.c_str(), serializeOutcome.GetError().c_str());
                AZ_Error("ImGuiGpuMemoryView", false, m_captureMessage.c_str());
                return;
            }

            m_loadedCapturePath = fileName;
            rapidjson::Document& doc = serializeOutcome.GetValue();

            auto loadOutcome = AZ::RHI::RHIMemoryStatisticsInterface::Get()->LoadResourcePoolInfoFromJson(
                                                m_savedPools, m_savedHeaps, doc, fileName);
            if (!loadOutcome.IsSuccess())
            {
                m_captureMessage = loadOutcome.GetError();
                return;
            }

            // load from json here
            UpdateTableRows();
            UpdateTreemaps();
        }


        // C4702: Unreachable code
        // MSVC 2022 believes that `return true;` below is unreacahable, which is not true.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
        template <typename T>
        bool parseCSVField(const AZStd::string& field, T& out)
        {
            if constexpr (AZStd::is_same_v<T, int>)
            {
                if (azsscanf(field.c_str(), "%i", &out) != 1)
                {
                    return false;
                }
            }
            else if constexpr (AZStd::is_same_v<T, uint32_t>)
            {
                if (azsscanf(field.c_str(), "%" PRIu32, &out) != 1)
                {
                    return false;
                }
            }
            else if constexpr (AZStd::is_same_v<T, uint64_t>)
            {
                if (azsscanf(field.c_str(), "%" PRIu64, &out) != 1)
                {
                    return false;
                }
            }
            else if constexpr (AZStd::is_same_v<T, AZ::Name>)
            {
                out = AZ::Name{ field.c_str() };
                return true;
            }
            else
            {
                return false;
            }

            return true;
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        static constexpr const char* MemoryCSVHeader =
            "Pool Name, Memory Type (0 == Host : 1 == Device), Allocation Name, Allocation Type (0 == Buffer : "
            "1 == Texture), Byte Size, Flags\n";
        static constexpr size_t MemoryCSVFieldCount = 6;

        void ImGuiGpuMemoryView::LoadFromCSV(const AZStd::string& fileName)
        {
            m_loadedCapturePath.clear();
            AZ::IO::SystemFile fileIn;

            if (!fileIn.Open(fileName.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                return;
            }

            AZStd::string data;
            data.resize_no_construct(fileIn.Length());

            fileIn.Read(fileIn.Length(), data.data());

            AZStd::vector<AZStd::string> lines;
            AZ::StringFunc::Tokenize(data, lines, "\n");

            if (lines.empty())
            {
                m_captureMessage = AZStd::string::format("Attempted to load memory data from %s but file was empty", fileName.c_str());
                AZ_Error("ImGuiGpuMemoryView", false, m_captureMessage.c_str());
                return;
            }

            if (lines[0] + '\n' != MemoryCSVHeader)
            {
                m_captureMessage = AZStd::string::format(
                    "Attempted to load memory data from %s but the CSV header (%s) did not match", fileName.c_str(), MemoryCSVHeader);
                AZ_Error("ImGuiGpuMemoryView", false, m_captureMessage.c_str());
                return;
            }

            m_loadedCapturePath = fileName;
            m_savedHeaps.clear();
            m_savedHeaps.resize(2);
            m_savedHeaps[0].m_name = AZ::Name{ "Host Heap" };
            m_savedHeaps[0].m_heapMemoryType = RHI::HeapMemoryLevel::Host;
            m_savedHeaps[1].m_name = AZ::Name{ "Device Heap" };
            m_savedHeaps[1].m_heapMemoryType = RHI::HeapMemoryLevel::Device;

            m_savedPools.clear();
            AZStd::unordered_map<AZ::Name, AZ::RHI::MemoryStatistics::Pool> pools;

            AZStd::vector<AZStd::string> fields;
            fields.reserve(MemoryCSVFieldCount);

            for (size_t i = 1; i != lines.size(); ++i)
            {
                fields.clear();
                const AZStd::string& line = lines[i];
                AZ::Name poolName;
                int memoryType;
                AZ::Name resourceName;
                int resourceType;
                uint64_t byteSize;
                uint32_t bindFlags;

                AZ::StringFunc::Tokenize(line, fields, ",\n", true, true);

                if (fields.size() == MemoryCSVFieldCount && parseCSVField(fields[0], poolName) && parseCSVField(fields[1], memoryType) &&
                    parseCSVField(fields[2], resourceName) && parseCSVField(fields[3], resourceType) &&
                    parseCSVField(fields[4], byteSize) && parseCSVField(fields[5], bindFlags))
                {
                    RHI::MemoryStatistics::Pool* pool;

                    auto it = pools.find(poolName);
                    if (it == pools.end())
                    {
                        pool = &pools.try_emplace(poolName).first->second;
                        pool->m_name = AZStd::move(poolName);
                    }
                    else
                    {
                        pool = &it->second;
                    }

                    if (memoryType != 0 && memoryType != 1)
                    {
                        // Unknown memory type
                        m_captureMessage = AZStd::string::format(
                            "Attempted to load memory data from %s but an unknown memory type was detected (indicating invalid file "
                            "format)",
                            fileName.c_str());
                        AZ_Error("ImGuiGpuMemoryView", false, m_captureMessage.c_str());
                        return;
                    }

                    if (resourceType == 0 /* buffer */)
                    {
                        RHI::MemoryStatistics::Buffer buffer;
                        buffer.m_name = AZStd::move(resourceName);
                        buffer.m_bindFlags = static_cast<RHI::BufferBindFlags>(bindFlags);
                        buffer.m_sizeInBytes = byteSize;
                        pool->m_buffers.push_back(AZStd::move(buffer));
                    }
                    else if (resourceType == 1 /* image */)
                    {
                        RHI::MemoryStatistics::Image image;
                        image.m_name = AZStd::move(resourceName);
                        image.m_bindFlags = static_cast<RHI::ImageBindFlags>(bindFlags);
                        image.m_sizeInBytes = byteSize;
                        pool->m_images.push_back(AZStd::move(image));
                    }

                    pool->m_memoryUsage.m_memoryUsagePerLevel[memoryType].m_usedResidentInBytes += byteSize;
                    pool->m_memoryUsage.m_memoryUsagePerLevel[memoryType].m_totalResidentInBytes += byteSize;

                    // NOTE: This information isn't strictly accurate because we're reconstructing data from a list of
                    // allocations.
                    m_savedHeaps[memoryType].m_memoryUsage.m_totalResidentInBytes += byteSize;
                    m_savedHeaps[memoryType].m_memoryUsage.m_usedResidentInBytes += byteSize;
                }
                else
                {
                    m_captureMessage = AZStd::string::format(
                        "Attempted to load memory data from %s but a parse error occurred (indicating invalid file "
                        "format)",
                        fileName.c_str());
                    AZ_Error("ImGuiGpuMemoryView", false, m_captureMessage.c_str());
                    return;
                }
            }

            for (auto& pool : pools)
            {
                m_savedPools.push_back(AZStd::move(pool.second));
            }

            UpdateTableRows();
            UpdateTreemaps();
        }

        // --- ImGuiGpuProfiler ---

        void ImGuiGpuProfiler::Draw(bool& draw, RHI::Ptr<RPI::ParentPass> rootPass)
        {
            // Update the PassEntry database.
            const PassEntry* rootPassEntryRef = CreatePassEntries(rootPass);

            bool wasDraw = draw;

            GpuProfilerImGuiHelper::Begin("Gpu Profiler", &draw, ImGuiWindowFlags_NoResize, [this, &rootPass]()
            {
                if (ImGui::Checkbox("Enable TimestampView", &m_drawTimestampView))
                {
                    rootPass->SetTimestampQueryEnabled(m_drawTimestampView);
                }
                ImGui::Spacing();
                if(ImGui::Checkbox("Enable PipelineStatisticsView", &m_drawPipelineStatisticsView))
                {
                    rootPass->SetPipelineStatisticsQueryEnabled(m_drawPipelineStatisticsView);
                }
                ImGui::Spacing();
                ImGui::Checkbox("Enable GpuMemoryView", &m_drawGpuMemoryView);
            });

            // Draw the PipelineStatistics window.
            m_timestampView.DrawTimestampWindow(m_drawTimestampView, rootPassEntryRef, m_passEntryDatabase, rootPass);

            // Draw the PipelineStatistics window.
            m_pipelineStatisticsView.DrawPipelineStatisticsWindow(m_drawPipelineStatisticsView, rootPassEntryRef, m_passEntryDatabase, rootPass);

            // Draw the GpuMemory window.
            m_gpuMemoryView.DrawGpuMemoryWindow(m_drawGpuMemoryView);

            //closing window
            if (wasDraw && !draw)
            {
                rootPass->SetTimestampQueryEnabled(false);
                rootPass->SetPipelineStatisticsQueryEnabled(false);
            }
        }

        void ImGuiGpuProfiler::InterpolatePassEntries(AZStd::unordered_map<Name, PassEntry>& passEntryDatabase, float weight) const
        {
            for (auto& entry : passEntryDatabase)
            {
                const auto oldEntryIt = m_passEntryDatabase.find(entry.second.m_path);
                if (oldEntryIt != m_passEntryDatabase.end())
                {
                    // Interpolate the timestamps.
                    const double interpolated = Lerp(static_cast<double>(oldEntryIt->second.m_interpolatedTimestampInNanoseconds),
                        static_cast<double>(entry.second.m_timestampResult.GetDurationInNanoseconds()),
                        static_cast<double>(weight));
                    entry.second.m_interpolatedTimestampInNanoseconds = static_cast<uint64_t>(interpolated);
                }
            }
        }

        PassEntry* ImGuiGpuProfiler::CreatePassEntries(RHI::Ptr<RPI::ParentPass> rootPass)
        {
            AZStd::unordered_map<Name, PassEntry> passEntryDatabase;
            const auto addPassEntry = [&passEntryDatabase](const RPI::Pass* pass, PassEntry* parent) -> PassEntry*
            {
                // If parent a nullptr, it's assumed to be the rootpass.
                if (parent == nullptr)
                {
                    return &passEntryDatabase[pass->GetPathName()];
                }
                else
                {
                    PassEntry entry(pass, parent);

                    // Set the time stamp in the database.
                    [[maybe_unused]] const auto passEntry = passEntryDatabase.find(entry.m_path);
                    AZ_Assert(passEntry == passEntryDatabase.end(), "There already is an entry with the name \"%s\".", entry.m_path.GetCStr());

                    // Set the entry in the map.
                    PassEntry& entryRef = passEntryDatabase[entry.m_path] = entry;
                    return &entryRef;
                }
            };

            // NOTE: Write it all out, can't have recursive functions for lambdas.
            const AZStd::function<void(const RPI::Pass*, PassEntry*)> getPassEntryRecursive = [&addPassEntry, &getPassEntryRecursive](const RPI::Pass* pass, PassEntry* parent) -> void
            {
                if (pass->GetRenderPipeline() && pass->GetRenderPipeline()->GetRenderMode() == RPI::RenderPipeline::RenderMode::NoRender)
                {
                    // Ignore passes from render pipelines that are currently not rendering
                    // E.g. the Preview pipeline
                    return;
                }
                // Add new entry to the timestamp map.
                if (pass->IsEnabled())
                {
                    const RPI::ParentPass* passAsParent = pass->AsParent();
                    PassEntry* entry = addPassEntry(pass, parent);

                    // Recur if it's a parent.
                    if (passAsParent)
                    {
                        for (const auto& childPass : passAsParent->GetChildren())
                        {
                            getPassEntryRecursive(childPass.get(), entry);
                        }
                    }
                }
            };

            // Set up the root entry.
            PassEntry rootEntry(static_cast<RPI::Pass*>(rootPass.get()), nullptr);
            PassEntry& rootEntryRef = passEntryDatabase[rootPass->GetPathName()] = rootEntry;

            // Create an intermediate structure from the passes.
            // Recursively create the timestamp entries tree.
            getPassEntryRecursive(static_cast<RPI::Pass*>(rootPass.get()), nullptr);

            // Interpolate the old values.
            const float lerpWeight = 0.2f;
            InterpolatePassEntries(passEntryDatabase, lerpWeight);

            // Set the new database.
            m_passEntryDatabase = AZStd::move(passEntryDatabase);

            return &rootEntryRef;
        }
    } // namespace Render
} // namespace AZ
