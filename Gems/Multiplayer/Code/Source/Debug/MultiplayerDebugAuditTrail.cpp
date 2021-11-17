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
        const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth;

        if (ImGui::BeginTable("", 4, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 60.0f);
            ImGui::TableSetupColumn("Input ID", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableSetupColumn("HostFrame", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableHeadersRow();

            for (auto elem = auditTrailElems.rbegin(); elem != auditTrailElems.rend(); ++elem)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::TreeNodeEx(elem->name.c_str(), treeFlags))
                {
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", elem->inputId);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", elem->hostFrameId);

                    for (const auto& child : elem->children)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if (child.elements.size() > 0)
                        {
                            if (ImGui::TreeNodeEx(child.name.c_str(), treeFlags))
                            {
                                ImGui::TableNextColumn();
                                ImGui::TableNextColumn();
                                ImGui::TableNextColumn();
                                for (const auto& childElem : child.elements)
                                {
                                    ImGui::TableNextRow();
                                    ImGui::TableNextColumn();
                                    ImGui::TreeNodeEx(
                                        childElem.first.c_str(),
                                        (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth));
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%s", childElem.second.c_str());
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
                        }
                    }
                    ImGui::TreePop();
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
