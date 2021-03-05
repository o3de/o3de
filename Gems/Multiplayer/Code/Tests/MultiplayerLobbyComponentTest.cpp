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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <gmock/gmock.h>
#include <GridMate/Session/LANSession.h>
#include <Multiplayer/MultiplayerLobbyComponent.h>
#include "MultiplayerMocks.h"
#include <Multiplayer_Traits_Platform.h>

#if !defined(AZ_TRAIT_MULTIPLAYER_DISABLE_GAMELIFT_TESTS)
#if defined(BUILD_GAMELIFT_CLIENT)
namespace UnitTest
{ 
    class MultiplayerLobbyComponentMock : public Multiplayer::MultiplayerLobbyComponent
    {
    public:
        using Multiplayer::MultiplayerLobbyComponent::ShowSelectionLobby;
        using Multiplayer::MultiplayerLobbyComponent::SelectGameLiftServerType;
        using Multiplayer::MultiplayerLobbyComponent::SelectLANServerType;
        using Multiplayer::MultiplayerLobbyComponent::CreateServer;
        using Multiplayer::MultiplayerLobbyComponent::JoinServer;
        using Multiplayer::MultiplayerLobbyComponent::ListServers;
        using Multiplayer::MultiplayerLobbyComponent::StartSessionService;
        using Multiplayer::MultiplayerLobbyComponent::SanityCheck;
        using Multiplayer::MultiplayerLobbyComponent::SanityCheckGameLift;
        using Multiplayer::MultiplayerLobbyComponent::StartGameLiftMatchmaking;
        using Multiplayer::MultiplayerLobbyComponent::m_listSearch;
        using Multiplayer::MultiplayerLobbyComponent::m_gameliftCreationSearch;
        using Multiplayer::MultiplayerLobbyComponent::m_dedicatedHostTypeSelectionCanvas;
        using Multiplayer::MultiplayerLobbyComponent::m_gameLiftLobbyCanvas;
        using Multiplayer::MultiplayerLobbyComponent::m_busyAndErrorCanvas;
        using Multiplayer::MultiplayerLobbyComponent::m_lanGameLobbyCanvas;
        using Multiplayer::MultiplayerLobbyComponent::m_multiplayerLobbyServiceWrapper;
        
        AZ_COMPONENT(MultiplayerLobbyComponentMock, "{98B2857C-C738-478A-80AD-048FB79AEA79}");
        MultiplayerLobbyComponentMock()
        {
            ON_CALL(*this, Activate()).WillByDefault(Invoke(this, &MultiplayerLobbyComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(Invoke(this, &MultiplayerLobbyComponentMock::DeactivateMock));
            ON_CALL(*this, SanityCheck()).WillByDefault(Return(true));
            ON_CALL(*this, SanityCheckGameLift()).WillByDefault(Return(true));
        }

        void ActivateMock()
        {
            Multiplayer::MultiplayerLobbyBus::Handler::BusConnect(GetEntityId());
            GridMate::SessionEventBus::Handler::BusConnect(gEnv->pNetwork->GetGridMate());

            m_dedicatedHostTypeSelectionCanvas = aznew MultiplayerDedicatedHostTypeSelectionCanvasMock();
            m_gameLiftLobbyCanvas = aznew MultiplayerGameLiftLobbyCanvasMock();
            m_busyAndErrorCanvas = aznew MultiplayerBusyAndErrorCanvasMock();
            m_lanGameLobbyCanvas = aznew MultiplayerLANGameLobbyCanvasMock();
        }

        void DeactivateMock()
        {
            ClearSearches();
            delete m_dedicatedHostTypeSelectionCanvas;
            delete m_gameLiftLobbyCanvas;
            delete m_busyAndErrorCanvas;
            delete m_lanGameLobbyCanvas;

            GridMate::SessionEventBus::Handler::BusDisconnect();
            Multiplayer::MultiplayerLobbyBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());
        MOCK_METHOD0(SanityCheck, bool());
        MOCK_METHOD0(SanityCheckGameLift, bool());
    };
}

class MultiplayerLobbyComponentTest : public UnitTest::MultiplayerClientSessionAllocatorFixture
{
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;

public:
    UnitTest::MultiplayerLobbyComponentMock* m_lobbyComponent;
    AZ::Entity* m_entity;

    void SetUp() override
    {
        UnitTest::MultiplayerClientSessionAllocatorFixture::SetUp();
        ApplyCVars();
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_componentDescriptor.reset(UnitTest::MultiplayerLobbyComponentMock::CreateDescriptor());
        m_componentDescriptor->Reflect(m_serializeContext.get());

        m_entity = aznew AZ::Entity();
        m_lobbyComponent = m_entity->CreateComponent<UnitTest::MultiplayerLobbyComponentMock>();
        m_entity->Init();

        EXPECT_CALL(*m_lobbyComponent, Activate()).Times(1);
        EXPECT_CALL(*m_lobbyComponent, Deactivate()).Times(1);
        m_entity->Activate();
    }

    void TearDown() override
    {
        m_entity->Deactivate();
        m_entity->RemoveComponent(m_lobbyComponent);
        delete m_lobbyComponent;
        m_lobbyComponent = nullptr;

        delete m_entity;
        m_entity = nullptr;

        m_serializeContext.reset();
        m_componentDescriptor.reset();
        m_console.reset();

        UnitTest::MultiplayerClientSessionAllocatorFixture::TearDown();
    }
};

class MultiplayerLANLobbyComponentTest : public MultiplayerLobbyComponentTest
{
public:
    void SetUp() override
    {
        MultiplayerLobbyComponentTest::SetUp();

        EXPECT_CALL(*(UnitTest::MultiplayerDedicatedHostTypeSelectionCanvasMock*)m_lobbyComponent->m_dedicatedHostTypeSelectionCanvas, Show()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissError(UnitTest::_)).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
        m_lobbyComponent->ShowSelectionLobby();

        EXPECT_CALL(*(UnitTest::MultiplayerDedicatedHostTypeSelectionCanvasMock*)m_lobbyComponent->m_dedicatedHostTypeSelectionCanvas, Hide()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, Show()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, ClearSearchResults()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissError(UnitTest::_)).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
        m_lobbyComponent->SelectLANServerType();

        if (m_lobbyComponent->m_multiplayerLobbyServiceWrapper)
        {
            delete m_lobbyComponent->m_multiplayerLobbyServiceWrapper;
        }

        m_lobbyComponent->m_multiplayerLobbyServiceWrapper = aznew testing::NiceMock<UnitTest::MultiplayerLobbyLANServiceWrapperMock>(m_lobbyComponent->GetEntityId());
    }

    void TearDown() override
    {
        delete m_lobbyComponent->m_multiplayerLobbyServiceWrapper;
        MultiplayerLobbyComponentTest::TearDown();
    }

};

TEST_F(MultiplayerLANLobbyComponentTest, LANCreateServer_Success)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, GetMapName()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, GetServerName()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    m_lobbyComponent->CreateServer();
}

TEST_F(MultiplayerLANLobbyComponentTest, LANCreateServer_EmptyMapNameError)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(0);
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, GetMapName()).Times(1).WillOnce(UnitTest::Return(""));
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowError(UnitTest::_)).Times(1);
    m_lobbyComponent->CreateServer();
}

TEST_F(MultiplayerLANLobbyComponentTest, LANListAndJoinServer_Success)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLobbyLANServiceWrapperMock*)m_lobbyComponent->m_multiplayerLobbyServiceWrapper, ListServers(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, ClearSearchResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    m_lobbyComponent->ListServers();

    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, DisplaySearchResults(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
    ((UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch)->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_lobbyComponent->m_listSearch);

    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLobbyLANServiceWrapperMock*)m_lobbyComponent->m_multiplayerLobbyServiceWrapper, JoinSession(UnitTest::_, UnitTest::_, UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, GetSelectedServerResult()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetNumResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetResult(UnitTest::_)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    m_lobbyComponent->JoinServer();
}

TEST_F(MultiplayerLANLobbyComponentTest, LANListServer_LANServiceWrapperListServerError)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLobbyLANServiceWrapperMock*)m_lobbyComponent->m_multiplayerLobbyServiceWrapper, ListServers(UnitTest::_)).Times(1).WillOnce(UnitTest::Return(nullptr));
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowError(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, ClearSearchResults()).Times(1);
    m_lobbyComponent->ListServers();

    AZ_Assert(m_lobbyComponent->m_listSearch == nullptr, "Expected ListSearch to be Null after error");
}

TEST_F(MultiplayerLANLobbyComponentTest, LANJoinServer_WithoutListServersError)
{
    EXPECT_CALL(*(UnitTest::MultiplayerLobbyLANServiceWrapperMock*)m_lobbyComponent->m_multiplayerLobbyServiceWrapper, JoinSession(UnitTest::_, UnitTest::_, UnitTest::_)).Times(0);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowError(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, GetSelectedServerResult()).Times(1); 
    m_lobbyComponent->JoinServer();
}

TEST_F(MultiplayerLANLobbyComponentTest, LANListAndJoinServer_LANServiceWrapperJoinSessionError)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLobbyLANServiceWrapperMock*)m_lobbyComponent->m_multiplayerLobbyServiceWrapper, ListServers(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, ClearSearchResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    m_lobbyComponent->ListServers();

    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, DisplaySearchResults(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
    ((UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch)->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_lobbyComponent->m_listSearch);

    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerLobbyLANServiceWrapperMock*)m_lobbyComponent->m_multiplayerLobbyServiceWrapper, JoinSession(UnitTest::_, UnitTest::_, UnitTest::_)).Times(1)
        .WillOnce(UnitTest::Return(nullptr));
    EXPECT_CALL(*(UnitTest::MultiplayerLANGameLobbyCanvasMock*)m_lobbyComponent->m_lanGameLobbyCanvas, GetSelectedServerResult()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetNumResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetResult(UnitTest::_)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(0);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowError(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    m_lobbyComponent->JoinServer();
}


class MultiplayerGameLiftLobbyComponentTest : public MultiplayerLobbyComponentTest
{
public:

    void SetUp() override
    {
        MultiplayerLobbyComponentTest::SetUp();
        GridMate::GameLiftClientServiceDesc desc;

        EXPECT_CALL(*(UnitTest::MultiplayerDedicatedHostTypeSelectionCanvasMock*)m_lobbyComponent->m_dedicatedHostTypeSelectionCanvas, Show()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissError(UnitTest::_)).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
        m_lobbyComponent->ShowSelectionLobby();

        EXPECT_CALL(*(UnitTest::MultiplayerDedicatedHostTypeSelectionCanvasMock*)m_lobbyComponent->m_dedicatedHostTypeSelectionCanvas, Hide()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, Show()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, ClearSearchResults()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissError(UnitTest::_)).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
        EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(2);
        EXPECT_CALL(m_gameLiftRequestBus, StartClientService(UnitTest::_)).Times(1);
        m_lobbyComponent->SelectGameLiftServerType();
        GridMate::GameLiftClientServiceEventsBus::Broadcast(
            &GridMate::GameLiftClientServiceEventsBus::Events::OnGameLiftSessionServiceReady, nullptr);
    }
};

TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftCreateServer_Success)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*m_lobbyComponent, SanityCheckGameLift()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetMapName()).Times(2);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetServerName()).Times(3);
    EXPECT_CALL(m_gameLiftClientServiceBus, RequestSession(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);

    m_lobbyComponent->CreateServer();

    EXPECT_CALL(m_gameLiftClientServiceBus, JoinSessionBySearchInfo(UnitTest::_, UnitTest::_)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_gameliftCreationSearch, GetResult(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_gameliftCreationSearch, GetNumResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
    ((UnitTest::MockGridSearch*)m_lobbyComponent->m_gameliftCreationSearch)->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_lobbyComponent->m_gameliftCreationSearch);
}

TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftCreateServer_EmptyServerName_Error)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetMapName()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetServerName()).Times(1).WillOnce(UnitTest::Return(""));
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowError(UnitTest::_)).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, RequestSession(UnitTest::_)).Times(0);
    m_lobbyComponent->CreateServer();
}

TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftCreateServer_SanityCheckFail_Error)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1).WillOnce(UnitTest::Return(false));
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetMapName()).Times(0);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetServerName()).Times(0);
    EXPECT_CALL(m_gameLiftClientServiceBus, RequestSession(UnitTest::_)).Times(0);
    m_lobbyComponent->CreateServer();
}

TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftListAndJoin_Success)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheckGameLift()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, ClearSearchResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, StartSearch(UnitTest::_)).Times(1);
    m_lobbyComponent->ListServers();

    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, DisplaySearchResults(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
    ((UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch)->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_lobbyComponent->m_listSearch);

    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*m_lobbyComponent, SanityCheckGameLift()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetSelectedServerResult()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetNumResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetResult(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, JoinSessionBySearchInfo(UnitTest::_, UnitTest::_)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(1);
    m_lobbyComponent->JoinServer();
}

TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftList_SanityCheckGameLiftFail_Error)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheckGameLift()).Times(1).WillOnce(UnitTest::Return(false));
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, ClearSearchResults()).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, StartSearch(UnitTest::_)).Times(0);
    m_lobbyComponent->ListServers();
}

TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftListAndJoin_SanityCheckGameLiftFail_Error)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheckGameLift()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, ClearSearchResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, StartSearch(UnitTest::_)).Times(1);
    m_lobbyComponent->ListServers();

    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, DisplaySearchResults(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
    ((UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch)->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_lobbyComponent->m_listSearch);

    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1);
    EXPECT_CALL(*m_lobbyComponent, SanityCheckGameLift()).Times(1).WillOnce(UnitTest::Return(false));
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetSelectedServerResult()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetNumResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetResult(UnitTest::_)).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, JoinSessionBySearchInfo(UnitTest::_, UnitTest::_)).Times(0);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(0);
    m_lobbyComponent->JoinServer();
}

TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftListAndJoin_SanityCheckFail_Error)
{
    EXPECT_CALL(*m_lobbyComponent, SanityCheckGameLift()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, ClearSearchResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, StartSearch(UnitTest::_)).Times(1);
    m_lobbyComponent->ListServers();

    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, DisplaySearchResults(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
    ((UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch)->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_lobbyComponent->m_listSearch);

    EXPECT_CALL(*m_lobbyComponent, SanityCheck()).Times(1).WillOnce(UnitTest::Return(false));
    EXPECT_CALL(*(UnitTest::MultiplayerGameLiftLobbyCanvasMock*)m_lobbyComponent->m_gameLiftLobbyCanvas, GetSelectedServerResult()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetNumResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_listSearch, GetResult(UnitTest::_)).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, JoinSessionBySearchInfo(UnitTest::_, UnitTest::_)).Times(0);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(0);
    m_lobbyComponent->JoinServer();
}


TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftMatchmaking_Success)
{
    EXPECT_CALL(m_gameLiftClientServiceBus, StartMatchmaking(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    m_lobbyComponent->StartGameLiftMatchmaking();

    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_gameliftCreationSearch, GetNumResults()).Times(1);
    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_gameliftCreationSearch, GetResult(UnitTest::_)).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, JoinSessionBySearchInfo(UnitTest::_, UnitTest::_)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
    ((UnitTest::MockGridSearch*)m_lobbyComponent->m_gameliftCreationSearch)->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_lobbyComponent->m_gameliftCreationSearch);
}

TEST_F(MultiplayerGameLiftLobbyComponentTest, GameLiftMatchmaking_GameLiftClientServiceBus_StartMatchmakingEmptySearch_Error)
{
    EXPECT_CALL(m_gameLiftClientServiceBus, StartMatchmaking(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowBusyScreen()).Times(1);
    m_lobbyComponent->StartGameLiftMatchmaking();

    EXPECT_CALL(*(UnitTest::MockGridSearch*)m_lobbyComponent->m_gameliftCreationSearch, GetNumResults()).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, JoinSessionBySearchInfo(UnitTest::_, UnitTest::_)).Times(0);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(UnitTest::_)).Times(0);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, ShowError(UnitTest::_)).Times(1);
    EXPECT_CALL(*(UnitTest::MultiplayerBusyAndErrorCanvasMock*)m_lobbyComponent->m_busyAndErrorCanvas, DismissBusyScreen(UnitTest::_)).Times(1);
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_lobbyComponent->m_gameliftCreationSearch);
}

#endif //BUILD_GAMELIFT_CLIENT
#endif //AZ_TRAIT_MULTIPLAYER_DISABLE_GAMELIFT_TESTS
