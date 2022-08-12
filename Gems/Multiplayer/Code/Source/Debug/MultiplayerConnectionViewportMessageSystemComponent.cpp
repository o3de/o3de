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
#include <AzCore/Console/ILogger.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzNetworking/Framework/INetworking.h>
#include <Multiplayer/IMultiplayerSpawner.h>
#include <Multiplayer/MultiplayerConstants.h>

namespace Multiplayer
{
    constexpr float defaultConnectionMessageFontSize = 0.7f;
    const AZ::Vector2 viewportConnectionBottomRightBorderPadding(-40.0f, -40.0f);

    AZ_CVAR_SCOPED(bool, bg_viewportConnectionStatus, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "This will enable displaying connection status in the client's viewport while running multiplayer.");

    AZ_CVAR_SCOPED(float, bg_viewportConnectionMessageFontSize, defaultConnectionMessageFontSize, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, 
        "The font size used for displaying updates on screen while the multiplayer editor is connecting to the server.");

    AZ_CVAR_SCOPED(int, cl_viewportConnectionStatusMaxDrawCount, 4, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, 
        "Limits the number of connect statuses seen in the viewport. Generally, clients are connected to 1 server, but defining a max draw count in case other connections are established.");

    AZ_CVAR_EXTERNED(bool, sv_isDedicated);

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
        MultiplayerEditorServerNotificationBus::Handler::BusConnect();
    }

    void MultiplayerConnectionViewportMessageSystemComponent::Deactivate()
    {
        MultiplayerEditorServerNotificationBus::Handler::BusDisconnect();
        AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnRenderTick()
    {
        if (!bg_viewportConnectionStatus)
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
        m_drawParams.m_scale = AZ::Vector2(bg_viewportConnectionMessageFontSize);
        m_lineSpacing = 0.5f*m_fontDrawInterface->GetTextSize(m_drawParams, " ").GetY();

        AzFramework::WindowSize viewportSize = viewport->GetViewportSize();

        // Display the custom center viewport text
        if (!m_centerViewportDebugText.empty())
        {            
            const float center_screenposition_x = 0.5f*viewportSize.m_width;
            const float center_screenposition_y = 0.5f*viewportSize.m_height;

            // Draw title
            const float textHeight = m_fontDrawInterface->GetTextSize(m_drawParams, CenterViewportDebugTitle).GetY();
            const float screenposition_title_y = center_screenposition_y-textHeight*0.5f;
            m_drawParams.m_position = AZ::Vector3(center_screenposition_x, screenposition_title_y, 1.0f);
            m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Center;
            m_drawParams.m_color = m_centerViewportDebugTextColor;
            m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, CenterViewportDebugTitle);
            
            // Draw center debug text under the title
            // Calculate line spacing based on the font's actual line height
            m_drawParams.m_color = AZ::Colors::White;
            m_drawParams.m_position.SetY(m_drawParams.m_position.GetY() + textHeight + m_lineSpacing);
            m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, m_centerViewportDebugText.c_str());
        }

        // Build the connection status string (just show client connected or disconnected status for now)
        const auto multiplayerSystemComponent = AZ::Interface<IMultiplayer>::Get();
        MultiplayerAgentType agentType = multiplayerSystemComponent->GetAgentType();

        // Display the connection status in the bottom-right viewport
        m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Right;
        m_drawParams.m_position = AZ::Vector3(aznumeric_cast<float>(viewportSize.m_width), aznumeric_cast<float>(viewportSize.m_height), 1.0f) + AZ::Vector3(viewportConnectionBottomRightBorderPadding) * viewport->GetDpiScalingFactor();

        AzNetworking::INetworkInterface* networkInterface = AZ::Interface<AzNetworking::INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpNetworkInterfaceName));
        switch (agentType)
        {
        case MultiplayerAgentType::Uninitialized:
            if (sv_isDedicated)
            {
                DrawConnectionStatusLine(DedicatedServerNotHosting, AZ::Colors::Red);
                DrawConnectionStatusLine(DedicatedServerStatusTitle, AZ::Colors::White);
            }
            break;
        case MultiplayerAgentType::Client:
            {
                if (!networkInterface)
                {
                    break;
                }

                AzNetworking::IConnectionSet& connectionSet = networkInterface->GetConnectionSet();
                m_currentConnectionsDrawCount = 0;
                if (connectionSet.GetConnectionCount() > 0)
                {
                    connectionSet.VisitConnections(
                        [this](AzNetworking::IConnection& connection)
                        {
                            m_hostIpAddress = connection.GetRemoteAddress();
                            this->DrawConnectionStatus(connection.GetConnectionState(), m_hostIpAddress);
                        });
                }
                else
                {
                    // If we're a client yet are lacking a connection then we've been unintentionally disconnected
                    // Display a disconnect message in the viewport
                    DrawConnectionStatus(AzNetworking::ConnectionState::Disconnected, m_hostIpAddress);
                }
            }
            break;
        case MultiplayerAgentType::ClientServer:
            {
                if (!networkInterface)
                {
                    break;
                }
                const auto clientServerHostingPort =
                    AZStd::fixed_string<MaxMessageLength>::format(ServerHostingPort, networkInterface->GetPort());
                const auto clientServerClientCount = AZStd::fixed_string<MaxMessageLength>::format(
                    ClientServerHostingClientCount, 1+networkInterface->GetConnectionSet().GetConnectionCount());

                DrawConnectionStatusLine(clientServerClientCount.c_str(), AZ::Colors::Green);
                DrawConnectionStatusLine(clientServerHostingPort.c_str(), AZ::Colors::Green);
                DrawConnectionStatusLine(ClientServerStatusTitle, AZ::Colors::White);
                break;
            }
        case MultiplayerAgentType::DedicatedServer:
            {
                if (!networkInterface)
                {
                    break;
                }
                const auto dedicatedServerHostingPort = AZStd::fixed_string<MaxMessageLength>::format(
                    ServerHostingPort, networkInterface->GetPort());
                const auto dedicatedServerClientCount = AZStd::fixed_string<MaxMessageLength>::format(
                    DedicatedServerHostingClientCount, networkInterface->GetConnectionSet().GetConnectionCount());

                const AZ::Color serverHostStatusColor = networkInterface->GetConnectionSet().GetConnectionCount() > 0 ? AZ::Colors::Green : AZ::Colors::Yellow;
                DrawConnectionStatusLine(dedicatedServerClientCount.c_str(), serverHostStatusColor);
                DrawConnectionStatusLine(dedicatedServerHostingPort.c_str(), serverHostStatusColor);
                DrawConnectionStatusLine(DedicatedServerStatusTitle, AZ::Colors::White);
                break;
            }
        default:
            AZLOG_ERROR(
                "MultiplayerConnectionViewportMessageSystemComponent doesn't support drawing status for multiplayer agent type %s. Please update code to support the new agent type.",
                GetEnumString(agentType));
            break;
        }
    }

    void MultiplayerConnectionViewportMessageSystemComponent::DrawConnectionStatus(AzNetworking::ConnectionState connectionState, const AzNetworking::IpAddress& hostIpAddress)
    {
        // Limit the amount of connections we draw on screen
        if (m_currentConnectionsDrawCount >= cl_viewportConnectionStatusMaxDrawCount)
        {
            return;
        }
        ++m_currentConnectionsDrawCount;

        AZ::Color connectionStateColor;
        switch (connectionState)
        {
        case AzNetworking::ConnectionState::Connecting:
            connectionStateColor = AZ::Colors::Yellow;
            break;
        case AzNetworking::ConnectionState::Connected:
            connectionStateColor = AZ::Colors::Green;
            break;
        case AzNetworking::ConnectionState::Disconnecting:
            connectionStateColor = AZ::Colors::Yellow;
            break;
        case AzNetworking::ConnectionState::Disconnected:
            connectionStateColor = AZ::Colors::Red;
            break;
        default:
            connectionStateColor = AZ::Colors::White;
        }

        // Draw our host's remote ip address
        const auto multiplayerSystemComponent = AZ::Interface<IMultiplayer>::Get();
        MultiplayerAgentType agentType = multiplayerSystemComponent->GetAgentType();
        if (agentType == MultiplayerAgentType::Client)
        {
            auto hostAddressText = AZStd::fixed_string<32>::format("Server IP %s", hostIpAddress.GetString().c_str());
            DrawConnectionStatusLine(hostAddressText.c_str(), connectionStateColor);
        }

        // Draw the connect state (example: Connected or Disconnected)
        DrawConnectionStatusLine(ToString(connectionState).data(), connectionStateColor);

        // Draw the status title
        DrawConnectionStatusLine(ClientStatusTitle, AZ::Colors::White);
    }

    void MultiplayerConnectionViewportMessageSystemComponent::DrawConnectionStatusLine(const char* line, const AZ::Color& color)
    {
        m_drawParams.m_color = color;
        m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, line);

        // Status text renders in the lower right corner, so we draw from the bottom up.
        // Move the font draw position up to get ready for the next text line.
        const float textHeight = m_fontDrawInterface->GetTextSize(m_drawParams, line).GetY();
        m_drawParams.m_position.SetY(m_drawParams.m_position.GetY() - textHeight - m_lineSpacing);
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnServerLaunched()
    {
        m_centerViewportDebugTextColor = AZ::Colors::Yellow;
        m_centerViewportDebugText = OnServerLaunchedMessage;
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnServerLaunchFail()
    {
        m_centerViewportDebugTextColor = AZ::Colors::Red;
        m_centerViewportDebugText = OnServerLaunchFailMessage;
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnEditorSendingLevelData()
    {
        m_centerViewportDebugTextColor = AZ::Colors::Yellow;
        m_centerViewportDebugText = OnEditorSendingLevelDataMessage;
    }   

    void MultiplayerConnectionViewportMessageSystemComponent::OnEditorConnectionAttempt(uint16_t connectionAttempts, uint16_t maxAttempts)
    {
        m_centerViewportDebugTextColor = AZ::Colors::Yellow;
        m_centerViewportDebugText = AZStd::fixed_string<MaxMessageLength>::format(OnEditorConnectionAttemptMessage, connectionAttempts, maxAttempts);
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnEditorConnectionAttemptsFailed(uint16_t failedAttempts)
    {
        m_centerViewportDebugTextColor = AZ::Colors::Red;
        m_centerViewportDebugText = AZStd::fixed_string<MaxMessageLength>::format(OnEditorConnectionAttemptsFailedMessage, failedAttempts);
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnConnectToSimulationFail(uint16_t serverPort)
    {
        m_centerViewportDebugTextColor = AZ::Colors::Red;
        m_centerViewportDebugText = AZStd::fixed_string<MaxMessageLength>::format(OnConnectToSimulationFailMessage, serverPort);
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnConnectToSimulationSuccess() 
    {
        m_centerViewportDebugText.clear();
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnPlayModeEnd()
    {
        m_centerViewportDebugText.clear();
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnEditorServerProcessStoppedUnexpectedly()
    {
        m_centerViewportDebugTextColor = AZ::Colors::Red;
        m_centerViewportDebugText = OnEditorServerStoppedUnexpectedly;
    }

}
