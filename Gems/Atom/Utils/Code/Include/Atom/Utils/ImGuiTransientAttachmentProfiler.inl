/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DeviceTransientAttachmentPool.h>

namespace AZ
{
    namespace Render
    {
        constexpr double BytesToMB = 1.0 / (1024.0 * 1024.0);

        inline bool ImGuiTransientAttachmentProfiler::Draw(const AZStd::unordered_map<int, RHI::TransientAttachmentStatistics>& statistics)
        {
            ImVec2 windowSize(300, 500.f);
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
            bool isOpen = true;
            if (ImGui::Begin("Transient Attachment Pool", &isOpen, ImGuiWindowFlags_None))
            {
                if (ImGui::TreeNodeEx("Memory", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    RHI::TransientAttachmentStatistics::MemoryUsage memoryUsage{};

                    for (auto& [deviceIndex, deviceStatistics] : statistics)
                    {
                        memoryUsage = deviceStatistics.m_reservedMemory;
                    }

                    ImGui::Text("Strategy: %s", ToString(RHI::RHISystemInterface::Get()->GetTransientAttachmentPoolDescriptor()->begin()->second.m_heapParameters.m_type));
                    ImGui::Text("Buffer Memory: %.1f MB", static_cast<double>(memoryUsage.m_bufferMemoryInBytes * BytesToMB));
                    ImGui::Text("Image Memory: %.1f MB", static_cast<double>(memoryUsage.m_imageMemoryInBytes * BytesToMB));
                    ImGui::Text("Rendertarget Memory: %.1f MB", static_cast<double>(memoryUsage.m_rendertargetMemoryInBytes * BytesToMB));
                    ImGui::Text("Total Memory: %.1f MB", static_cast<double>(
                        (memoryUsage.m_bufferMemoryInBytes +
                            memoryUsage.m_imageMemoryInBytes +
                            memoryUsage.m_rendertargetMemoryInBytes)
                        * BytesToMB));
                    ImGui::TreePop();
                }

                if (ImGui::TreeNodeEx("Heaps", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (auto& [deviceIndex, deviceStatistics] : statistics)
                    {
                        for (uint32_t i = 0; i < deviceStatistics.m_heaps.size(); ++i)
                        {
                            DrawHeap(deviceStatistics.m_heaps[i], deviceStatistics.m_scopes);
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::End();
            return isOpen;
        }

        inline bool AddButton(const char* id, const char* label, const ImColor& color, const ImVec2& size)
        {
            // Adds a button and the hover colors.
            float hue, sat, value;
            ImGui::PushID(id);
            ImGui::ColorConvertRGBtoHSV(color.Value.x, color.Value.y, color.Value.z, hue, sat, value);
            float lighterFactor = 1.2f;
            ImGui::PushStyleColor(ImGuiCol_Button, color.Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue, sat, value * lighterFactor).Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue, sat, value * lighterFactor * lighterFactor).Value);
            bool pushed = ImGui::Button(label, size);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            return pushed;
        }

        inline void ImGuiTransientAttachmentProfiler::DrawHeap(
            const AZ::RHI::TransientAttachmentStatistics::Heap& heapStats,
            const AZStd::vector<AZ::RHI::TransientAttachmentStatistics::Scope>& scopes)
        {
            // Some constants for sizes and colors.
            const float HeapButtonWidth = 100.0f;
            const float ScopeButtonHeight = 30.0f;
            const float ContentArea = 400.0f;
            const float scrollBarWidth = 20.0f;
            const ImColor scopeColors[RHI::HardwareQueueClassCount] =
            {
                ImColor(38, 43, 219),
                ImColor(70, 187, 88),
                ImColor(128, 64, 64)
            };

            const ImColor attachmentColors[static_cast<uint32_t>(RHI::AliasedResourceType::Count)] =
            {
                ImColor(72, 61, 153),
                ImColor(153, 61, 150),
                ImColor(200, 61, 61)
            };

            const char* heapId = heapStats.m_name.GetCStr();
            if (ImGui::TreeNode(heapId))
            {
                // Main child that contains the scopes, heap and attachments.
                if (ImGui::BeginChild(
                    AZStd::string::format("Content%s", heapId).c_str(),
                    ImVec2(0, ContentArea), // Use max width from the parent window
                    false,
                    ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar)) // Don't allow scrolling inside this child because we will scroll inside the attachment window.
                {
                    ImGuiID heapScaleId = ImGui::GetID(AZStd::string::format("Scale%s", heapId).c_str());
                    float scale = ImGui::GetStateStorage()->GetFloat(heapScaleId, 1.0f);
                    float AttachmentsAreaHeight = (ContentArea - ScopeButtonHeight) * scale;
                    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
                    {
                        ImGuiIO& io = ImGui::GetIO();
                        // Calculate zoom if pressing
                        // Ctrl + Mouse Wheel.
                        if (io.KeyCtrl && io.MouseWheel)
                        {
                            scale += io.MouseWheel * 0.05f;
                            scale = AZStd::max(scale, 1.0f);
                            ImGui::GetStateStorage()->SetFloat(heapScaleId, scale);
                        }
                    }
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 20.0f);
                    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImColor(128, 128, 128).Value);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImColor(62, 62, 62).Value);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImColor(168, 168, 168).Value);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImColor(198, 198, 198).Value);
                    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImColor(255, 255, 255).Value);

                    // We create 3 separate child windows for scopes, heap memory and attachments so we can scroll
                    // only the attachment windows and not the others.

                    // This child window contains the list of scopes.
                    auto scopesId = AZStd::string::format("Scope%s", heapId);
                    if (ImGui::BeginChild(
                        scopesId.c_str(),
                        ImVec2(0, ScopeButtonHeight), // Use max width from the parent window
                        false,
                        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar)) // Don't allow scrolling inside this child because we will scroll inside the attachment window.
                    {
                        // Add an invisible button to have the same offset as the Attachments child window.
                        ImGui::InvisibleButton("##dummy", ImVec2(HeapButtonWidth, ScopeButtonHeight));
                        ImGui::SameLine();
                        float scopesAreaWidthAvailable = ImGui::GetContentRegionAvail().x * scale;
                        float scopeButtonWidth = (scopesAreaWidthAvailable / scopes.size()) - ImGui::GetStyle().ItemSpacing.x;
                        for (uint32_t scopeNum = 0; scopeNum < scopes.size(); ++scopeNum)
                        {
                            if (scopeNum)
                            {
                                ImGui::SameLine();
                            }
                            const auto& scope = scopes[scopeNum];
                            AddButton(
                                AZStd::string::format("ScopeButton%s", heapId).c_str(),
                                scope.m_scopeId.GetCStr(),
                                scopeColors[static_cast<uint32_t>(scope.m_hardwareQueueClass)],
                                ImVec2(scopeButtonWidth, 0));

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("Id: %s", scope.m_scopeId.GetCStr());
                                ImGui::Text("Hardware Queue Class: %s", ToString(scope.m_hardwareQueueClass));
                                ImGui::EndTooltip();
                            }

                        }
                    }
                    ImGui::EndChild();

                    // Child window that contain a button that represents the Heap's Memory.
                    auto heapMemoryId = AZStd::string::format("HeapMemory%s", heapId);
                    if (ImGui::BeginChild(
                        heapMemoryId.c_str(),
                        ImVec2(HeapButtonWidth, 0),
                        false,
                        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
                    {
                        AZStd::string heapMemoryLabel = AZStd::string::format("%.1f MB", heapStats.m_heapSize * BytesToMB);
                        AZStd::string verticalHeapMemoryLabel;
                        for (const auto& c : heapMemoryLabel)
                        {
                            verticalHeapMemoryLabel.push_back(c);
                            verticalHeapMemoryLabel += "\n";
                        }

                        AddButton(
                            AZStd::string::format("HeapMemoryButton%s", heapId).c_str(),
                            verticalHeapMemoryLabel.c_str(),
                            ImColor(35, 197, 140),
                            ImVec2(HeapButtonWidth, AttachmentsAreaHeight));

                        if (ImGui::IsItemHovered())
                        {
                            AZStd::string resourceType;
                            for (uint32_t i = 0; i < static_cast<uint32_t>(AZ::RHI::AliasedResourceType::Count); ++i)
                            {
                                if (AZ::RHI::CheckBitsAny(heapStats.m_resourceTypeFlags, static_cast<AZ::RHI::AliasedResourceTypeFlags>(AZ_BIT(i))))
                                {
                                    resourceType += ToString(static_cast<AZ::RHI::AliasedResourceType>(i));
                                    resourceType += "|";
                                }
                            }
                            resourceType.pop_back();

                            ImGui::BeginTooltip();
                            ImGui::Text("Type: %s", resourceType.c_str());
                            ImGui::Text("Size: %.1f MB", static_cast<double>(heapStats.m_heapSize * BytesToMB));
                            ImGui::Text("Watermark: %.1f MB", static_cast<double>(heapStats.m_watermarkSize * BytesToMB));
                            ImGui::Text("Waste: %.1f%%", (1.0 - static_cast<double>(heapStats.m_watermarkSize) / heapStats.m_heapSize) * 100.0);
                            ImGui::EndTooltip();
                        }
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    float scrollingX = 0.f;
                    float scrollingY = 0.f;
                    // The child window for attachments
                    if (ImGui::BeginChild(
                        AZStd::string::format("Attachments%s", heapId).c_str(),
                        ImVec2(0, 0),
                        false,
                        ImGuiWindowFlags_HorizontalScrollbar)) // The only window that scrolls.
                    {
                        float areaWidthAvailable = ImGui::GetContentRegionAvail().x;
                        if (ImGui::GetScrollMaxX())
                        {
                            areaWidthAvailable += scrollBarWidth;
                        }
                        areaWidthAvailable *= scale;
                        const float spacingX = ImGui::GetStyle().ItemSpacing.x;
                        const float scopeIndexToPos = areaWidthAvailable / scopes.size();
                        const float heapOffsetToPos = AttachmentsAreaHeight / heapStats.m_heapSize;

                        for (const RHI::TransientAttachmentStatistics::Attachment& attachmentStats : heapStats.m_attachments)
                        {
                            ImGui::SetCursorPosX((attachmentStats.m_scopeOffsetMin * scopeIndexToPos) + spacingX);
                            ImGui::SetCursorPosY(attachmentStats.m_heapOffsetMin * heapOffsetToPos);
                            ImVec2 attachmentSize;
                            attachmentSize.x = ((attachmentStats.m_scopeOffsetMax - attachmentStats.m_scopeOffsetMin + 1) * scopeIndexToPos) - spacingX;
                            attachmentSize.y = (attachmentStats.m_heapOffsetMax - attachmentStats.m_heapOffsetMin) * heapOffsetToPos;
                            AddButton(
                                AZStd::string::format("AttachmentsButton%s", heapId).c_str(),
                                attachmentStats.m_id.GetCStr(),
                                attachmentColors[static_cast<uint32_t>(attachmentStats.m_type)],
                                attachmentSize);

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("Id: %s", attachmentStats.m_id.GetCStr());
                                ImGui::Text("Heap Begin: %.1f MB", attachmentStats.m_heapOffsetMin * BytesToMB);
                                ImGui::Text("Heap End: %.1f MB", attachmentStats.m_heapOffsetMax * BytesToMB);
                                ImGui::Text("Size: %.1f MB", attachmentStats.m_sizeInBytes * BytesToMB);
                                ImGui::Text("Scope Begin: %s", scopes[attachmentStats.m_scopeOffsetMin].m_scopeId.GetCStr());
                                ImGui::Text("Scope End: %s", scopes[attachmentStats.m_scopeOffsetMax].m_scopeId.GetCStr());
                                ImGui::EndTooltip();
                            }

                            scrollingX = ImGui::GetScrollX();
                            scrollingY = ImGui::GetScrollY();
                        }
                    }
                    ImGui::EndChild();

                    // Manually set the scrolling for the scope buttons and heap memory button window
                    // to match the scrolling of the attachment windows.
                    {
                        ImGui::BeginChild(heapMemoryId.c_str());
                        ImGui::SetScrollY(scrollingY);
                        ImGui::EndChild();
                    }
                    {

                        ImGui::BeginChild(scopesId.c_str());
                        ImGui::SetScrollX(scrollingX);
                        ImGui::EndChild();
                    }

                    ImGui::PopStyleVar(3);
                    ImGui::PopStyleColor(5);
                }
                ImGui::EndChild();
                ImGui::TreePop();
            }
        }
    } // namespace Render
} // namespace AZ
