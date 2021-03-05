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

#include "Multiplayer_precompiled.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

#include <GameLift/GameLiftBus.h>
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftSessionDefs.h>
#include <GameLift/Session/GameLiftSessionRequest.h>

#include <GridMate/Carrier/Driver.h>
#include <GridMate/NetworkGridMate.h>

#include <LyShine/Bus/UiCursorBus.h>

#include "Multiplayer/IMultiplayerGem.h"
#include "Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyLANServiceWrapper.h"
#include "Source/Canvas/MultiplayerDedicatedHostTypeSelectionCanvas.h"
#include "Source/Canvas/MultiplayerGameLiftLobbyCanvas.h"
#include "Source/Canvas/MultiplayerLANGameLobbyCanvas.h"
#include "Source/Canvas/MultiplayerBusyAndErrorCanvas.h"

#include "Multiplayer/MultiplayerLobbyComponent.h"

#include <Multiplayer_Traits_Platform.h>

#include "Multiplayer/MultiplayerUtils.h"

#include <Source/Canvas/MultiplayerCanvasHelper.h>

namespace Platform
{
    bool ListServers(const AZStd::string& actionName, const AZ::EntityId& entityId, Multiplayer::MultiplayerLobbyServiceWrapper*& multiplayerLobbyServiceWrapper);
}

namespace Multiplayer
{
    void MultiplayerLobbyComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serialize)
        {
            serialize->Class<MultiplayerLobbyComponent,AZ::Component>()
                ->Version(1)
                ->Field("MaxPlayers",&MultiplayerLobbyComponent::m_maxPlayers)
                ->Field("Port",&MultiplayerLobbyComponent::m_port)
                ->Field("EnableDisconnectDetection",&MultiplayerLobbyComponent::m_enableDisconnectDetection)
                ->Field("ConnectionTimeout",&MultiplayerLobbyComponent::m_connectionTimeoutMS)
                ->Field("DefaultMap",&MultiplayerLobbyComponent::m_defaultMap)
                ->Field("DefaultServer",&MultiplayerLobbyComponent::m_defaultServerName)
                ->Field("DefaultMatchmakingConfig",&MultiplayerLobbyComponent::m_defaultMatchmakingConfig)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();

            if (editContext)
            {
                editContext->Class<MultiplayerLobbyComponent>("Multiplayer Lobby Component","This component will load up and manage a simple lobby for connecting for LAN and GameLift sessions.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData,"")
                        ->Attribute(AZ::Edit::Attributes::Category,"MultiplayerSample")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand,true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_maxPlayers,"Max Players","The total number of players that can join in the game.")
                        ->Attribute(AZ::Edit::Attributes::Min,0)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_port,"Port","The port on which the game service will create connections through.")
                        ->Attribute(AZ::Edit::Attributes::Min,1)
                        ->Attribute(AZ::Edit::Attributes::Max,65534)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_enableDisconnectDetection,"Enable Disconnect Detection","Enables disconnecting players if they do not respond within the Timeout window.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_connectionTimeoutMS,"Timeout","The length of time a client has to respond before being disconnected(if disconnection detection is enabled.")
                        ->Attribute(AZ::Edit::Attributes::Suffix,"ms")
                        ->Attribute(AZ::Edit::Attributes::Min,0)
                        ->Attribute(AZ::Edit::Attributes::Max,60000)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_defaultMap,"DefaultMap", "The default value that will be added to the map field when loading the lobby.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_defaultServerName,"DefaultServerName","The default value that will be added to the server name field when loading the lobby.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_defaultMatchmakingConfig,"DefaultMatchmaking","The default value that will be used for matchmaking in the GameLift lobby.")
                ;
            }
        }
    }

    MultiplayerLobbyComponent::MultiplayerLobbyComponent()
        : m_maxPlayers(8)
        , m_port(SERVER_DEFAULT_PORT)
        , m_enableDisconnectDetection(true)
        , m_connectionTimeoutMS(500)
        , m_defaultMap("")
        , m_defaultServerName("MyServer")
        , m_defaultMatchmakingConfig("MyConfig")
        , m_unregisterGameliftServiceOnErrorDismiss(false)
        , m_hasGameliftSession(false)
        , m_lobbyMode(LobbyMode::Unknown)
        , m_listSearch(nullptr)
        , m_multiplayerLobbyServiceWrapper(nullptr)
        , m_gameliftCreationSearch(nullptr)
    {
    }

    MultiplayerLobbyComponent::~MultiplayerLobbyComponent()
    {
    }

    void MultiplayerLobbyComponent::Activate()
    {
        Multiplayer::MultiplayerLobbyBus::Handler::BusConnect(GetEntityId());

        MultiplayerDedicatedHostTypeSelectionCanvasContext dedicatedHostTypeSelectionCanvasContext;
        dedicatedHostTypeSelectionCanvasContext.OnLANButtonClicked = std::bind(&MultiplayerLobbyComponent::SelectLANServerType, this);
        dedicatedHostTypeSelectionCanvasContext.OnGameLiftConnectButtonClicked = std::bind(&MultiplayerLobbyComponent::SelectGameLiftServerType, this);
        m_dedicatedHostTypeSelectionCanvas = aznew MultiplayerDedicatedHostTypeSelectionCanvas(dedicatedHostTypeSelectionCanvasContext);

        MultiplayerLANGameLobbyCanvasContext lanGameLobbyCanvasContext;
        lanGameLobbyCanvasContext.CreateServerViewContext.OnCreateServerButtonClicked = std::bind(&MultiplayerLobbyComponent::CreateServer, this);
        lanGameLobbyCanvasContext.OnReturnButtonClicked = std::bind(&MultiplayerLobbyComponent::ShowSelectionLobby, this);
        lanGameLobbyCanvasContext.JoinServerViewContext.OnJoinButtonClicked = std::bind(&MultiplayerLobbyComponent::JoinServer, this);
        lanGameLobbyCanvasContext.JoinServerViewContext.OnRefreshButtonClicked = std::bind(&MultiplayerLobbyComponent::ListServers, this);
        lanGameLobbyCanvasContext.CreateServerViewContext.DefaultMapName = m_defaultMap;
        lanGameLobbyCanvasContext.CreateServerViewContext.DefaultServerName = m_defaultServerName;
        m_lanGameLobbyCanvas = aznew MultiplayerLANGameLobbyCanvas(lanGameLobbyCanvasContext);
        m_lanGameLobbyCanvas->Hide();

        MultiplayerGameLiftLobbyCanvasContext gameLiftLobbyCanvasContext;
        gameLiftLobbyCanvasContext.CreateServerViewContext.OnCreateServerButtonClicked = std::bind(&MultiplayerLobbyComponent::CreateServer, this);
        gameLiftLobbyCanvasContext.OnReturnButtonClicked = std::bind(&MultiplayerLobbyComponent::ShowSelectionLobby, this);
        gameLiftLobbyCanvasContext.JoinServerViewContext.OnJoinButtonClicked = std::bind(&MultiplayerLobbyComponent::JoinServer, this);
        gameLiftLobbyCanvasContext.JoinServerViewContext.OnRefreshButtonClicked = std::bind(&MultiplayerLobbyComponent::ListServers, this);

 #if defined(BUILD_GAMELIFT_CLIENT)
        gameLiftLobbyCanvasContext.GameLiftFlexMatchViewContext.OnStartMatchmakingButtonClicked = std::bind(&MultiplayerLobbyComponent::StartGameLiftMatchmaking, this);
#endif

        gameLiftLobbyCanvasContext.CreateServerViewContext.DefaultMapName = m_defaultMap;
        gameLiftLobbyCanvasContext.CreateServerViewContext.DefaultServerName = m_defaultServerName;
        gameLiftLobbyCanvasContext.GameLiftFlexMatchViewContext.DefaultMatchmakingConfig = m_defaultMatchmakingConfig;
        m_gameLiftLobbyCanvas = aznew MultiplayerGameLiftLobbyCanvas(gameLiftLobbyCanvasContext);
        m_gameLiftLobbyCanvas->Hide();

        MultiplayerBusyAndErrorCanvasContext busyAndErrorCanvasContext;
        busyAndErrorCanvasContext.OnDismissErrroWindowButtonClicked = std::bind(&MultiplayerLobbyComponent::DismissError, this, false);
        m_busyAndErrorCanvas = aznew MultiplayerBusyAndErrorCanvas(busyAndErrorCanvasContext);

        ShowSelectionLobby();

        UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);

        if (gEnv->pNetwork)
        {
            GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

            if (gridMate)
            {
                GridMate::SessionEventBus::Handler::BusConnect(gridMate);
            }
        }
        AZ_TRAIT_MULTIPLAYER_LOBBY_SERVICE_ASSIGN_DEFAULT_PORT(AZ_TRAIT_MULTIPLAYER_LOBBY_SERVICE_ASSIGN_DEFAULT_PORT_VALUE);
    }

    void MultiplayerLobbyComponent::Deactivate()
    {
        GridMate::SessionEventBus::Handler::BusDisconnect();

        delete m_dedicatedHostTypeSelectionCanvas;
        m_dedicatedHostTypeSelectionCanvas = nullptr;
        delete m_lanGameLobbyCanvas;
        m_lanGameLobbyCanvas = nullptr;
        delete m_gameLiftLobbyCanvas;
        m_gameLiftLobbyCanvas = nullptr;

        UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);

        ClearSearches();

        delete m_multiplayerLobbyServiceWrapper;
        m_multiplayerLobbyServiceWrapper = nullptr;
    }

    void MultiplayerLobbyComponent::SelectLANServerType()
    {
        if (m_multiplayerLobbyServiceWrapper)
        {
            delete m_multiplayerLobbyServiceWrapper;
        }

        m_multiplayerLobbyServiceWrapper = aznew MultiplayerLobbyLANServiceWrapper(GetEntityId());

        ShowLobby(LobbyMode::ServiceWrapperLobby);
    }

    void MultiplayerLobbyComponent::SelectGameLiftServerType()
    {
#if defined(BUILD_GAMELIFT_CLIENT)
        ShowLobby(LobbyMode::GameliftLobby);
#else
        AZ_Assert(false, "Trying to use GameLift on unsupported Platform.");
#endif
    }

    void MultiplayerLobbyComponent::OnSessionCreated(GridMate::GridSession* session)
    {
        GridMate::GridSession* gridSession = nullptr;
        EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);

        if (gridSession == session && session->IsHost())
        {
            Multiplayer::Utils::SynchronizeSessionState(session);
        }
    }

    void MultiplayerLobbyComponent::OnSessionError([[maybe_unused]] GridMate::GridSession* session,const GridMate::string& errorMsg)
    {
        ShowError(errorMsg.c_str());
    }

    void MultiplayerLobbyComponent::OnGridSearchComplete(GridMate::GridSearch* search)
    {
        if (search == m_gameliftCreationSearch)
        {
            DismissBusyScreen();

            GridMate::Network* network = static_cast<GridMate::Network*>(gEnv->pNetwork);

            if (network)
            {
                if (search->GetNumResults() == 0)
                {
                    ShowError("Error creating GameLift Session");
                }
                else
                {
                    const GridMate::SearchInfo* searchInfo = m_gameliftCreationSearch->GetResult(0);
                    JoinSession(searchInfo);
                }
            }

            m_gameliftCreationSearch->Release();
            m_gameliftCreationSearch = nullptr;
        }
        else if (search == m_listSearch)
        {
            if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
            {
                m_lanGameLobbyCanvas->DisplaySearchResults(m_listSearch);
            }
            else if(m_lobbyMode == LobbyMode::GameliftLobby)
            {
                m_gameLiftLobbyCanvas->DisplaySearchResults(m_listSearch);
            }
            DismissBusyScreen();
        }
    }

    int MultiplayerLobbyComponent::GetGamePort() const
    {
        return m_port;
    }

    void MultiplayerLobbyComponent::ConfigureSessionParams(GridMate::SessionParams& sessionParams)
    {
        sessionParams.m_topology = GridMate::ST_CLIENT_SERVER;
        sessionParams.m_numPublicSlots = m_maxPlayers + (gEnv->IsDedicated() ? 1 : 0); // One slot for server member.
        sessionParams.m_numPrivateSlots = 0;
        sessionParams.m_peerToPeerTimeout = 60000;
        sessionParams.m_flags = 0;

        sessionParams.m_numParams = 0;
        sessionParams.m_params[sessionParams.m_numParams].m_id = "sv_name";
        sessionParams.m_params[sessionParams.m_numParams].SetValue(GetServerName().c_str());
        sessionParams.m_numParams++;

        sessionParams.m_params[sessionParams.m_numParams].m_id = "sv_map";
        sessionParams.m_params[sessionParams.m_numParams].SetValue(GetMapName().c_str());
        sessionParams.m_numParams++;
    }

    void MultiplayerLobbyComponent::ShowSelectionLobby()
    {
        const bool forceHide = true;

        if (m_lobbyMode != LobbyMode::LobbySelection)
        {
            ClearSearches();
            StopSessionService();

            if (m_multiplayerLobbyServiceWrapper)
            {
                delete m_multiplayerLobbyServiceWrapper;
                m_multiplayerLobbyServiceWrapper = nullptr;
            }

            HideLobby();
            m_lobbyMode = LobbyMode::LobbySelection;
            m_dedicatedHostTypeSelectionCanvas->Show();

            DismissError(forceHide);
            DismissBusyScreen(forceHide);
        }
    }

    void MultiplayerLobbyComponent::ShowLobby(LobbyMode lobbyMode)
    {
        if (lobbyMode == LobbyMode::LobbySelection)
        {
            ShowSelectionLobby();
        }
        else if (m_lobbyMode == LobbyMode::LobbySelection)
        {
            bool showLobby = StartSessionService(lobbyMode);

            if (showLobby)
            {
                HideLobby();
                m_lobbyMode = lobbyMode;

                if (lobbyMode == LobbyMode::ServiceWrapperLobby)
                {
                    m_lanGameLobbyCanvas->ClearSearchResults();
                    m_lanGameLobbyCanvas->Show();
                }
                else if (lobbyMode == LobbyMode::GameliftLobby)
                {
                    m_gameLiftLobbyCanvas->ClearSearchResults();
                    m_gameLiftLobbyCanvas->Show();
                }

                const bool forceHide = true;
                DismissError(forceHide);
                DismissBusyScreen(forceHide);
            }
        }
    }

    void MultiplayerLobbyComponent::HideLobby()
    {
        switch (m_lobbyMode)
        {
        case LobbyMode::ServiceWrapperLobby:
            m_lanGameLobbyCanvas->Hide();
            break;

        case LobbyMode::GameliftLobby:
            m_gameLiftLobbyCanvas->Hide();
            break;

        case LobbyMode::LobbySelection:
            m_dedicatedHostTypeSelectionCanvas->Hide();
            break;
        default:
            break;
        }
        m_lobbyMode = LobbyMode::Unknown;
    }

    bool MultiplayerLobbyComponent::StartSessionService(LobbyMode lobbyMode)
    {
        bool startedService = false;

        if (lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            if (gEnv->pNetwork && gEnv->pNetwork->GetGridMate())
            {
                if (SanityCheckWrappedSessionService())
                {
                    startedService = m_multiplayerLobbyServiceWrapper->StartSessionService(gEnv->pNetwork->GetGridMate());
                }
            }
        }
        else if (lobbyMode == LobbyMode::GameliftLobby)
        {
#if defined(BUILD_GAMELIFT_CLIENT)
            startedService = StartGameLiftSession();
#else
            startedService = false;
#endif
        }

        return startedService;
    }

    void MultiplayerLobbyComponent::StopSessionService()
    {
        // Stop whatever session we may have been using, if any
        if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            if (gEnv->pNetwork && gEnv->pNetwork->GetGridMate())
            {
                if (SanityCheckWrappedSessionService())
                {
                    m_multiplayerLobbyServiceWrapper->StopSessionService(gEnv->pNetwork->GetGridMate());
                }
            }
        }
        else if (m_lobbyMode == LobbyMode::GameliftLobby)
        {
#if defined(BUILD_GAMELIFT_CLIENT)
            StopGameLiftSession();
            m_hasGameliftSession = false;
#else
            AZ_Assert(false,"Trying to use Gamelift on Unsupported platform.");
#endif
        }
    }

    void MultiplayerLobbyComponent::CreateServer()
    {
        if (m_lobbyMode == LobbyMode::LobbySelection)
        {
            return;
        }
        else if (SanityCheck())
        {
            if (GetMapName().empty())
            {
                ShowError("Invalid Map Name");
            }
            else if (GetServerName().empty())
            {
                ShowError("Invalid Server Name");
            }
            else
            {
                bool netSecEnabled = false;
                EBUS_EVENT_RESULT(netSecEnabled, Multiplayer::MultiplayerRequestBus,IsNetSecEnabled);

                if (netSecEnabled)
                {
                    if (!NetSec::CanCreateSecureSocketForHosting())
                    {
                        ShowError("Invalid Secure Socket configuration given for hosting a session.\nEnsure that a Public and Private key are being supplied.");
                        return;
                    }
                }

                if (m_lobbyMode == LobbyMode::GameliftLobby)
                {
#if defined(BUILD_GAMELIFT_CLIENT)
                    if (SanityCheckGameLift())
                    {
                        CreateServerForGameLift();
                    }
#else
                    AZ_Assert(false,"Trying to use Gamelift on unsupported platform.");
#endif
                }
                else if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
                {
                    if (SanityCheckWrappedSessionService())
                    {
                        CreateServerForWrappedService();
                    }
                }
            }
        }
    }

    void MultiplayerLobbyComponent::ListServers()
    {
        if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            m_lanGameLobbyCanvas->ClearSearchResults();
        }
        else if (m_lobbyMode == LobbyMode::GameliftLobby)
        {
            m_gameLiftLobbyCanvas->ClearSearchResults();
        }

        if (m_lobbyMode == LobbyMode::GameliftLobby)
        {
#if defined(BUILD_GAMELIFT_CLIENT)
            if (SanityCheckGameLift())
            {
                ListServersForGameLift();
            }
#else
            AZ_Assert(false,"Trying to use Gamelift lobby on unsupported platform.")
#endif
        }
        else if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            ListServersForWrappedService();
        }
    }

    void MultiplayerLobbyComponent::ClearSearches()
    {
        if (m_listSearch)
        {
            if (!m_listSearch->IsDone())
            {
                m_listSearch->AbortSearch();
            }

            m_listSearch->Release();
            m_listSearch = nullptr;
        }

        if (m_gameliftCreationSearch)
        {
            if (!m_gameliftCreationSearch->IsDone())
            {
                m_gameliftCreationSearch->AbortSearch();
            }

            m_gameliftCreationSearch->Release();
            m_gameliftCreationSearch = nullptr;
        }
    }

    void MultiplayerLobbyComponent::JoinServer()
    {
        if (m_lobbyMode == LobbyMode::LobbySelection)
        {
            return;
        }

        int selectedServerResult = -1;
        if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            selectedServerResult = m_lanGameLobbyCanvas->GetSelectedServerResult();
        }
        else if (m_lobbyMode == LobbyMode::GameliftLobby)
        {
            selectedServerResult = m_gameLiftLobbyCanvas->GetSelectedServerResult();
        }
        if (   m_listSearch == nullptr
            || !m_listSearch->IsDone()
            || selectedServerResult < 0
            || m_listSearch->GetNumResults() <= selectedServerResult)
        {
            ShowError("No Server Selected to Join.");
            return;
        }

        const GridMate::SearchInfo* searchInfo = m_listSearch->GetResult(selectedServerResult);

        if (!SanityCheck())
        {
            return;
        }
        else if (searchInfo == nullptr)
        {
            ShowError("Invalid Server Selection.");
            return;
        }
        else
        {
            bool netSecEnabled = false;
            EBUS_EVENT_RESULT(netSecEnabled, Multiplayer::MultiplayerRequestBus,IsNetSecEnabled);

            if (netSecEnabled)
            {
                if (!NetSec::CanCreateSecureSocketForJoining())
                {
                    ShowError("Invalid Secure Socket configuration given for joining an encrypted session.\nEnsure that a Certificate Authority is being supplied.");
                    return;
                }
            }

            if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
            {
                if (!SanityCheckWrappedSessionService())
                {
                    return;
                }
            }
            else if (m_lobbyMode == LobbyMode::GameliftLobby)
            {
#if defined(BUILD_GAMELIFT_CLIENT)
                if (!SanityCheckGameLift())
                {
                    return;
                }
#else
                AZ_Assert(false,"Trying to use Gamelift lobby on unsupported platform.")
                return;
#endif
            }
        }

        ShowBusyScreen();

        if (!JoinSession(searchInfo))
        {
            ShowError("Found a game session, but failed to join.");
        }
    }

    bool MultiplayerLobbyComponent::JoinSession(const GridMate::SearchInfo* searchInfo)
    {
        GridMate::GridSession* session = nullptr;
        GridMate::CarrierDesc carrierDesc;

        Multiplayer::Utils::InitCarrierDesc(carrierDesc);
        Multiplayer::NetSec::ConfigureCarrierDescForJoin(carrierDesc);

        GridMate::JoinParams joinParams;

        if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            if (SanityCheckWrappedSessionService())
            {
                session = m_multiplayerLobbyServiceWrapper->JoinSession(gEnv->pNetwork->GetGridMate(),carrierDesc,searchInfo);
            }
        }
        else if (m_lobbyMode == LobbyMode::GameliftLobby)
        {
#if defined(BUILD_GAMELIFT_CLIENT)
            const GridMate::GameLiftSearchInfo& gameliftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo&>(*searchInfo);
            EBUS_EVENT_ID_RESULT(session, gEnv->pNetwork->GetGridMate(), GridMate::GameLiftClientServiceBus, JoinSessionBySearchInfo, gameliftSearchInfo, carrierDesc);
#endif
        }

        if (session != nullptr)
        {
            EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSession,session);
        }
        else
        {
            Multiplayer::NetSec::OnSessionFailedToCreate(carrierDesc);
        }

        return session != nullptr;
    }

    bool MultiplayerLobbyComponent::SanityCheck()
    {
        if (gEnv->IsEditor())
        {
            ShowError("Unsupported action inside of Editor.");
            return false;
        }
        else if (gEnv->pNetwork == nullptr)
        {
            ShowError("Network Environment is null");
            return false;
        }
        else
        {
            GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

            if (gridMate == nullptr)
            {
                ShowError("GridMate is null.");
                return false;
            }
        }

        return true;
    }

    bool MultiplayerLobbyComponent::SanityCheckWrappedSessionService()
    {
        return m_multiplayerLobbyServiceWrapper != nullptr;
    }

    void MultiplayerLobbyComponent::CreateServerForWrappedService()
    {
        GridMate::GridSession* gridSession = nullptr;
        EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);

        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        if (!gridSession && SanityCheckWrappedSessionService())
        {
            GridMate::CarrierDesc carrierDesc;

            Multiplayer::Utils::InitCarrierDesc(carrierDesc);
            Multiplayer::NetSec::ConfigureCarrierDescForHost(carrierDesc);

            carrierDesc.m_port = m_port;
            carrierDesc.m_enableDisconnectDetection = m_enableDisconnectDetection;
            carrierDesc.m_connectionTimeoutMS = m_connectionTimeoutMS;
            carrierDesc.m_threadUpdateTimeMS = 30;

            ShowBusyScreen();

            GridMate::GridSession* session = m_multiplayerLobbyServiceWrapper->CreateServer(gridMate,carrierDesc);

            if (session == nullptr)
            {
                Multiplayer::NetSec::OnSessionFailedToCreate(carrierDesc);
                ShowError("Error while hosting Session.");
            }
            else
            {
                EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSession,session);
            }
        }
        else
        {
            ShowError("Invalid Gem Session");
        }
    }

    void MultiplayerLobbyComponent::ListServersForWrappedService()
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        if (gridMate && SanityCheck() && SanityCheckWrappedSessionService())
        {
            ShowBusyScreen();

            if (m_listSearch)
            {
                m_listSearch->AbortSearch();
                m_listSearch->Release();
                m_listSearch = nullptr;
            }

            m_listSearch = m_multiplayerLobbyServiceWrapper->ListServers(gridMate);

            if (m_listSearch == nullptr)
            {
                ShowError("ListServers failed to start a GridSearch.");
            }
        }
        else
        {
            ShowError("Missing Online Service.");
        }
    }

    bool MultiplayerLobbyComponent::SanityCheckGameLift()
    {
#if defined(BUILD_GAMELIFT_CLIENT)

        if (!ValidateGameLiftConfig())
        {
            return false;
        }

        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        // This should already be errored by the previous sanity check.
        if (gridMate == nullptr)
        {
            return false;
        }
        else if (!GridMate::HasGridMateService<GridMate::GameLiftClientService>(gridMate))
        {
            ShowError("MultiplayerService is missing.");
            return false;
        }

        return true;
#else
        return false;
#endif
    }

    bool MultiplayerLobbyComponent::ValidateGameLiftConfig()
    {
        const AZStd::string fleetId = GetConsoleVarValue("gamelift_fleet_id");
        const AZStd::string aliasId = GetConsoleVarValue("gamelift_alias_id");
        const AZStd::string queueName = GetConsoleVarValue("gamelift_queue_name");

        //Validation on inputs.
        //Service still supports the use of developers credentials on top of the player credentials.
        if (fleetId.empty() && aliasId.empty() && queueName.empty())
        {
            AZ_TracePrintf("GameLift", "You need to provide at least [gamelift_aliasid, gamelift_aws_access_key, gamelift_aws_secret_key] or [gamelift_fleetid, gamelift_aws_access_key, gamelift_aws_secret_key] or [gamelift_queue_name, gamelift_aws_access_key, gamelift_aws_secret_key]\n");
            return false;
        }

        if (!fleetId.empty())
        {
            if (!aliasId.empty() || !queueName.empty())
            {
                AZ_TracePrintf("GameLift", "Initialize failed. Cannot use fleet id with aliasId/queueName.\n");
                return false;
            }
        }

        if (!aliasId.empty()) {
            if (!fleetId.empty() || !queueName.empty())
            {
                AZ_TracePrintf("GameLift", "Initialize failed. Cannot use alias id with fleetId/queueName.\n");
                return false;
            }
        }

        //If the using queues.
        if (!queueName.empty())
        {
            if (!fleetId.empty() || !aliasId.empty())
            {
                AZ_TracePrintf("GameLift", "Initialize failed. Cannot use queue name with fleetId/aliasId.\n");
                return false;
            }
        }

        return true;
    }

#if defined(BUILD_GAMELIFT_CLIENT)

    bool MultiplayerLobbyComponent::StartGameLiftSession()
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        // Not sure what happens if we start this once and it fails to be created...
        // calling it again causes an assert.
        if (gridMate && !m_hasGameliftSession)
        {
            ShowBusyScreen();

            GridMate::GameLiftClientServiceEventsBus::Handler::BusConnect(gridMate);

            GridMate::GameLiftClientServiceDesc serviceDesc;
            serviceDesc.m_accessKey = GetConsoleVarValue("gamelift_aws_access_key");
            serviceDesc.m_secretKey = GetConsoleVarValue("gamelift_aws_secret_key");
            serviceDesc.m_endpoint = GetConsoleVarValue("gamelift_endpoint");
            serviceDesc.m_region = GetConsoleVarValue("gamelift_aws_region");
            serviceDesc.m_playerId = GetConsoleVarValue("gamelift_player_id");
            serviceDesc.m_useGameLiftLocalServer = GetGetConsoleVarBoolValue("gamelift_uselocalserver");

            EBUS_EVENT(GameLift::GameLiftRequestBus, StartClientService, serviceDesc);
        }

        return m_hasGameliftSession;
    }

    void MultiplayerLobbyComponent::StopGameLiftSession()
    {
        EBUS_EVENT(GameLift::GameLiftRequestBus, StopClientService);
    }

    void MultiplayerLobbyComponent::StartGameLiftMatchmaking()
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        if (m_gameliftCreationSearch)
        {
            m_gameliftCreationSearch->AbortSearch();
            m_gameliftCreationSearch->Release();
            m_gameliftCreationSearch = nullptr;
        }

        ShowBusyScreen();

        EBUS_EVENT_ID_RESULT(m_gameliftCreationSearch, gridMate, GridMate::GameLiftClientServiceBus, StartMatchmaking, GetConsoleVarValue("gamelift_matchmaking_config_name"));
    }

    void MultiplayerLobbyComponent::CreateServerForGameLift()
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        if (m_gameliftCreationSearch)
        {
            m_gameliftCreationSearch->AbortSearch();
            m_gameliftCreationSearch->Release();
            m_gameliftCreationSearch = nullptr;
        }

        GridMate::GameLiftSessionRequestParams reqParams;
        ConfigureSessionParams(reqParams);
        reqParams.m_instanceName = m_gameLiftLobbyCanvas->GetServerName().c_str();
        reqParams.m_fleetId = GetConsoleVarValue("gamelift_fleet_id");
        reqParams.m_queueName = GetConsoleVarValue("gamelift_queue_name");
        reqParams.m_aliasId = GetConsoleVarValue("gamelift_alias_id");
        reqParams.m_useFleetId = !reqParams.m_fleetId.empty();
        ShowBusyScreen();

        EBUS_EVENT_ID_RESULT(m_gameliftCreationSearch, gridMate, GridMate::GameLiftClientServiceBus, RequestSession, reqParams);

        if (m_gameliftCreationSearch == nullptr)
        {
            ShowError("Failed to create Server for GameLift");
        }
    }

    void MultiplayerLobbyComponent::ListServersForGameLift()
    {
        ShowBusyScreen();
        GridMate::Network* network = static_cast<GridMate::Network*>(gEnv->pNetwork);
        GridMate::IGridMate* gridMate = network->GetGridMate();

        if (m_listSearch)
        {
            m_listSearch->AbortSearch();
            m_listSearch->Release();
            m_listSearch = nullptr;
        }
        GridMate::GameLiftSearchParams searchParams;
        searchParams.m_fleetId = GetConsoleVarValue("gamelift_fleet_id");
        searchParams.m_queueName = GetConsoleVarValue("gamelift_queue_name");
        searchParams.m_aliasId = GetConsoleVarValue("gamelift_alias_id");
        searchParams.m_useFleetId = !searchParams.m_fleetId.empty();
        EBUS_EVENT_ID_RESULT(m_listSearch,gridMate,GridMate::GameLiftClientServiceBus, StartSearch, searchParams);

        if (m_listSearch == nullptr)
        {
            ShowError("Failed to start a GridSearch");
        }
    }

    void MultiplayerLobbyComponent::OnGameLiftSessionServiceReady(GridMate::GameLiftClientService* service)
    {
        AZ_UNUSED(service);
        DismissBusyScreen();

        m_hasGameliftSession = true;
        ShowLobby(LobbyMode::GameliftLobby);
    }

    void MultiplayerLobbyComponent::OnGameLiftSessionServiceFailed(GridMate::GameLiftClientService* service, const AZStd::string& message)
    {
        AZ_UNUSED(service);

        DismissBusyScreen();

        m_hasGameliftSession = false;

        m_unregisterGameliftServiceOnErrorDismiss = true;

        AZStd::string errorMessage("GameLift Error: ");
        errorMessage += message;
        ShowError(errorMessage.c_str());
    }

#endif

    void MultiplayerLobbyComponent::ShowError(const char* error)
    {
        m_busyAndErrorCanvas->ShowError(error);
    }

    void MultiplayerLobbyComponent::DismissError([[maybe_unused]] bool force)
    {
        m_busyAndErrorCanvas->DismissError();

        if (m_unregisterGameliftServiceOnErrorDismiss)
        {
            m_unregisterGameliftServiceOnErrorDismiss = false;

#if defined(BUILD_GAMELIFT_CLIENT)
            StopGameLiftSession();
#endif
        }
    }

    void MultiplayerLobbyComponent::ShowBusyScreen()
    {
        m_busyAndErrorCanvas->ShowBusyScreen();
    }

    void MultiplayerLobbyComponent::DismissBusyScreen(bool force)
    {
        m_busyAndErrorCanvas->DismissBusyScreen(force);
    }

    LyShine::StringType MultiplayerLobbyComponent::GetMapName() const
    {
        switch (m_lobbyMode)
        {
        case LobbyMode::ServiceWrapperLobby:
            return m_lanGameLobbyCanvas->GetMapName();

        case LobbyMode::GameliftLobby:
            return m_gameLiftLobbyCanvas->GetMapName();

        default:
            return "";

        }
    }

    LyShine::StringType MultiplayerLobbyComponent::GetServerName() const
    {
        switch (m_lobbyMode)
        {
        case LobbyMode::ServiceWrapperLobby:
            return m_lanGameLobbyCanvas->GetServerName();

        case LobbyMode::GameliftLobby:
            return m_gameLiftLobbyCanvas->GetServerName();

        default:
            return "";

        }
    }
}
