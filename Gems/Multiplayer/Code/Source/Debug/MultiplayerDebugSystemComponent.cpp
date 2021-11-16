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
        m_networkMetrics = AZStd::make_unique<MultiplayerDebugNetworkMetrics>();
        m_multiplayerMetrics = AZStd::make_unique<MultiplayerDebugMultiplayerMetrics>();
#endif
    }

    void MultiplayerDebugSystemComponent::Deactivate()
    {
#ifdef IMGUI_ENABLED
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif
    }

#ifdef IMGUI_ENABLED
    void MultiplayerDebugSystemComponent::ShowEntityBandwidthDebugOverlay()
    {
        m_reporter = AZStd::make_unique<MultiplayerDebugPerEntityReporter>();
    }

    void MultiplayerDebugSystemComponent::HideEntityBandwidthDebugOverlay()
    {
        m_reporter.reset();
    }

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

    void MultiplayerDebugSystemComponent::OnImGuiUpdate()
    {
        if (m_displayNetworkingStats)
        {
            if (ImGui::Begin("Networking Stats", &m_displayNetworkingStats, ImGuiWindowFlags_None))
            {
                m_networkMetrics->OnImGuiUpdate();
            }
        }

        if (m_displayMultiplayerStats)
        {
            if (ImGui::Begin("Multiplayer Stats", &m_displayMultiplayerStats, ImGuiWindowFlags_None))
            {
                m_multiplayerMetrics->OnImGuiUpdate();
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
