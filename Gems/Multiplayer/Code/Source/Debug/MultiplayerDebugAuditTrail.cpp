/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerDebugAuditTrail.h"

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Debug/MultiplayerDebugSystemComponent.h>

#if defined(IMGUI_ENABLED)
#include <imgui/imgui.h>
#endif

namespace Multiplayer
{
    constexpr char DESYNC_TITLE[] = "Desync on %s";
    constexpr char INPUT_TITLE[] = "%s Inputs";
    constexpr char EVENT_TITLE[] = "DevEvent on %s";

    MultiplayerDebugAuditTrail::MultiplayerDebugAuditTrail()
        : m_updateDebugOverlay([this]() { UpdateDebugOverlay(); }, AZ::Name("UpdateAuditTrail"))
    {
        m_updateDebugOverlay.Enqueue(AZ::TimeMs{ 0 }, true);
    }

    MultiplayerDebugAuditTrail::~MultiplayerDebugAuditTrail()
    {
        ;
    }

    // --------------------------------------------------------------------------------------------
    void MultiplayerDebugAuditTrail::OnImGuiUpdate(AZStd::deque<AuditTrailInput> auditTrailElems)
    {
#if defined(IMGUI_ENABLED)
        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const ImGuiTableFlags flags = ImGuiTableFlags_BordersV
            | ImGuiTableFlags_BordersOuterH
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("", 5, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Client Value", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 60.0f);
            ImGui::TableSetupColumn("Server Value", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 60.0f);
            ImGui::TableSetupColumn("Input ID", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableSetupColumn("HostFrame", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableHeadersRow();

            bool atRootLevel = true;

            for (auto elem = auditTrailElems.begin(); elem != auditTrailElems.end(); ++elem)
            {
                if (elem == auditTrailElems.begin() && elem->m_category != MultiplayerAuditCategory::Desync)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    atRootLevel = !ImGui::TreeNodeEx("HEAD", ImGuiTreeNodeFlags_SpanFullWidth);
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                }
                else if (!atRootLevel && elem != auditTrailElems.begin() && elem->m_category == MultiplayerAuditCategory::Desync)
                {
                    atRootLevel = true;
                    ImGui::TreePop();
                }

                if (!atRootLevel || elem->m_category == MultiplayerAuditCategory::Desync)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    const char* nodeTitle = elem->m_category == MultiplayerAuditCategory::Desync
                        ? DESYNC_TITLE
                        : (elem->m_category == MultiplayerAuditCategory::Input ? INPUT_TITLE : EVENT_TITLE);

                    // Draw Events as a single line entry, they should only have one line item
                    if (elem->m_category == MultiplayerAuditCategory::Event)
                    {
                        if (elem->m_children.size() > 0 && elem->m_children.front().elements.size() > 0)
                        {
                            AZStd::pair<AZStd::string, AZStd::string> cliServValues =
                                elem->m_children.front().elements.front()->GetClientServerValues();
                            ImGui::TreeNodeEx(
                                AZStd::string::format(nodeTitle, elem->m_name.c_str()).c_str(),
                                (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth));
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", cliServValues.first.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", cliServValues.second.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("%d", elem->m_inputId);
                            ImGui::TableNextColumn();
                            ImGui::Text("%d", elem->m_hostFrameId);
                        }
                    }
                    // Draw desyncs and inputs as a collapsable node, they can contain multiple line items
                    else if (ImGui::TreeNodeEx(
                            AZStd::string::format(nodeTitle, elem->m_name.c_str()).c_str(),
                            ImGuiTreeNodeFlags_SpanFullWidth))
                    {
                        atRootLevel = false;
                        ImGui::TableNextColumn();
                        ImGui::TableNextColumn();
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", elem->m_inputId);
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", elem->m_hostFrameId);

                        for (const auto& child : elem->m_children)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            if (child.elements.size() > 0)
                            {
                                const ImGuiTableFlags childFlags = elem->m_category == MultiplayerAuditCategory::Desync
                                    ? ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen
                                    : ImGuiTreeNodeFlags_SpanFullWidth;
                                if (ImGui::TreeNodeEx(child.name.c_str(), childFlags))
                                {
                                    ImGui::TableNextColumn();
                                    ImGui::TableNextColumn();
                                    ImGui::TableNextColumn();
                                    ImGui::TableNextColumn();
                                    for (const auto& childElem : child.elements)
                                    {
                                        AZStd::pair<AZStd::string, AZStd::string> cliServValues = childElem->GetClientServerValues();
                                        ImGui::TableNextRow();
                                        ImGui::TableNextColumn();
                                        ImGui::TreeNodeEx(
                                            childElem->GetName().c_str(),
                                            (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                             ImGuiTreeNodeFlags_SpanFullWidth));
                                        ImGui::TableNextColumn();
                                        ImGui::Text("%s", cliServValues.first.c_str());
                                        ImGui::TableNextColumn();
                                        ImGui::Text("%s", cliServValues.second.c_str());
                                        ImGui::TableNextColumn();
                                        ImGui::TableNextColumn();
                                    }
                                    ImGui::TreePop();
                                }
                            }
                            else
                            {
                                ImGui::Text(child.name.c_str());
                                ImGui::TableNextColumn();
                                ImGui::TableNextColumn();
                                ImGui::TableNextColumn();
                                ImGui::TableNextColumn();
                            }
                        }
                        if (elem->m_category != MultiplayerAuditCategory::Desync)
                        {
                            ImGui::TreePop();
                        }
                    }
                    else
                    {
                        ImGui::TableNextColumn();
                        ImGui::TableNextColumn();
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", elem->m_inputId);
                        ImGui::TableNextColumn();
                        ImGui::Text("%d", elem->m_hostFrameId);
                        ImGui::TableNextRow();
                    }
                }
            }

            ImGui::EndTable();
            ImGui::NewLine();
        }
#endif
    }


    void MultiplayerDebugAuditTrail::UpdateDebugOverlay()
    {
        if (m_debugDisplay == nullptr)
        {
            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, AzFramework::g_defaultSceneEntityDebugDisplayId);
            m_debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
        }

        const AZ::u32 stateBefore = m_debugDisplay->GetState();
        m_debugDisplay->SetColor(AZ::Colors::White);

        m_debugDisplay->SetState(stateBefore);
    }
}
