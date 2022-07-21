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
        static constexpr char CenterViewportDebugTitle[] = "Multiplayer Editor";
        static constexpr char ClientStatusTitle[] = "Multiplayer Client Status:";
        static constexpr char OnServerLaunchedMessage[] = "(1/3) Launching server...";
        static constexpr char OnServerLaunchFailMessage[] = "(1/3) Could not launch editor server.\nSee console for more info.";
        static constexpr char OnEditorConnectionAttemptMessage[] = "(2/3) Editor tcp connection attempt #%i.";
        static constexpr char OnEditorSendingLevelDataMessage[] = "(3/3) Editor is sending the editor-server the level data packet.";
        static constexpr char OnConnectToSimulationFailMessage[] = "EditorServerReady packet was received, but connecting to the editor-server's network simulation failed! Is the editor and server using the same sv_port (%i)?";

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
        void OnEditorConnectionAttempt(uint16_t connectionAttempts) override;
        void OnEditorSendingLevelData() override;
        void OnConnectToSimulationSuccess() override;
        void OnConnectToSimulationFail(uint16_t serverPort) override;
        void OnPlayModeEnd() override;
        //! @}

        void DrawConnectionStatus(AzNetworking::ConnectionState connectionState);

        AZStd::fixed_string<128> m_centerViewportDebugText;
        AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
        AzFramework::TextDrawParameters m_drawParams;
        float m_lineSpacing = 0.0f;
    };
}
