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

#include "GridMocks.h"

#include <GridMate/Replica/Interest/BitmaskInterestHandler.h>
#include <GridMate/Replica/Interest/InterestManager.h>
#include <GridMate/Replica/Interest/ProximityInterestHandler.h>

#include <AzFramework/Network/InterestManagerComponent.h>
#include <AzCore/Socket/AzSocket.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    using testing::_;

    class MockInterestManagerEvents
        : public AzFramework::InterestManagerEventsBus::Handler
    {
    public:
        MockInterestManagerEvents()
        {
            BusConnect();
        }

        virtual ~MockInterestManagerEvents()
        {
            BusDisconnect();
        }

        MOCK_METHOD1(OnInterestManagerActivate, void(GridMate::InterestManager* im));
        MOCK_METHOD1(OnInterestManagerDeactivate, void(GridMate::InterestManager* im));
    };

    class InterestManagerComponentFixture
        : public AllocatorsFixture
    {
    public:
        InterestManagerComponentFixture()
            : AllocatorsFixture()
        {
            
        }

        ~InterestManagerComponentFixture()
        {
            
        }

        void SetUp() override
        {
            AZ::AzSock::Startup();

            AllocatorsFixture::SetUp();
            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();
            m_gridMate = GridMate::GridMateCreate(GridMate::GridMateDesc());
            m_carrier = GridMate::DefaultCarrier::Create(GridMate::CarrierDesc(), m_gridMate);

            m_sessionService = AZStd::make_unique<UnitTest::MockSessionService>();
            m_gridSession = AZStd::make_unique<UnitTest::MockSession>(m_sessionService.get());

            m_replicaManagerDesc.m_carrier = m_carrier;
            m_replicaManagerDesc.m_myPeerId = AZ::Crc32(testing::UnitTest::GetInstance()->current_test_info()->test_case_name());
            m_replicaManagerDesc.m_roles = GridMate::ReplicaMgrDesc::Role_SyncHost;
            m_replicaManager = AZStd::make_unique<GridMate::ReplicaManager>();
            m_replicaManager->Init(m_replicaManagerDesc);

            m_gridSession->SetReplicaManager(m_replicaManager.get());
        }

        void TearDown() override
        {
            m_gridSession = nullptr;
            m_sessionService = nullptr;

            m_replicaManager->Shutdown();
            m_replicaManager = nullptr;

            m_carrier->Shutdown();
            delete m_carrier;
            GridMate::GridMateDestroy(m_gridMate);
            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
            AllocatorsFixture::TearDown();

            AZ::AzSock::Cleanup();
        }

        AZStd::unique_ptr<UnitTest::MockSessionService> m_sessionService;
        AZStd::unique_ptr<UnitTest::MockSession> m_gridSession;

        GridMate::IGridMate* m_gridMate;
        GridMate::Carrier* m_carrier;

        GridMate::ReplicaMgrDesc m_replicaManagerDesc;
        AZStd::unique_ptr<GridMate::ReplicaManager> m_replicaManager;
    };

    TEST_F(InterestManagerComponentFixture, TestNetworkSessionDeactivate)
    {
        // Using StrictMock here will ensure that the test fails if any of the events fire (as no EXPECT_CALL has been set).
        testing::StrictMock<MockInterestManagerEvents> interestManagerEvents;
        AzFramework::InterestManagerComponent interestManagerComponent;

        // This will connect the component to the NetBindingSystemEventsBus
        interestManagerComponent.Activate();

        // Ensure that the interest manager component handles receiving OnNetworkSessionDeactivated for a session that was never activated.
        // This can happen in the event of a client failing to connect to a host.
        AzFramework::NetBindingSystemEventsBus::Broadcast(
            &AzFramework::NetBindingSystemEvents::OnNetworkSessionDeactivated, m_gridSession.get());

        interestManagerComponent.Deactivate();
    }

    TEST_F(InterestManagerComponentFixture, TestNetworkSessionActivateAndDeactivate)
    {
        // Using StrictMock here will ensure that the test fails if any of the events fire (as no EXPECT_CALL has been set).
        testing::StrictMock<MockInterestManagerEvents> interestManagerEvents;
        AzFramework::InterestManagerComponent interestManagerComponent;
        GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<GridMate::BitmaskInterestChunk>();
        GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<GridMate::ProximityInterestChunk>();

        // This will connect the component to the NetBindingSystemEventsBus
        interestManagerComponent.Activate();

        // Golden path test that the interest manager component behaves as expected under normal conditions
        // (receiving OnNetworkSessionActivated followed by OnNetworkSessionDeactivated).
        testing::Expectation activationEvent = EXPECT_CALL(interestManagerEvents, OnInterestManagerActivate(_))
            .Times(1);
        AzFramework::NetBindingSystemEventsBus::Broadcast(
            &AzFramework::NetBindingSystemEvents::OnNetworkSessionActivated, m_gridSession.get());

        EXPECT_CALL(interestManagerEvents, OnInterestManagerDeactivate(_))
            .Times(1)
            .After(activationEvent);
        AzFramework::NetBindingSystemEventsBus::Broadcast(
            &AzFramework::NetBindingSystemEvents::OnNetworkSessionDeactivated, m_gridSession.get());

        interestManagerComponent.Deactivate();
    }
}
