/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Font/FontInterface.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/MultiplayerEditorServerBus.h>


namespace Multiplayer
{
    //! System component that draws viewport messaging as the editor attempts connection to the editor-server while starting up game-mode.
    class MultiplayerConnectionViewportMessageSystemComponent final
        : public AZ::Component
        , public AZ::RPI::ViewportContextNotificationBus::Handler
        , MultiplayerEditorServerNotificationBus::Handler
    {
    public:
        static constexpr int MaxMessageLength = 256;
        static constexpr float ScrimAlpha = 0.6f;

        static constexpr AZ::TimeMs CenterViewportDebugToastTimePerWord = AZ::TimeMs{ 300 }; // Consider reading speed to be 200 words per minute (300 ms)
        static constexpr AZ::TimeMs CenterViewportDebugToastTimePrefix = AZ::TimeMs{ 2000 }; // Give viewers 2.0 seconds to notice the toast
        static constexpr AZ::TimeMs CenterViewportDebugToastTimeFade = AZ::TimeMs{ 1000 }; // Milliseconds toast takes to fade out

        // Messaging for client during editor play mode
        static constexpr char CenterViewportDebugTitle[] = "Multiplayer Editor";
        static constexpr char OnServerLaunchedMessage[] = "(1/4) Launching server...";
        static constexpr char OnServerLaunchFailMessage[] = "(1/4) Could not launch editor server.\nSee console for more info.";
        static constexpr char OnEditorConnectionAttemptMessage[] = "(2/4) Attempting to connect to server in order to send level data.\nAttempt %i of %i";
        static constexpr char OnEditorConnectionAttemptsFailedMessage[] = "(2/4) Failed to connect to server after %i attempts!\nPlease exit play mode and try again.";  
        static constexpr char OnEditorSendingLevelDataMessage[] = "(3/4) Editor is sending the editor-server the level data packet.\nBytes %u / %u sent.";
        static constexpr char OnEditorSendingLevelDataFailedMessage[] =
            "(3/4) Editor failed to send the editor-server the level data packet.\nPlease exit play mode and try again.";
        static constexpr char OnEditorSendingLevelDataSuccessMessage[] =
            "(4/4) Waiting for editor-server to finish loading the level data.";
        static constexpr char OnConnectToSimulationFailMessage[] =
            "EditorServerReady packet was received, but connecting to the editor-server's network simulation failed! Is the editor and "
            "server using the same sv_port (%i)?";
        static constexpr char OnEditorServerStoppedUnexpectedly[] ="Editor server has unexpectedly stopped running!";

        // Messaging for clients
        static constexpr char ClientStatusTitle[] = "Multiplayer Client Status:";

        // Messaging common for both dedicated server and client-server
        static constexpr char ServerHostingPort[] = "Hosting on port %i";

        // Messaging for dedicated server
        static constexpr char DedicatedServerStatusTitle[] = "Multiplayer Dedicated Server Status:";
        static constexpr char DedicatedServerNotHosting[] = "Not Hosting";
        static constexpr char DedicatedServerHostingClientCount[] = "%i client(s)";

        // Messaging for client-server
        static constexpr char ClientServerStatusTitle[] = "Multiplayer Client-Server Status:";
        static constexpr char ClientServerHostingClientCount[] = "%i client(s) (including self)";

        // Toast Messages
        static constexpr char CenterViewportToastTitle[] = "Multiplayer Alert";
        static constexpr char OnBlockedLevelLoadMessage[] = "Blocked level load; see log for details.";
        static constexpr char OnNoServerLevelLoadedMessageClientSide[] = "Server accept message did not provide a level.\nEnsure server has level loaded before connecting.";
        static constexpr char OnNoServerLevelLoadedMessageServerSide[] = "A client has connected, but we're not in a level.\nPlease load a valid multiplayer level before accepting clients.";
        static constexpr char OnVersionMismatch[] = "Multiplayer Version Mismatch.\nEnsure server and client are both up to date.";

        AZ_COMPONENT(MultiplayerConnectionViewportMessageSystemComponent, "{7600cfcf-e380-4876-aa90-8120e57205e9}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}


    private:
        //! AZ::RPI::ViewportContextNotificationBus::Handler overrides.
        //! @{
        void OnRenderTick() override;
        //! @}

        //! MultiplayerEditorServerNotificationBus::Handler overrides.
        //! @{
        void OnServerLaunched() override;
        void OnServerLaunchFail() override;
        void OnEditorConnectionAttempt(uint16_t connectionAttempts, uint16_t maxAttempts) override;
        void OnEditorConnectionAttemptsFailed(uint16_t failedAttempts) override;
        void OnEditorSendingLevelData(uint32_t bytesSent, uint32_t bytesTotal) override;
        void OnEditorSendingLevelDataFailed() override;
        void OnEditorSendingLevelDataSuccess() override;
        void OnConnectToSimulationSuccess() override;
        void OnConnectToSimulationFail(uint16_t serverPort) override;
        void OnPlayModeEnd() override;
        void OnEditorServerProcessStoppedUnexpectedly() override;
        //! @}

        //! IMultiplayer AZ::Event handlers.
        //! @{
        void OnBlockedLevelLoad();
        LevelLoadBlockedEvent::Handler m_levelLoadBlockedHandler = LevelLoadBlockedEvent::Handler([this](){OnBlockedLevelLoad();});

        void OnNoServerLevelLoadedEvent();
        NoServerLevelLoadedEvent::Handler m_noServerLevelLoadedHandler = NoServerLevelLoadedEvent::Handler([this](){OnNoServerLevelLoadedEvent();});

        void OnVersionMismatchEvent();
        VersionMismatchEvent::Handler m_versionMismatchEventHandler = VersionMismatchEvent::Handler([this](){OnVersionMismatchEvent();});
        //! @}

        void DrawConnectionStatus(AzNetworking::ConnectionState connectionState, const AzNetworking::IpAddress& hostAddress);
        void DrawConnectionStatusLine(const char* line, const AZ::Color& color);

        // Render a scrim (a gentle background shading) to create contrast in order that the debug text in the foreground is readable.
        // Make scrim most pronounced from the center of the screen and fade out towards the top and bottom on the screen.
        void DrawScrim(float alphaMultiplier = 1.0f) const;

        // Draws a message in the center of the viewport
        // @param title text is displayed over the message
        // @param title color is color of the title. Generally yellow or red (in the case of an error)
        // @param message to display
        // @param alpha value of the message (useful for fading out the message over time)
        void DrawCenterViewportMessage(const char* title, const AZ::Color& titleColor, const char* message, float alpha);

        AZStd::fixed_string<MaxMessageLength> m_centerViewportDebugText;
        AZ::Color m_centerViewportDebugTextColor = AZ::Colors::Yellow;

        AZStd::fixed_string<MaxMessageLength> m_centerViewportDebugToastText;
        AZ::TimeMs m_centerViewportDebugToastStartTime;

        AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
        AzFramework::TextDrawParameters m_drawParams;

        float m_lineSpacing = 0.0f;
        AzNetworking::IpAddress m_hostIpAddress;
        int m_currentConnectionsDrawCount = 0;
    };
}
