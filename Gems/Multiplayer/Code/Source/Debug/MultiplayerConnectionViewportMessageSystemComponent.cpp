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

        if (auto multiplayerSystemComponent = AZ::Interface<IMultiplayer>::Get())
        {
            multiplayerSystemComponent->AddVersionMismatchHandler(m_versionMismatchEventHandler);
            multiplayerSystemComponent->AddLevelLoadBlockedHandler(m_levelLoadBlockedHandler);
            multiplayerSystemComponent->AddNoServerLevelLoadedHandler(m_noServerLevelLoadedHandler);
        }
    }

    void MultiplayerConnectionViewportMessageSystemComponent::Deactivate()
    {
        m_noServerLevelLoadedHandler.Disconnect();
        m_levelLoadBlockedHandler.Disconnect();
        m_versionMismatchEventHandler.Disconnect();
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
        DrawCenterViewportMessage(CenterViewportDebugTitle, m_centerViewportDebugTextColor, m_centerViewportDebugText.c_str(), 1.0f);
        
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
            if (const auto console = AZ::Interface<AZ::IConsole>::Get())
            {
                bool isDedicatedServer = false;
                if (console->GetCvarValue("sv_isDedicated", isDedicatedServer) != AZ::GetValueResult::Success)
                {
                    AZLOG_WARN("MultiplayerConnectionViewport failed to access cvar  (sv_isDedicated).")
                    break;
                }

                if (isDedicatedServer)
                {
                    DrawConnectionStatusLine(DedicatedServerNotHosting, AZ::Colors::Red);
                    DrawConnectionStatusLine(DedicatedServerStatusTitle, AZ::Colors::White);
                }
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

        // Display the viewport toast text
        if (!m_centerViewportDebugToastText.empty())
        {
            // Fade out the toast over time
            const size_t wordCount = m_centerViewportDebugToastText.find(' ') + 1; // Word count estimated by counting the number of spaces and adding 1.
            const AZ::TimeMs toastDuration = static_cast<AZ::TimeMs>(wordCount) * CenterViewportDebugToastTimePerWord + CenterViewportDebugToastTimePrefix + CenterViewportDebugToastTimeFade;
            const AZ::TimeMs currentTime = static_cast<AZ::TimeMs>(AZStd::GetTimeUTCMilliSecond());
            const AZ::TimeMs remainingTime = toastDuration - (currentTime - m_centerViewportDebugToastStartTime);
            const float toastAlpha = AZStd::clamp(aznumeric_cast<float>(remainingTime) / aznumeric_cast<float>(CenterViewportDebugToastTimeFade), 0.0f, 1.0f);
            DrawCenterViewportMessage(CenterViewportToastTitle, AZ::Colors::Red, m_centerViewportDebugToastText.c_str(), toastAlpha);

            if (toastAlpha < 0.01f)
            {
                // toast is completely faded out, remove the toast
                m_centerViewportDebugToastText.clear();
            }
        }
    }

    void MultiplayerConnectionViewportMessageSystemComponent::DrawCenterViewportMessage(const char* title, const AZ::Color& titleColor, const char* message, float alpha)
    {
        const AZ::RPI::ViewportContextPtr viewport = AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
        if (!viewport)
        {
            return;
        }
        
        // make sure there's a title and message to render
        if (title == nullptr || strlen(title) == 0)
        {
            return;
        }

        if (message == nullptr || strlen(message) == 0)
        {
            return;
        }

        // only render text that will be visible
        if (alpha < 0.01f)
        {
            return;
        }

        // Draw background for text contrast
        DrawScrim(alpha);

        // Find viewport center
        AzFramework::WindowSize viewportSize = viewport->GetViewportSize();
        const float center_screenposition_x = 0.5f * viewportSize.m_width;
        const float center_screenposition_y = 0.5f * viewportSize.m_height;

        // Draw title
        const float textHeight = m_fontDrawInterface->GetTextSize(m_drawParams, title).GetY();
        const float screenposition_title_y = center_screenposition_y - textHeight * 0.5f;
        m_drawParams.m_position = AZ::Vector3(center_screenposition_x, screenposition_title_y, 1.0f);
        m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Center;
        m_drawParams.m_color = titleColor;
        m_drawParams.m_color.SetA(alpha);
        m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, title);

        // Draw message under the title
        // Calculate line spacing based on the font's actual line height
        m_drawParams.m_color = AZ::Colors::White;
        m_drawParams.m_color.SetA(alpha);
        m_drawParams.m_position.SetY(m_drawParams.m_position.GetY() + textHeight + m_lineSpacing);
        m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, message);
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

    void MultiplayerConnectionViewportMessageSystemComponent::DrawScrim(float alphaMultiplier) const
    {
        AZ::RPI::ViewportContextPtr viewport = AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
        if (!viewport)
        {
            return;
        }

        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, viewport->GetId());
        AzFramework::DebugDisplayRequests*  debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
        if (!debugDisplay)
        {
            return;
        }

        // we're going to alter the state of depth write and test, store it here so we can restore when we're done drawing.
        const AZ::u32 previousState = debugDisplay->GetState();
        debugDisplay->DepthWriteOff();
        debugDisplay->DepthTestOff();
        debugDisplay->DrawQuad2dGradient(
            AZ::Vector2(0, 0),
            AZ::Vector2(1.f, 0),
            AZ::Vector2(1.f, 0.5f),
            AZ::Vector2(0, 0.5f),
            0,
            AZ::Color(0.f, 0.f, 0.f, 0.f),
            AZ::Color(0, 0, 0, ScrimAlpha * alphaMultiplier));

        debugDisplay->DrawQuad2dGradient(
            AZ::Vector2(0, 0.5f),
            AZ::Vector2(1.f, 0.5f),
            AZ::Vector2(1.f, 1.f),
            AZ::Vector2(0, 1.f),
            0,
            AZ::Color(0, 0, 0, ScrimAlpha * alphaMultiplier),
            AZ::Color(0.f, 0.f, 0.f, 0.f));

        debugDisplay->SetState(previousState);
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

    void MultiplayerConnectionViewportMessageSystemComponent::OnEditorSendingLevelData(uint32_t bytesSent, uint32_t bytesTotal)
    {
        m_centerViewportDebugTextColor = AZ::Colors::Yellow;
        m_centerViewportDebugText =
            AZStd::fixed_string<MaxMessageLength>::format(OnEditorSendingLevelDataMessage, bytesSent, bytesTotal);
    }   

    void MultiplayerConnectionViewportMessageSystemComponent::OnEditorSendingLevelDataFailed()
    {
        m_centerViewportDebugTextColor = AZ::Colors::Red;
        m_centerViewportDebugText = OnEditorSendingLevelDataFailedMessage;
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnEditorSendingLevelDataSuccess()
    {
        m_centerViewportDebugTextColor = AZ::Colors::Yellow;
        m_centerViewportDebugText = OnEditorSendingLevelDataSuccessMessage;
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

    void MultiplayerConnectionViewportMessageSystemComponent::OnBlockedLevelLoad()
    {
        m_centerViewportDebugToastStartTime = static_cast<AZ::TimeMs>(AZStd::GetTimeUTCMilliSecond());
        m_centerViewportDebugToastText = OnBlockedLevelLoadMessage;
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnNoServerLevelLoadedEvent()
    {
        const auto multiplayerSystemComponent = AZ::Interface<IMultiplayer>::Get();
        if (!multiplayerSystemComponent)
        {
            return;
        }

        const MultiplayerAgentType agentType = multiplayerSystemComponent->GetAgentType();
        if (agentType == MultiplayerAgentType::Client)
        {
            m_centerViewportDebugToastText = OnNoServerLevelLoadedMessageClientSide;
        }
        else
        {
            m_centerViewportDebugToastText = OnNoServerLevelLoadedMessageServerSide;
        }
        m_centerViewportDebugToastStartTime = static_cast<AZ::TimeMs>(AZStd::GetTimeUTCMilliSecond());
    }

    void MultiplayerConnectionViewportMessageSystemComponent::OnVersionMismatchEvent()
    {
        m_centerViewportDebugToastText = OnVersionMismatch;
        m_centerViewportDebugToastStartTime = static_cast<AZ::TimeMs>(AZStd::GetTimeUTCMilliSecond());
    }
}
