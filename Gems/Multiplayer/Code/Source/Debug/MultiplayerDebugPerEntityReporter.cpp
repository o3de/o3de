/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerDebugPerEntityReporter.h"
#include <Multiplayer/IMultiplayer.h>

#if defined(IMGUI_ENABLED)
#include <imgui/imgui.h>
#endif

#pragma optimize("", off)

namespace MultiplayerDiagnostics
{
#if defined(IMGUI_ENABLED)
    static const ImVec4 k_ImGuiTomato = ImVec4(1.0f, 0.4f, 0.3f, 1.0f);
    static const ImVec4 k_ImGuiKhaki = ImVec4(0.9f, 0.8f, 0.5f, 1.0f);
    static const ImVec4 k_ImGuiCyan = ImVec4(0.5f, 1.0f, 1.0f, 1.0f);
    static const ImVec4 k_ImGuiDusk = ImVec4(0.7f, 0.7f, 1.0f, 1.0f);
    static const ImVec4 k_ImGuiWhite = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    // --------------------------------------------------------------------------------------------
    template <typename Reporter>
    bool ReplicatedStateTreeNode(const AZStd::string& name, Reporter& report, const ImVec4& color, int depth = 0)
    {
        const int defaultPadAmount = 55;
        const int depthReduction = 3;
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        const bool expanded = ImGui::TreeNode(name.c_str(),
            "%-*s %7.2f kbps %7.2f B Avg. %4zu B Max %10zu B Payload",
            defaultPadAmount - depthReduction * depth,
            name.c_str(),
            report.GetKbitsPerSecond(),
            report.GetAverageBytes(),
            report.GetMaxBytes(),
            report.GetTotalBytes());
        ImGui::PopStyleColor();
        return expanded;
    }

    // --------------------------------------------------------------------------------------------
    void DisplayReplicatedStateReport(AZStd::map<AZStd::string, ComponentReporter>& componentReports, float kbpsWarn, float maxWarn)
    {
        for (auto& componentPair : componentReports)
        {
            ImGui::Separator();
            ComponentReporter& componentReport = componentPair.second;

            if (ReplicatedStateTreeNode(componentPair.first, componentReport, k_ImGuiCyan, 1))
            {
                ImGui::Separator();
                ImGui::Columns(6, "replicated_field_columns");
                ImGui::NextColumn();
                ImGui::Text("kbps");
                ImGui::NextColumn();
                ImGui::Text("Avg. Bytes");
                ImGui::NextColumn();
                ImGui::Text("Min Bytes");
                ImGui::NextColumn();
                ImGui::Text("Max Bytes");
                ImGui::NextColumn();
                ImGui::Text("Total Bytes");
                ImGui::NextColumn();

                auto fieldReports = componentReport.GetFieldReports();
                for (auto& fieldPair : fieldReports)
                {
                    MultiplayerDebugByteReporter& fieldReport = *fieldPair.second;
                    const float kbitsLastSecond = fieldReport.GetKbitsPerSecond();

                    const ImVec4* textColor = &k_ImGuiWhite;
                    if (fieldReport.GetMaxBytes() > maxWarn)
                    {
                        textColor = &k_ImGuiKhaki;
                    }

                    if (kbitsLastSecond > kbpsWarn)
                    {
                        textColor = &k_ImGuiTomato;
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, *textColor);

                    ImGui::Text("%s", fieldPair.first.c_str());
                    ImGui::NextColumn();
                    ImGui::Text("%.2f", kbitsLastSecond);
                    ImGui::NextColumn();
                    ImGui::Text("%.2f", fieldReport.GetAverageBytes());
                    ImGui::NextColumn();
                    ImGui::Text("%zu", fieldReport.GetMinBytes());
                    ImGui::NextColumn();
                    ImGui::Text("%zu", fieldReport.GetMaxBytes());
                    ImGui::NextColumn();
                    ImGui::Text("%zu", fieldReport.GetTotalBytes());
                    ImGui::NextColumn();

                    ImGui::PopStyleColor();
                }

                ImGui::Columns(1);
                ImGui::TreePop();
            }
        }
    }
#endif

    // --------------------------------------------------------------------------------------------
    void MultiplayerDebugPerEntityReporter::OnImGuiUpdate()
    {
#if defined(IMGUI_ENABLED)
        static ImGuiTextFilter filter;
        filter.Draw();

        if (ImGui::CollapsingHeader("Receiving Entities"))
        {
            for (auto& entityPair : m_receivingEntityReports)
            {
                if (!filter.PassFilter(entityPair.first.c_str()))
                {
                    continue;
                }

                ImGui::Separator();
                if (ReplicatedStateTreeNode(entityPair.first, entityPair.second, k_ImGuiDusk))
                {
                    DisplayReplicatedStateReport(entityPair.second.GetComponentReports(), m_replicatedStateKbpsWarn, m_replicatedStateMaxSizeWarn);
                    ImGui::TreePop();
                }
            }
        }

        if (ImGui::CollapsingHeader("Sending Entities"))
        {
            for (auto& entityPair : m_sendingEntityReports)
            {
                if (!filter.PassFilter(entityPair.first.c_str()))
                {
                    continue;
                }

                ImGui::Separator();
                if (ReplicatedStateTreeNode(entityPair.first, entityPair.second, k_ImGuiDusk))
                {
                    DisplayReplicatedStateReport(entityPair.second.GetComponentReports(), m_replicatedStateKbpsWarn, m_replicatedStateMaxSizeWarn);
                    ImGui::TreePop();
                }
            }
        }
#endif
    }

    void MultiplayerDebugPerEntityReporter::RecordEntitySerializeStart(AzNetworking::SerializerMode mode,
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const char* entityName)
    {
        switch (mode)
        {
        case AzNetworking::SerializerMode::ReadFromObject:
            m_currentSendingEntityReport.Reset();
            break;
        case AzNetworking::SerializerMode::WriteToObject:
            m_currentReceivingEntityReport.Reset();
            break;
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordComponentSerializeEnd(AzNetworking::SerializerMode mode, [[maybe_unused]] Multiplayer::NetComponentId netComponentId)
    {
        switch (mode)
        {
        case AzNetworking::SerializerMode::ReadFromObject:
            m_currentSendingEntityReport.ReportFragmentEnd();
            break;
        case AzNetworking::SerializerMode::WriteToObject:
            m_currentReceivingEntityReport.ReportFragmentEnd();
            break;
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordEntitySerializeStop(AzNetworking::SerializerMode mode,
        [[maybe_unused]] AZ::EntityId entityId, const char* entityName)
    {
        switch (mode)
        {
        case AzNetworking::SerializerMode::ReadFromObject:
            m_sendingEntityReports[entityName].Combine(m_currentSendingEntityReport);
            break;
        case AzNetworking::SerializerMode::WriteToObject:
            m_receivingEntityReports[entityName].Combine(m_currentReceivingEntityReport);
            break;
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordPropertySent(
        Multiplayer::NetComponentId netComponentId,
        Multiplayer::PropertyIndex propertyId,
        uint32_t totalBytes)
    {
        if (const Multiplayer::MultiplayerComponentRegistry* componentRegistry = Multiplayer::GetMultiplayerComponentRegistry())
        {
            m_currentSendingEntityReport.ReportField(static_cast<AZ::u32>(netComponentId),
                componentRegistry->GetComponentName(netComponentId),
                componentRegistry->GetComponentPropertyName(netComponentId, propertyId), totalBytes);
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordPropertyReceived(
        Multiplayer::NetComponentId netComponentId,
        Multiplayer::PropertyIndex propertyId,
        uint32_t totalBytes)
    {
        if (const Multiplayer::MultiplayerComponentRegistry* componentRegistry = Multiplayer::GetMultiplayerComponentRegistry())
        {
            m_currentReceivingEntityReport.ReportField(static_cast<AZ::u32>(netComponentId),
                componentRegistry->GetComponentName(netComponentId),
                componentRegistry->GetComponentPropertyName(netComponentId, propertyId), totalBytes);
        }
    }

    void MultiplayerDebugPerEntityReporter::RecordRpcSent(Multiplayer::NetComponentId netComponentId, Multiplayer::RpcIndex rpcId, uint32_t totalBytes)
    {
        if (const Multiplayer::MultiplayerComponentRegistry* componentRegistry = Multiplayer::GetMultiplayerComponentRegistry())
        {
            m_currentSendingEntityReport.ReportField(static_cast<AZ::u32>(netComponentId),
                componentRegistry->GetComponentName(netComponentId),
                componentRegistry->GetComponentRpcName(netComponentId, rpcId), totalBytes);
        }
    }
}
