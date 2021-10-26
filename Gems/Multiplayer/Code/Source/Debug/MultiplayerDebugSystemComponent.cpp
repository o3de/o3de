/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Debug/MultiplayerDebugSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Interface/Interface.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <Multiplayer/IMultiplayer.h>

void OnDebugEntities_ShowBandwidth_Changed(const bool& showBandwidth);

AZ_CVAR(bool, net_DebugEntities_ShowBandwidth, false, &OnDebugEntities_ShowBandwidth_Changed, AZ::ConsoleFunctorFlags::Null,
    "If true, prints bandwidth values over entities that use a considerable amount of network traffic");

namespace Multiplayer
{
    void MultiplayerDebugSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerDebugSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void MultiplayerDebugSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerDebugSystemComponent"));
    }

    void MultiplayerDebugSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        ;
    }

    void MultiplayerDebugSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile)
    {
        incompatbile.push_back(AZ_CRC_CE("MultiplayerDebugSystemComponent"));
    }

    void MultiplayerDebugSystemComponent::Activate()
    {
#ifdef IMGUI_ENABLED
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
#endif
    }

    void MultiplayerDebugSystemComponent::Deactivate()
    {
#ifdef IMGUI_ENABLED
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif
    }

    void MultiplayerDebugSystemComponent::ShowEntityBandwidthDebugOverlay()
    {
        m_reporter = AZStd::make_unique<MultiplayerDebugPerEntityReporter>();
    }

    void MultiplayerDebugSystemComponent::HideEntityBandwidthDebugOverlay()
    {
        m_reporter.reset();
    }

#ifdef IMGUI_ENABLED
    void MultiplayerDebugSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Multiplayer"))
        {
            ImGui::Checkbox("Networking Stats", &m_displayNetworkingStats);
            ImGui::Checkbox("Multiplayer Stats", &m_displayMultiplayerStats);
            ImGui::Checkbox("Multiplayer Entity Stats", &m_displayPerEntityStats);
            ImGui::Checkbox("Multiplayer Hierarchy Debugger", &m_displayHierarchyDebugger);
            ImGui::EndMenu();
        }
    }

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
        outCallsPerSecond += (summedCalls > 0 && totalTimeSeconds > 0.0f) ? static_cast<float>(summedCalls) / totalTimeSeconds : 0.0f;
        outBytesPerSecond += (summedBytes > 0 && totalTimeSeconds > 0.0f) ? static_cast<float>(summedBytes) / totalTimeSeconds : 0.0f;
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

    void DrawNetworkingStats()
    {
        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;

        const ImGuiTableFlags flags = ImGuiTableFlags_BordersV
            | ImGuiTableFlags_BordersOuterH
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_NoBordersInBody;

        const ImGuiTreeNodeFlags nodeFlags = (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);

        AzNetworking::INetworking* networking = AZ::Interface<AzNetworking::INetworking>::Get();

        ImGui::Text("Total sockets monitored by TcpListenThread: %u", networking->GetTcpListenThreadSocketCount());
        ImGui::Text("Total time spent updating TcpListenThread: %lld", aznumeric_cast<AZ::s64>(networking->GetTcpListenThreadUpdateTime()));
        ImGui::Text("Total sockets monitored by UdpReaderThread: %u", networking->GetUdpReaderThreadSocketCount());
        ImGui::Text("Total time spent updating UdpReaderThread: %lld", aznumeric_cast<AZ::s64>(networking->GetUdpReaderThreadUpdateTime()));
        ImGui::NewLine();

        for (auto& networkInterface : networking->GetNetworkInterfaces())
        {
            if (ImGui::CollapsingHeader(networkInterface.second->GetName().GetCStr()))
            {
                const char* protocol = networkInterface.second->GetType() == AzNetworking::ProtocolType::Tcp ? "Tcp" : "Udp";
                const char* trustZone = networkInterface.second->GetTrustZone() == AzNetworking::TrustZone::ExternalClientToServer ? "ExternalClientToServer" : "InternalServerToServer";
                const uint32_t port = aznumeric_cast<uint32_t>(networkInterface.second->GetPort());
                ImGui::Text("%sNetworkInterface open to %s on port %u", protocol, trustZone, port);

                if (ImGui::BeginTable("", 2, flags))
                {
                    const AzNetworking::NetworkInterfaceMetrics& metrics = networkInterface.second->GetMetrics();
                    ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
                    ImGui::TableHeadersRow();
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total time spent updating (ms)");
                    ImGui::TableNextColumn();
                    ImGui::Text("%lld", aznumeric_cast<AZ::s64>(metrics.m_updateTimeMs));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total number of connections");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_connectionCount));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total send time (ms)");
                    ImGui::TableNextColumn();
                    ImGui::Text("%lld", aznumeric_cast<AZ::s64>(metrics.m_sendTimeMs));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total sent packets");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::s64>(metrics.m_sendPackets));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total sent bytes after compression");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_sendBytes));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total sent bytes before compression");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_sendBytesUncompressed));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total sent compressed packets without benefit");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_sendCompressedPacketsNoGain));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total gain from packet compression");
                    ImGui::TableNextColumn();
                    ImGui::Text("%lld", aznumeric_cast<AZ::s64>(metrics.m_sendBytesCompressedDelta));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total packets resent");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_resentPackets));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total receive time (ms)");
                    ImGui::TableNextColumn();
                    ImGui::Text("%lld", aznumeric_cast<AZ::s64>(metrics.m_recvTimeMs));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total received packets");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_recvPackets));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total received bytes after compression");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_recvBytes));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total received bytes before compression");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_recvBytesUncompressed));
                    ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::Text("Total packets discarded due to load");
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", aznumeric_cast<AZ::u64>(metrics.m_discardedPackets));
                    ImGui::EndTable();
                }

                if (ImGui::BeginTable("", 7, flags))
                {
                    // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
                    ImGui::TableSetupColumn("RemoteAddr", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Conn. Id", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 6.0f);
                    ImGui::TableSetupColumn("Send (Bps)", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 10.0f);
                    ImGui::TableSetupColumn("Recv (Bps)", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 10.0f);
                    ImGui::TableSetupColumn("RTT (ms)", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 8.0f);
                    ImGui::TableSetupColumn("% Lost", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 8.0f);
                    ImGui::TableSetupColumn("Debug Settings", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 32.0f);
                    ImGui::TableHeadersRow();

                    auto displayConnectionRow = [](AzNetworking::IConnection& connection)
                    {
                        ImGui::PushID(&connection);

                        const AzNetworking::ConnectionMetrics& metrics = connection.GetMetrics();
                        const AzNetworking::IpAddress::IpString remoteAddr = connection.GetRemoteAddress().GetString();
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TreeNodeEx(remoteAddr.c_str(), nodeFlags);
                        ImGui::TableNextColumn();
                        ImGui::Text("%5llu", aznumeric_cast<AZ::u64>(connection.GetConnectionId()));
                        ImGui::TableNextColumn();
                        ImGui::Text("%9.2f", metrics.m_sendDatarate.GetBytesPerSecond());
                        ImGui::TableNextColumn();
                        ImGui::Text("%9.2f", metrics.m_recvDatarate.GetBytesPerSecond());
                        ImGui::TableNextColumn();
                        ImGui::Text("%7.2f", metrics.m_connectionRtt.GetRoundTripTimeSeconds() * 1000.0f);
                        ImGui::TableNextColumn();
                        ImGui::Text("%7.2f", metrics.m_sendDatarate.GetLossRatePercent());
                        ImGui::TableNextColumn();

                        {
                            AzNetworking::ConnectionQuality& quality = connection.GetConnectionQuality();
                            int32_t latency = aznumeric_cast<int32_t>(quality.m_latencyMs);
                            int32_t variance = aznumeric_cast<int32_t>(quality.m_varianceMs);
                            ImGui::SliderInt("Loss %", &quality.m_lossPercentage, 0, 100);
                            if (ImGui::SliderInt("Latency(ms)", &latency, 0, 3000))
                            {
                                quality.m_latencyMs = AZ::TimeMs{ latency };
                            }
                            if (ImGui::SliderInt("Jitter(ms)", &variance, 0, 1000))
                            {
                                quality.m_varianceMs = AZ::TimeMs{ variance };
                            }
                        }
                        ImGui::PopID();
                    };
                    networkInterface.second->GetConnectionSet().VisitConnections(displayConnectionRow);
                    ImGui::EndTable();
                }
            }
            ImGui::NewLine();
        }
        ImGui::End();
    }

    void DrawMultiplayerStats()
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

        if (ImGui::BeginTable("", 5, flags))
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

    void MultiplayerDebugSystemComponent::OnImGuiUpdate()
    {
        if (m_displayNetworkingStats)
        {
            if (ImGui::Begin("Networking Stats", &m_displayNetworkingStats, ImGuiWindowFlags_None))
            {
                DrawNetworkingStats();
            }
        }

        if (m_displayMultiplayerStats)
        {
            if (ImGui::Begin("Multiplayer Stats", &m_displayMultiplayerStats, ImGuiWindowFlags_None))
            {
                DrawMultiplayerStats();
            }
        }

        if (m_displayPerEntityStats)
        {
            if (ImGui::Begin("Multiplayer Per Entity Stats", &m_displayPerEntityStats, ImGuiWindowFlags_AlwaysAutoResize))
            {
                // This overrides @net_DebugNetworkEntity_ShowBandwidth value
                if (m_reporter == nullptr)
                {
                    ShowEntityBandwidthDebugOverlay();
                }

                if (m_reporter)
                {
                    m_reporter->OnImGuiUpdate();
                }
            }
        }

        if (m_displayHierarchyDebugger)
        {
            if (ImGui::Begin("Multiplayer Hierarchy Debugger", &m_displayHierarchyDebugger))
            {
                if (m_hierarchyDebugger == nullptr)
                {
                    m_hierarchyDebugger = AZStd::make_unique<MultiplayerDebugHierarchyReporter>();
                }

                if (m_hierarchyDebugger)
                {
                    m_hierarchyDebugger->OnImGuiUpdate();
                }
            }
        }
        else
        {
            if (m_hierarchyDebugger)
            {
                m_hierarchyDebugger.reset();
            }
        }
    }
#endif
}

void OnDebugEntities_ShowBandwidth_Changed(const bool& showBandwidth)
{
    if (showBandwidth)
    {
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->ShowEntityBandwidthDebugOverlay();
    }
    else
    {
        AZ::Interface<Multiplayer::IMultiplayerDebug>::Get()->HideEntityBandwidthDebugOverlay();
    }
}
