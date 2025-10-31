/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Debug/MultiplayerDebugNetworkMetrics.h>
#include <AzNetworking/Framework/INetworking.h>

namespace Multiplayer
{
#ifdef IMGUI_ENABLED
    void MultiplayerDebugNetworkMetrics::OnImGuiUpdate()
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
            const AZ::Name& name = networkInterface.first;
            const AzNetworking::NetworkInterfaceMetrics& metrics = networkInterface.second->GetMetrics();
            const bool needsInit = m_sendHistograms.find(name) == m_sendHistograms.end();

            NetworkMetricDisplay& sendHistogram = m_sendHistograms[name];
            NetworkMetricDisplay& recvHistogram = m_recvHistograms[name];
            if (needsInit)
            {
                AZ::CVarFixedString sendName = name.GetCStr();
                sendName += " Send (Bytes/Sec)";
                sendHistogram.m_histogram.Init(sendName.c_str(), 250, ImGui::LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 0.0f, 100.0f);

                AZ::CVarFixedString recvName = name.GetCStr();
                recvName += " Receive (Bytes/Sec)";
                recvHistogram.m_histogram.Init(recvName.c_str(), 250, ImGui::LYImGuiUtils::HistogramContainer::ViewType::Histogram, true, 0.0f, 100.0f);
            }

            sendHistogram.m_histogram.PushValue(aznumeric_cast<float>(metrics.m_sendBytes - sendHistogram.m_lastValue));
            sendHistogram.m_lastValue = metrics.m_sendBytes;

            recvHistogram.m_histogram.PushValue(aznumeric_cast<float>(metrics.m_recvBytes - recvHistogram.m_lastValue));
            recvHistogram.m_lastValue = metrics.m_recvBytes;

            if (ImGui::CollapsingHeader(networkInterface.second->GetName().GetCStr()))
            {
                const char* protocol = networkInterface.second->GetType() == AzNetworking::ProtocolType::Tcp ? "Tcp" : "Udp";
                const char* trustZone = networkInterface.second->GetTrustZone() == AzNetworking::TrustZone::ExternalClientToServer ? "ExternalClientToServer" : "InternalServerToServer";
                const uint32_t port = aznumeric_cast<uint32_t>(networkInterface.second->GetPort());
                ImGui::Text("%sNetworkInterface open to %s on port %u", protocol, trustZone, port);

                sendHistogram.m_histogram.Draw(ImGui::GetColumnWidth(), 100.0f);
                recvHistogram.m_histogram.Draw(ImGui::GetColumnWidth(), 100.0f);

                if (ImGui::BeginTable("Traffic Details", 2, flags))
                {
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

                if (ImGui::BeginTable("Interface Overview", 7, flags))
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
#endif
}
