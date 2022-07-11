/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "MultiplayerConnectionViewportMessageSystemComponent.h"
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AzCore/Console/IConsole.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzNetworking/Framework/INetworking.h>
#include <Multiplayer/IMultiplayerSpawner.h>
#include <Multiplayer/MultiplayerConstants.h>

namespace Multiplayer
{
    constexpr float defaultConnectionMessageFontSize = 0.7f;
    const AZ::Vector2 viewportConnectionBottomRightBorderPadding(-40.0f, -40.0f);

    AZ_CVAR_SCOPED(bool, cl_viewportConnectionStatus, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "This will enable displaying connection status in the client's viewport while running multiplayer.");

    AZ_CVAR_SCOPED(float, cl_viewportConnectionMessageFontSize, defaultConnectionMessageFontSize, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, 
        "The font size used for displaying updates on screen while the multiplayer editor is connecting to the server.");
    
    void MultiplayerConnectionViewportMessageSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerConnectionViewportMessageSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }
    
    void MultiplayerConnectionViewportMessageSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("MultiplayerService"));
    }

    void MultiplayerConnectionViewportMessageSystemComponent::Activate()
    {
        AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(
            AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContextName());
    }

    void MultiplayerConnectionViewportMessageSystemComponent::Deactivate()
    {
        AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnRenderTick()
    {
        if (!cl_viewportConnectionStatus)
        {
            return;
        }

        AZ::RPI::ViewportContextPtr viewport = AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
        if (!viewport)
        {
            return;
        }

        auto fontQueryInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get();
        if (!fontQueryInterface)
        {
            return;
        }

        m_fontDrawInterface = fontQueryInterface->GetDefaultFontDrawInterface();
        if (!m_fontDrawInterface)
        {
            return;
        }

        m_drawParams.m_drawViewportId = viewport->GetId();
        m_drawParams.m_scale = AZ::Vector2(cl_viewportConnectionMessageFontSize);
        m_lineSpacing = 0.5f*m_fontDrawInterface->GetTextSize(m_drawParams, " ").GetY();

        AzFramework::WindowSize viewportSize = viewport->GetViewportSize();

        // Display the custom center viewport text
        if (!m_centerViewportDebugText.empty())
        {            
            const float center_screenposition_x = 0.5f*viewportSize.m_width;
            const float center_screenposition_y = 0.5f*viewportSize.m_height;

            // Draw title
            const char* centerViewportDebugTextTitle = "Multiplayer Editor";
            const float textHeight = m_fontDrawInterface->GetTextSize(m_drawParams, centerViewportDebugTextTitle).GetY();
            const float screenposition_title_y = center_screenposition_y-textHeight*0.5f;
            m_drawParams.m_position = AZ::Vector3(center_screenposition_x, screenposition_title_y, 1.0f);
            m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Center;
            m_drawParams.m_color = AZ::Colors::Yellow;
            m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, centerViewportDebugTextTitle);
            
            // Draw center debug text under the title
            // Calculate line spacing based on the font's actual line height
            m_drawParams.m_color = AZ::Colors::White;
            m_drawParams.m_position.SetY(m_drawParams.m_position.GetY() + textHeight + m_lineSpacing);
            m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, m_centerViewportDebugText.c_str());
        }

        // Build the connection status string (just show client connected or disconnected status for now)
        const auto multiplayerSystemComponent = AZ::Interface<IMultiplayer>::Get();
        MultiplayerAgentType agentType = multiplayerSystemComponent->GetAgentType();        
        if (agentType == MultiplayerAgentType::Client)
        {        
            // Display the connection status in the bottom-right viewport
            if (AzNetworking::INetworkInterface* networkInterface = AZ::Interface<AzNetworking::INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpNetworkInterfaceName)))
            {
                AzNetworking::IConnectionSet& connectionSet = networkInterface->GetConnectionSet();
                if (connectionSet.GetConnectionCount() > 0)
                {
                    connectionSet.VisitConnections([this](AzNetworking::IConnection& connection){ this->DrawConnectionStatus(connection.GetConnectionState()); });
                }
                else
                {
                    // If we're a client yet are lacking a connection then we've been unintentionally disconnected
                    // Display a disconnect message in the viewport
                    DrawConnectionStatus(AzNetworking::ConnectionState::Disconnected);
                }
            }
        }
    }

    void MultiplayerConnectionViewportMessageSystemComponent::DisplayCenterViewportMessage(const char* text)
    {
        if (strlen(text) == 0)
        {
            StopCenterViewportDebugMessaging();
            return;
        }
        
        m_centerViewportDebugText = text;
    }
    
    void MultiplayerConnectionViewportMessageSystemComponent::StopCenterViewportDebugMessaging()
    {
        m_centerViewportDebugText.clear();
    }

    void MultiplayerConnectionViewportMessageSystemComponent::DrawConnectionStatus(AzNetworking::ConnectionState connectionState)
    {
        AZ::RPI::ViewportContextPtr viewport = AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
        if (!viewport)
        {
            return;
        }

        // Draw the status (example: Connected or Disconnected)
        const char* connectionStateText = ToString(connectionState).data();
        const AzFramework::WindowSize viewportSize = viewport->GetViewportSize();
        m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Right;
        m_drawParams.m_position = AZ::Vector3(static_cast<float>(viewportSize.m_width), static_cast<float>(viewportSize.m_height), 1.0f) + AZ::Vector3(viewportConnectionBottomRightBorderPadding) * viewport->GetDpiScalingFactor();
        
        switch (connectionState)
        {
        case AzNetworking::ConnectionState::Connecting:
            m_drawParams.m_color = AZ::Colors::Yellow;
            break;
        case AzNetworking::ConnectionState::Connected:
            m_drawParams.m_color = AZ::Colors::Green;
            break;
        case AzNetworking::ConnectionState::Disconnecting:
            m_drawParams.m_color = AZ::Colors::Orange;
            break;
        case AzNetworking::ConnectionState::Disconnected:
            m_drawParams.m_color = AZ::Colors::Red;
            break;
        default: m_drawParams.m_color = AZ::Colors::White;
        }

        m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, connectionStateText);

        // Draw the status title above the current status
        const float textHeight = m_fontDrawInterface->GetTextSize(m_drawParams, connectionStateText).GetY();
        m_drawParams.m_color = AZ::Colors::White;
        m_drawParams.m_position.SetY(m_drawParams.m_position.GetY() - textHeight - m_lineSpacing);
        m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, "Multiplayer Client Status:");
    }
}
