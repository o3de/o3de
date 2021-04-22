/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Source/Debug/MultiplayerDebugSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Interface/Interface.h>
#include <Include/IMultiplayer.h>

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

#ifdef IMGUI_ENABLED
    void MultiplayerDebugSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("Multiplayer"))
        {
            //{
            //    static int lossPercent{ 0 };
            //    lossPercent = static_cast<int>(net_UdpDebugLossPercent);
            //    if (ImGui::SliderInt("UDP Loss Percent", &lossPercent, 0, 100))
            //    {
            //        net_UdpDebugLossPercent = lossPercent;
            //        m_ClientAgent.UpdateConnectionCvars(net_UdpDebugLossPercent);
            //    }
            //}
            //
            //{
            //    static int latency{ 0 };
            //    latency = static_cast<int>(net_UdpDebugLatencyMs);
            //    if (ImGui::SliderInt("UDP Latency Ms", &latency, 0, 3000))
            //    {
            //        net_UdpDebugLatencyMs = latency;
            //        m_ClientAgent.UpdateConnectionCvars(net_UdpDebugLatencyMs);
            //    }
            //}
            //
            //{
            //    static int variance{ 0 };
            //    variance = static_cast<int>(net_UdpDebugVarianceMs);
            //    if (ImGui::SliderInt("UDP Variance Ms", &variance, 0, 1000))
            //    {
            //        net_UdpDebugVarianceMs = variance;
            //        m_ClientAgent.UpdateConnectionCvars(net_UdpDebugVarianceMs);
            //    }
            //}

            ImGui::Checkbox("Multiplayer Stats", &m_displayStats);
            ImGui::EndMenu();
        }
    }

    void ComputePerSecondValues(const MultiplayerStats& stats, const MultiplayerStats::Metric& metric, float& outCallsPerSecond, float& outBytesPerSecond)
    {
        uint64_t summedCalls = 0;
        uint64_t summedBytes = 0;
        for (uint32_t index = 0; index < MultiplayerStats::RingbufferSamples; ++index)
        {
            summedCalls += metric.m_callHistory[index];
            summedBytes += metric.m_byteHistory[index];
        }
        const float totalTimeSeconds = static_cast<float>(stats.m_totalHistoryTimeMs) / 1000.0f;
        outCallsPerSecond = static_cast<float>(summedCalls) / totalTimeSeconds;
        outBytesPerSecond = static_cast<float>(summedBytes) / totalTimeSeconds;
    }

    void DrawMetricTitle(const ImVec4& entryColour)
    {
        ImGui::Columns(6);

        ImGui::TextColored(entryColour, "Name"); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "Category"); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "Total Calls"); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "Total Bytes"); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "Calls/Sec"); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "Bytes/Sec"); ImGui::NextColumn();
    }

    void DrawMetricRow(const char* name, const char* category, const ImVec4& entryColour, const MultiplayerStats& stats, const MultiplayerStats::Metric& metric)
    {
        float callsPerSecond = 0.0f;
        float bytesPerSecond = 0.0f;
        ComputePerSecondValues(stats, metric, callsPerSecond, bytesPerSecond);

        ImGui::TextColored(entryColour, "%s", name); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "%s", category); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "%10llu", aznumeric_cast<AZ::u64>(metric.m_totalCalls)); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "%10llu", aznumeric_cast<AZ::u64>(metric.m_totalBytes)); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "%10.2f", callsPerSecond); ImGui::NextColumn();
        ImGui::TextColored(entryColour, "%10.2f", bytesPerSecond); ImGui::NextColumn();
    }

    void MultiplayerDebugSystemComponent::OnImGuiUpdate()
    {
        const ImVec4 titleColour = ImColor(1.00f, 0.80f, 0.12f);
        const ImVec4 entryColour = ImColor(0.32f, 1.00f, 1.00f);

        if (m_displayStats)
        {
            if (ImGui::Begin("Multiplayer Stats", &m_displayStats, ImGuiWindowFlags_HorizontalScrollbar))
            {
                IMultiplayer* multiplayer = AZ::Interface<IMultiplayer>::Get();
                const Multiplayer::MultiplayerStats& stats = multiplayer->GetStats();
                ImGui::Text("Multiplayer operating in %s mode", GetEnumString(multiplayer->GetAgentType()));
                ImGui::Text("Total networked entities: %llu", aznumeric_cast<AZ::u64>(stats.m_entityCount));
                ImGui::Text("Total client connections: %llu", aznumeric_cast<AZ::u64>(stats.m_clientConnectionCount));
                ImGui::Text("Total server connections: %llu", aznumeric_cast<AZ::u64>(stats.m_serverConnectionCount));

                const MultiplayerStats::Metric propertyUpdatesSent = stats.CalculateTotalPropertyUpdateSentMetrics();
                const MultiplayerStats::Metric propertyUpdatesRecv = stats.CalculateTotalPropertyUpdateRecvMetrics();
                const MultiplayerStats::Metric rpcsSent = stats.CalculateTotalRpcsSentMetrics();
                const MultiplayerStats::Metric rpcsRecv = stats.CalculateTotalRpcsRecvMetrics();

                DrawMetricTitle(titleColour);
                DrawMetricRow("Total", "PropertyUpdates Sent", entryColour, stats, propertyUpdatesSent);
                DrawMetricRow("Total", "PropertyUpdates Received", entryColour, stats, propertyUpdatesRecv);
                DrawMetricRow("Total", "Rpcs Sent", entryColour, stats, rpcsSent);
                DrawMetricRow("Total", "Rpcs Received", entryColour, stats, rpcsRecv);
            }
            ImGui::End();
        }
    }
#endif
}
