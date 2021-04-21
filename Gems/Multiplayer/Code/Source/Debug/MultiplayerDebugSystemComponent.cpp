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

    void MultiplayerDebugSystemComponent::OnImGuiUpdate()
    {
        if (m_displayStats)
        {
            if (ImGui::Begin("Multiplayer Stats", &m_displayStats, ImGuiWindowFlags_HorizontalScrollbar))
            {
                IMultiplayer* multiplayer = AZ::Interface<IMultiplayer>::Get();
                Multiplayer::MultiplayerStats& stats = multiplayer->GetStats();
                ImGui::Text("Multiplayer operating in %s mode", GetEnumString(multiplayer->GetAgentType()));
                ImGui::Text("Total networked entities: %llu", aznumeric_cast<AZ::u64>(stats.m_entityCount));
                ImGui::Text("Total client connections: %llu", aznumeric_cast<AZ::u64>(stats.m_clientConnectionCount));
                ImGui::Text("Total server connections: %llu", aznumeric_cast<AZ::u64>(stats.m_serverConnectionCount));
                ImGui::Text("Total property updates sent: %llu", aznumeric_cast<AZ::u64>(stats.m_propertyUpdatesSent));
                ImGui::Text("Total property updates sent bytes: %llu", aznumeric_cast<AZ::u64>(stats.m_propertyUpdatesSentBytes));
                ImGui::Text("Total property updates received: %llu", aznumeric_cast<AZ::u64>(stats.m_propertyUpdatesRecv));
                ImGui::Text("Total property updates received bytes: %llu", aznumeric_cast<AZ::u64>(stats.m_propertyUpdatesRecvBytes));
                ImGui::Text("Total RPCs sent: %llu", aznumeric_cast<AZ::u64>(stats.m_rpcsSent));
                ImGui::Text("Total RPCs sent bytes: %llu", aznumeric_cast<AZ::u64>(stats.m_rpcsSentBytes));
                ImGui::Text("Total RPCs received: %llu", aznumeric_cast<AZ::u64>(stats.m_rpcsRecv));
                ImGui::Text("Total RPCs received bytes: %llu", aznumeric_cast<AZ::u64>(stats.m_rpcsRecvBytes));
            }
            ImGui::End();
        }
    }
#endif
}
