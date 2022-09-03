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


   enum EAuxGeomPublicRenderflagBitMasks
    {
        e_Mode2D3DShift = 31,
        e_Mode2D3DMask = 0x1 << e_Mode2D3DShift,

        e_AlphaBlendingShift = 29,
        e_AlphaBlendingMask = 0x3 << e_AlphaBlendingShift,

        e_DrawInFrontShift = 28,
        e_DrawInFrontMask = 0x1 << e_DrawInFrontShift,

        e_FillModeShift = 26,
        e_FillModeMask = 0x3 << e_FillModeShift,

        e_CullModeShift = 24,
        e_CullModeMask = 0x3 << e_CullModeShift,

        e_DepthWriteShift = 23,
        e_DepthWriteMask = 0x1 << e_DepthWriteShift,

        e_DepthTestShift = 22,
        e_DepthTestMask = 0x1 << e_DepthTestShift,

        e_PublicParamsMask =
            e_Mode2D3DMask | e_AlphaBlendingMask | e_DrawInFrontMask | e_FillModeMask | e_CullModeMask | e_DepthWriteMask | e_DepthTestMask
    };

    enum EAuxGeomPublicRenderflags_Mode2D3D
    {
        e_Mode3D = 0x0 << e_Mode2D3DShift,
        e_Mode2D = 0x1 << e_Mode2D3DShift,
    };

    enum EAuxGeomPublicRenderflags_AlphaBlendMode
    {
        e_AlphaNone = 0x0 << e_AlphaBlendingShift,
        e_AlphaAdditive = 0x1 << e_AlphaBlendingShift,
        e_AlphaBlended = 0x2 << e_AlphaBlendingShift,
    };

      enum EAuxGeomPublicRenderflags_FillMode
    {
        e_FillModeSolid = 0x0 << e_FillModeShift,
        e_FillModeWireframe = 0x1 << e_FillModeShift,
        e_FillModePoint = 0x2 << e_FillModeShift,
    };

    enum EAuxGeomPublicRenderflags_CullMode
    {
        e_CullModeNone = 0x0 << e_CullModeShift,
        e_CullModeFront = 0x1 << e_CullModeShift,
        e_CullModeBack = 0x2 << e_CullModeShift,
    };


    enum EAuxGeomPublicRenderflags_DepthWrite
    {
        e_DepthWriteOn = 0x0 << e_DepthWriteShift,
        e_DepthWriteOff = 0x1 << e_DepthWriteShift,
    };

    enum EAuxGeomPublicRenderflags_DepthTest
    {
        e_DepthTestOn = 0x0 << e_DepthTestShift,
        e_DepthTestOff = 0x1 << e_DepthTestShift,
    };

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
            multiplayerSystemComponent->AddLevelLoadBlockedHandler(m_levelLoadBlockedHandler);
        }
    }

    void MultiplayerConnectionViewportMessageSystemComponent::Deactivate()
    {
        m_levelLoadBlockedHandler.Disconnect();
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
            const float scrimAlpha = 0.6f;
            DrawScrim(scrimAlpha);

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

        // Display the viewport toast text
        if (!m_centerViewportDebugToastText.empty())
        {
            const float center_screenposition_x = 0.5f * viewportSize.m_width;
            const float center_screenposition_y = 0.5f * viewportSize.m_height;

            // Fade out the toast over time
            const AZ::TimeMs currentTime = static_cast<AZ::TimeMs>(AZStd::GetTimeUTCMilliSecond());
            const AZ::TimeMs remainingTime = CenterViewportDebugToastTime - (currentTime - m_centerViewportDebugToastStartTime);
            const float alpha = AZStd::clamp(aznumeric_cast<float>(remainingTime) / aznumeric_cast<float>(CenterViewportDebugToastTimeFade), 0.0f, 1.0f);

            if (alpha > 0.01f)
            {
                // Draw background for text contrast
                DrawScrim(alpha);

                // Draw title
                const float textHeight = m_fontDrawInterface->GetTextSize(m_drawParams, CenterViewportToastTitle).GetY();
                const float screenposition_title_y = center_screenposition_y - textHeight * 0.5f;
                m_drawParams.m_position = AZ::Vector3(center_screenposition_x, screenposition_title_y, 1.0f);
                m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Center;
                m_drawParams.m_color = AZ::Colors::Red;
                m_drawParams.m_color.SetA(alpha);
                m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, CenterViewportToastTitle);

                // Draw toast text under the title
                // Calculate line spacing based on the font's actual line height
                m_drawParams.m_color = AZ::Colors::White;
                m_drawParams.m_color.SetA(alpha);
                m_drawParams.m_position.SetY(m_drawParams.m_position.GetY() + textHeight + m_lineSpacing);
                m_fontDrawInterface->DrawScreenAlignedText2d(m_drawParams, m_centerViewportDebugToastText.c_str());
            }
            else // Completed faded now, clear the toast.
            {
                m_centerViewportDebugToastText.clear();
            }
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

        const AZ::u32 previousState = debugDisplay->GetState();
        debugDisplay->SetState(
            e_Mode2D | e_AlphaBlended | e_FillModeSolid | e_CullModeBack | e_DepthWriteOff | static_cast<AZ::u32>(e_DepthTestOff));

        debugDisplay->DrawQuadGradient(
            AZ::Vector3(0, 0, 0),
            AZ::Vector3(1.f, 0, 0),
            AZ::Vector3(1.f, 0.5f, 0),
            AZ::Vector3(0, 0.5f, 0),
            AZ::Vector4(0, 0, 0, 0),
            AZ::Vector4(0, 0, 0, ScrimAlpha * alphaMultiplier));

        debugDisplay->DrawQuadGradient(
            AZ::Vector3(0, 0.5f, 0),
            AZ::Vector3(1.f, 0.5f, 0),
            AZ::Vector3(1.f, 1.f, 0),
            AZ::Vector3(0, 1.f, 0),
            AZ::Vector4(0, 0, 0, ScrimAlpha * alphaMultiplier),
            AZ::Vector4(0, 0, 0, 0));

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

    void MultiplayerConnectionViewportMessageSystemComponent::OnBlockedLevelLoad()
    {
        m_centerViewportDebugToastStartTime = static_cast<AZ::TimeMs>(AZStd::GetTimeUTCMilliSecond());
        m_centerViewportDebugToastText = OnBlockedLevelLoadMessage;
    }
}
