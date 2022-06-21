/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Debug/MultiplayerDebugMultiplayerMetrics.h>
#include <Multiplayer/IMultiplayer.h>

namespace Multiplayer
{
#ifdef IMGUI_ENABLED
    void AccumulatePerSecondValues(const MultiplayerStats& stats, const MultiplayerStats::Metric& metric, float& outCallsPerSecond, float& outBytesPerSecond)
    {
        uint64_t summedCalls = 0;
        uint64_t summedBytes = 0;
        for (uint32_t index = 0; index < MultiplayerStats::RingbufferSamples; ++index)
        {
            summedCalls += metric.m_callHistory[index];
            summedBytes += metric.m_byteHistory[index];
        }
        const float totalTimeSeconds = static_cast<float>(stats.m_totalHistoryTimeMs) / 1000.0f;
        outCallsPerSecond += (summedCalls > 0 && totalTimeSeconds > 0.0f) ? aznumeric_cast<float>(summedCalls) / totalTimeSeconds : 0.0f;
        outBytesPerSecond += (summedBytes > 0 && totalTimeSeconds > 0.0f) ? aznumeric_cast<float>(summedBytes) / totalTimeSeconds : 0.0f;
    }

    bool DrawMetricsRow(const char* name, bool expandable, uint64_t totalCalls, uint64_t totalBytes, float callsPerSecond, float bytesPerSecond)
    {
        const ImGuiTreeNodeFlags flags = expandable
            ? ImGuiTreeNodeFlags_SpanFullWidth
            : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        const bool open = ImGui::TreeNodeEx(name, flags);
        ImGui::TableNextColumn();
        ImGui::Text("%11llu", aznumeric_cast<AZ::u64>(totalCalls));
        ImGui::TableNextColumn();
        ImGui::Text("%11llu", aznumeric_cast<AZ::u64>(totalBytes));
        ImGui::TableNextColumn();
        ImGui::Text("%11.2f", callsPerSecond);
        ImGui::TableNextColumn();
        ImGui::Text("%11.2f", bytesPerSecond);
        return open;
    }

    bool DrawSummaryRow(const char* name, const MultiplayerStats& stats)
    {
        const MultiplayerStats::Metric propertyUpdatesSent = stats.CalculateTotalPropertyUpdateSentMetrics();
        const MultiplayerStats::Metric propertyUpdatesRecv = stats.CalculateTotalPropertyUpdateRecvMetrics();
        const MultiplayerStats::Metric rpcsSent = stats.CalculateTotalRpcsSentMetrics();
        const MultiplayerStats::Metric rpcsRecv = stats.CalculateTotalRpcsRecvMetrics();
        const uint64_t totalCalls = propertyUpdatesSent.m_totalCalls + propertyUpdatesRecv.m_totalCalls + rpcsSent.m_totalCalls + rpcsRecv.m_totalCalls;
        const uint64_t totalBytes = propertyUpdatesSent.m_totalBytes + propertyUpdatesRecv.m_totalBytes + rpcsSent.m_totalBytes + rpcsRecv.m_totalBytes;
        float callsPerSecond = 0.0f;
        float bytesPerSecond = 0.0f;
        AccumulatePerSecondValues(stats, propertyUpdatesSent, callsPerSecond, bytesPerSecond);
        AccumulatePerSecondValues(stats, propertyUpdatesRecv, callsPerSecond, bytesPerSecond);
        AccumulatePerSecondValues(stats, rpcsSent, callsPerSecond, bytesPerSecond);
        AccumulatePerSecondValues(stats, rpcsRecv, callsPerSecond, bytesPerSecond);
        return DrawMetricsRow(name, true, totalCalls, totalBytes, callsPerSecond, bytesPerSecond);
    }

    bool DrawComponentRow(const char* name, const MultiplayerStats& stats, NetComponentId netComponentId)
    {
        const MultiplayerStats::Metric propertyUpdatesSent = stats.CalculateComponentPropertyUpdateSentMetrics(netComponentId);
        const MultiplayerStats::Metric propertyUpdatesRecv = stats.CalculateComponentPropertyUpdateRecvMetrics(netComponentId);
        const MultiplayerStats::Metric rpcsSent = stats.CalculateComponentRpcsSentMetrics(netComponentId);
        const MultiplayerStats::Metric rpcsRecv = stats.CalculateComponentRpcsRecvMetrics(netComponentId);
        const uint64_t totalCalls = propertyUpdatesSent.m_totalCalls + propertyUpdatesRecv.m_totalCalls + rpcsSent.m_totalCalls + rpcsRecv.m_totalCalls;
        const uint64_t totalBytes = propertyUpdatesSent.m_totalBytes + propertyUpdatesRecv.m_totalBytes + rpcsSent.m_totalBytes + rpcsRecv.m_totalBytes;
        float callsPerSecond = 0.0f;
        float bytesPerSecond = 0.0f;
        AccumulatePerSecondValues(stats, propertyUpdatesSent, callsPerSecond, bytesPerSecond);
        AccumulatePerSecondValues(stats, propertyUpdatesRecv, callsPerSecond, bytesPerSecond);
        AccumulatePerSecondValues(stats, rpcsSent, callsPerSecond, bytesPerSecond);
        AccumulatePerSecondValues(stats, rpcsRecv, callsPerSecond, bytesPerSecond);
        return DrawMetricsRow(name, true, totalCalls, totalBytes, callsPerSecond, bytesPerSecond);
    }

    void DrawComponentDetails(const MultiplayerStats& stats, NetComponentId netComponentId)
    {
        MultiplayerComponentRegistry* componentRegistry = GetMultiplayerComponentRegistry();
        {
            const MultiplayerStats::Metric metric = stats.CalculateComponentPropertyUpdateSentMetrics(netComponentId);
            float callsPerSecond = 0.0f;
            float bytesPerSecond = 0.0f;
            AccumulatePerSecondValues(stats, metric, callsPerSecond, bytesPerSecond);
            if (DrawMetricsRow("PropertyUpdates Sent", true, metric.m_totalCalls, metric.m_totalBytes, callsPerSecond, bytesPerSecond))
            {
                const MultiplayerStats::ComponentStats& componentStats = stats.m_componentStats[aznumeric_cast<AZStd::size_t>(netComponentId)];
                for (AZStd::size_t index = 0; index < componentStats.m_propertyUpdatesSent.size(); ++index)
                {
                    const PropertyIndex propertyIndex = aznumeric_cast<PropertyIndex>(index);
                    const char* propertyName = componentRegistry->GetComponentPropertyName(netComponentId, propertyIndex);
                    const MultiplayerStats::Metric& subMetric = componentStats.m_propertyUpdatesSent[index];
                    callsPerSecond = 0.0f;
                    bytesPerSecond = 0.0f;
                    AccumulatePerSecondValues(stats, subMetric, callsPerSecond, bytesPerSecond);
                    DrawMetricsRow(propertyName, false, subMetric.m_totalCalls, subMetric.m_totalBytes, callsPerSecond, bytesPerSecond);
                }
                ImGui::TreePop();
            }
        }
        {
            const MultiplayerStats::Metric metric = stats.CalculateComponentPropertyUpdateRecvMetrics(netComponentId);
            float callsPerSecond = 0.0f;
            float bytesPerSecond = 0.0f;
            AccumulatePerSecondValues(stats, metric, callsPerSecond, bytesPerSecond);
            if (DrawMetricsRow("PropertyUpdates Recv", true, metric.m_totalCalls, metric.m_totalBytes, callsPerSecond, bytesPerSecond))
            {
                const MultiplayerStats::ComponentStats& componentStats = stats.m_componentStats[aznumeric_cast<AZStd::size_t>(netComponentId)];
                for (AZStd::size_t index = 0; index < componentStats.m_propertyUpdatesRecv.size(); ++index)
                {
                    const PropertyIndex propertyIndex = aznumeric_cast<PropertyIndex>(index);
                    const char* propertyName = componentRegistry->GetComponentPropertyName(netComponentId, propertyIndex);
                    const MultiplayerStats::Metric& subMetric = componentStats.m_propertyUpdatesRecv[index];
                    callsPerSecond = 0.0f;
                    bytesPerSecond = 0.0f;
                    AccumulatePerSecondValues(stats, subMetric, callsPerSecond, bytesPerSecond);
                    DrawMetricsRow(propertyName, false, subMetric.m_totalCalls, subMetric.m_totalBytes, callsPerSecond, bytesPerSecond);
                }
                ImGui::TreePop();
            }
        }
        {
            const MultiplayerStats::Metric metric = stats.CalculateComponentRpcsSentMetrics(netComponentId);
            float callsPerSecond = 0.0f;
            float bytesPerSecond = 0.0f;
            AccumulatePerSecondValues(stats, metric, callsPerSecond, bytesPerSecond);
            if (DrawMetricsRow("RemoteProcedures Sent", true, metric.m_totalCalls, metric.m_totalBytes, callsPerSecond, bytesPerSecond))
            {
                const MultiplayerStats::ComponentStats& componentStats = stats.m_componentStats[aznumeric_cast<AZStd::size_t>(netComponentId)];
                for (AZStd::size_t index = 0; index < componentStats.m_rpcsSent.size(); ++index)
                {
                    const RpcIndex rpcIndex = aznumeric_cast<RpcIndex>(index);
                    const char* rpcName = componentRegistry->GetComponentRpcName(netComponentId, rpcIndex);
                    const MultiplayerStats::Metric& subMetric = componentStats.m_rpcsSent[index];
                    callsPerSecond = 0.0f;
                    bytesPerSecond = 0.0f;
                    AccumulatePerSecondValues(stats, subMetric, callsPerSecond, bytesPerSecond);
                    DrawMetricsRow(rpcName, false, subMetric.m_totalCalls, subMetric.m_totalBytes, callsPerSecond, bytesPerSecond);
                }
                ImGui::TreePop();
            }
        }
        {
            const MultiplayerStats::Metric metric = stats.CalculateComponentRpcsRecvMetrics(netComponentId);
            float callsPerSecond = 0.0f;
            float bytesPerSecond = 0.0f;
            AccumulatePerSecondValues(stats, metric, callsPerSecond, bytesPerSecond);
            if (DrawMetricsRow("RemoteProcedures Recv", true, metric.m_totalCalls, metric.m_totalBytes, callsPerSecond, bytesPerSecond))
            {
                const MultiplayerStats::ComponentStats& componentStats = stats.m_componentStats[aznumeric_cast<AZStd::size_t>(netComponentId)];
                for (AZStd::size_t index = 0; index < componentStats.m_rpcsRecv.size(); ++index)
                {
                    const RpcIndex rpcIndex = aznumeric_cast<RpcIndex>(index);
                    const char* rpcName = componentRegistry->GetComponentRpcName(netComponentId, rpcIndex);
                    const MultiplayerStats::Metric& subMetric = componentStats.m_rpcsRecv[index];
                    callsPerSecond = 0.0f;
                    bytesPerSecond = 0.0f;
                    AccumulatePerSecondValues(stats, subMetric, callsPerSecond, bytesPerSecond);
                    DrawMetricsRow(rpcName, false, subMetric.m_totalCalls, subMetric.m_totalBytes, callsPerSecond, bytesPerSecond);
                }
                ImGui::TreePop();
            }
        }
    }

    void MultiplayerDebugMultiplayerMetrics::OnImGuiUpdate()
    {
        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;

        IMultiplayer* multiplayer = AZ::Interface<IMultiplayer>::Get();
        MultiplayerComponentRegistry* componentRegistry = GetMultiplayerComponentRegistry();
        const Multiplayer::MultiplayerStats& stats = multiplayer->GetStats();
        ImGui::Text("Multiplayer operating in %s mode", GetEnumString(multiplayer->GetAgentType()));
        ImGui::Text("Total networked entities: %llu", aznumeric_cast<AZ::u64>(stats.m_entityCount));
        ImGui::Text("Total client connections: %llu", aznumeric_cast<AZ::u64>(stats.m_clientConnectionCount));
        ImGui::Text("Total server connections: %llu", aznumeric_cast<AZ::u64>(stats.m_serverConnectionCount));
        ImGui::NewLine();

        static ImGuiTableFlags flags = ImGuiTableFlags_BordersV
            | ImGuiTableFlags_BordersOuterH
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("Multiplayer Metrics", 5, flags))
        {
            // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Total Calls", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableSetupColumn("Total Bytes", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableSetupColumn("Calls/Sec", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableSetupColumn("Bytes/Sec", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            ImGui::TableHeadersRow();

            if (DrawSummaryRow("Totals", stats))
            {
                for (AZStd::size_t index = 0; index < stats.m_componentStats.size(); ++index)
                {
                    const NetComponentId netComponentId = aznumeric_cast<NetComponentId>(index);
                    using StringLabel = AZStd::fixed_string<128>;
                    const StringLabel gemName = componentRegistry->GetComponentGemName(netComponentId);
                    const StringLabel componentName = componentRegistry->GetComponentName(netComponentId);
                    const StringLabel label = gemName + "::" + componentName;
                    if (DrawComponentRow(label.c_str(), stats, netComponentId))
                    {
                        DrawComponentDetails(stats, netComponentId);
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::EndTable();
            ImGui::NewLine();
        }
        ImGui::End();
    }
#endif
}
