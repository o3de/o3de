/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#ifndef GEMS_MULTIPLAYER_MULTIPLAYERLOBBYCOMPONENT_H
#define GEMS_MULTIPLAYER_MULTIPLAYERLOBBYCOMPONENT_H

#include <AzCore/Component/Component.h>

#include <GridMate/GridMate.h>
#include <GridMate/Session/Session.h>

#include <GameLift/Session/GameLiftClientServiceEventsBus.h>

#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCanvasBus.h>

#include "Multiplayer/MultiplayerLobbyBus.h"
#include "Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyServiceWrapper.h"

namespace Multiplayer
{
    class MultiplayerLobbyServiceWrapper;
    class MultiplayerDedicatedHostTypeSelectionCanvas;
    class MultiplayerGameLiftLobbyCanvas;
    class MultiplayerLANGameLobbyCanvas;
    class MultiplayerBusyAndErrorCanvas;

    class MultiplayerLobbyComponent
        : public AZ::Component
        , public GridMate::SessionEventBus::Handler
        , public Multiplayer::MultiplayerLobbyBus::Handler
#if defined(BUILD_GAMELIFT_CLIENT)
        , public GridMate::GameLiftClientServiceEventsBus::Handler
#endif
    {
    public:
        AZ_COMPONENT(MultiplayerLobbyComponent,"{916E8722-7CCF-4FBA-B2B2-81A7407B2272}");
        static void Reflect(AZ::ReflectContext* reflectContext);

        MultiplayerLobbyComponent();
        virtual ~MultiplayerLobbyComponent();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // SessionEventBus
        void OnSessionCreated(GridMate::GridSession* session) override;
        void OnSessionError(GridMate::GridSession* session, const GridMate::string& errorMsg) override;
        void OnGridSearchComplete(GridMate::GridSearch* search) override;

        // MultiplayerLobbyBus
        int GetGamePort() const;
        void ConfigureSessionParams(GridMate::SessionParams& sessionParams);

    protected:
        enum class LobbyMode
        {
            Unknown,
            LobbySelection, // Provides selection to enter LAN/GAMELIFT
            GameliftLobby, // GameLift lobby
            ServiceWrapperLobby // LAN lobby
        };

        // MultiplayerLobbyComponent
        void ShowSelectionLobby();
        void ShowLobby(LobbyMode lobbyType);
        void HideLobby();

        virtual bool StartSessionService(LobbyMode lobbyType);
        virtual void StopSessionService();

        void CreateServer();
        void ListServers();
        void ClearSearches();

        void JoinServer();
        bool JoinSession(const GridMate::SearchInfo* searchInfo);

        virtual bool SanityCheck();
        virtual bool SanityCheckGameLift();

        void SelectLANServerType();
        void SelectGameLiftServerType();
        void StartGameLiftMatchmaking();

        MultiplayerLANGameLobbyCanvas* m_lanGameLobbyCanvas;
        MultiplayerGameLiftLobbyCanvas* m_gameLiftLobbyCanvas;
        MultiplayerBusyAndErrorCanvas* m_busyAndErrorCanvas;
        MultiplayerDedicatedHostTypeSelectionCanvas* m_dedicatedHostTypeSelectionCanvas;
        // Wrapped Session Service
        MultiplayerLobbyServiceWrapper* m_multiplayerLobbyServiceWrapper;

        // Shared Configuration
        GridMate::GridSearch* m_listSearch;

        // Gamelift Configuration
        GridMate::GridSearch* m_gameliftCreationSearch;
    private:

        // ServiceWrapperLobby Functions
        bool SanityCheckWrappedSessionService();
        void CreateServerForWrappedService();
        void ListServersForWrappedService();

        bool ValidateGameLiftConfig();

#if defined(BUILD_GAMELIFT_CLIENT)
        bool StartGameLiftSession();
        void StopGameLiftSession();
        void CreateServerForGameLift();
        void ListServersForGameLift();

        // GridMate::GameLiftSessionServiceEventsBus::Handler
        void OnGameLiftSessionServiceReady(GridMate::GameLiftClientService*) override;
        void OnGameLiftSessionServiceFailed(GridMate::GameLiftClientService*, const AZStd::string& message) override;

#endif

        void ShowError(const char* error);
        void DismissError(bool force = false);
        void ShowBusyScreen();
        void DismissBusyScreen(bool force = false);

        LyShine::StringType GetMapName() const;
        LyShine::StringType GetServerName() const;

        // External Configuration
        int m_maxPlayers;
        int m_port;
        bool m_enableDisconnectDetection;
        unsigned int m_connectionTimeoutMS;

        AZStd::string m_defaultMap;
        AZStd::string m_defaultServerName;
        AZStd::string m_defaultMatchmakingConfig;

        bool m_unregisterGameliftServiceOnErrorDismiss;
        bool m_hasGameliftSession;
        LobbyMode m_lobbyMode;
    };
}

#endif
