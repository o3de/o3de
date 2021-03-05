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

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/containers/ring_buffer.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Network/NetBindingComponent.h>
#include <AzFramework/Network/NetBindingSystemComponent.h>
#include <AzFramework/Network/NetBindable.h>
#include <GridMate/GridMate.h>
#include <GridMate/Session/LANSession.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Serialize/CompressionMarshal.h>
#include <GridMate/Carrier/Utils.h>

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>

namespace UnitTest
{
#if 0
    using namespace AZ;

    /**
    */
    class NetBindingTestComponent
        : public AZ::Component
        , public AzFramework::NetBindable
    {
        friend class NetBindingComponentChunk;

    public:
        AZ_COMPONENT(NetBindingTestComponent, "{DE5CF1C0-B4B6-4BB0-86FE-936B400871E0}", AzFramework::NetBindable);

    protected:
        class NetChunk
            : public GridMate::ReplicaChunk
        {
        public:
            AZ_CLASS_ALLOCATOR(NetChunk, AZ::SystemAllocator, 0);

            static const char* GetChunkName() { return "NetBindingTestComponent::NetChunk"; }

            bool IsReplicaMigratable() override { return false; }
        };

        ///////////////////////////////////////////////////////////////////////
        // NetBindable
        GridMate::ReplicaChunkPtr GetNetworkBinding() override
        {
            AZ_TracePrintf("NetBinding", "NetBindingTestComponent::GetNetworkBinding()\n");
            m_chunk = GridMate::CreateReplicaChunk<NetChunk>();
            AZ_Assert(m_chunk, "Failed to create NetBindingTestComponent::NetChunk!");
            return m_chunk;
        }

        void SetNetworkBinding(GridMate::ReplicaChunkPtr binding) override
        {
            AZ_TracePrintf("NetBinding", "NetBindingTestComponent::SetNetworkBinding()\n");
            AZ_TEST_ASSERT(binding);
            AZ_TEST_ASSERT(binding->GetDescriptor()->GetChunkTypeId() == GridMate::ReplicaChunkClassId(NetChunk::GetChunkName()));
            m_chunk = AZStd::static_pointer_cast<NetChunk>(binding);
        }

        void UnbindFromNetwork() override
        {
            if (m_chunk)
            {
                AZ_TracePrintf("NetBinding", "NetBindingTestComponent::UnbindFromNetwork()\n");
                m_chunk = nullptr;
            }
        }
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        // AZ::Component
        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<NetBindingTestComponent, AZ::Component, AzFramework::NetBindable>()
                    ;
            }

            // We also need to register the chunk type, and this would be a good time to do so.
            GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<NetChunk>();
        }

        void Activate() override
        {
            AZ_TracePrintf("NetBinding", "NetBindingTestComponent::Activate()\n");
        }

        void Deactivate() override
        {
            AZ_TracePrintf("NetBinding", "NetBindingTestComponent::Deactivate()\n");
            UnbindFromNetwork();
        }
        ///////////////////////////////////////////////////////////////////////

        AZStd::intrusive_ptr<NetChunk> m_chunk;
    };

    /**
    * Fakes the behavior of NetBindingSystemContextData on the host side
    */
    class FakeNetBindingContextChunk
        : public GridMate::ReplicaChunk
    {
    public:
        AZ_CLASS_ALLOCATOR(FakeNetBindingContextChunk, AZ::SystemAllocator, 0);

        static const char* GetChunkName() { return "NetBindingSystemContextData"; } // We are pretending to be a NetBindingSystemContextData

        FakeNetBindingContextChunk()
            : m_bindingContextSequence("BindingContextSequence", AzFramework::UnspecifiedNetBindingContextSequence)
        {
        }

        bool IsReplicaMigratable() override { return true; }

        GridMate::DataSet<AZ::u32, GridMate::VlqU32Marshaler> m_bindingContextSequence;
    };

    /*
    * NetBindingSystemComponentLifecycleTest
    */
    class NetBindingSystemComponentLifecycleTest
        : public GridMate::SessionEventBus::Handler
        , public AzFramework::NetBindingHandlerBus::Handler
    {
    public:
        void OnSessionCreated(GridMate::GridSession* session) override
        {
            if (session == m_session)
            {
                if (session->IsHost())
                {
                    EBUS_EVENT(AzFramework::NetBindingSystemBus, OnNetworkSessionActivated, session);
                }
            }
        }

        void OnSessionJoined(GridMate::GridSession* session) override
        {
            if (session == m_session)
            {
                EBUS_EVENT(AzFramework::NetBindingSystemBus, OnNetworkSessionActivated, session);
            }
        }

        void OnSessionDelete(GridMate::GridSession* session)
        {
            if (session == m_session)
            {
                EBUS_EVENT(AzFramework::NetBindingSystemBus, OnNetworkSessionDeactivated, session);
                m_session = nullptr;
            }
        }

        void BindToNetwork(GridMate::ReplicaPtr bindTo) override
        {
            // Verify that BindToNetwork() is not called more than once
            AZ_TEST_ASSERT(!m_receivedBindEvent);
            m_receivedBindEvent = true;

            // Test that now we should be binding to the network
            bool shouldBindToNetwork = false;
            EBUS_EVENT_RESULT(shouldBindToNetwork, AzFramework::NetBindingSystemBus, ShouldBindToNetwork);
            AZ_TEST_ASSERT(shouldBindToNetwork);

            // Verify that the context sequence is no longer unspecified
            AzFramework::NetBindingContextSequence contextSequence = AzFramework::UnspecifiedNetBindingContextSequence;
            EBUS_EVENT_RESULT(contextSequence, AzFramework::NetBindingSystemBus, GetCurrentContextSequence);
            AZ_TEST_ASSERT(contextSequence != AzFramework::UnspecifiedNetBindingContextSequence);
        }

        void UnbindFromNetwork() override
        {
            // Verify that UnbindFromNetwork() is not called more than once
            AZ_TEST_ASSERT(!m_receivedUnbindEvent);
            m_receivedUnbindEvent = true;
        }

        void run()
        {
            // Setup
            AZ::ComponentApplication app;
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_recordsMode = AZ::Debug::AllocationRecords::RECORD_FULL;
            AZ::Entity* systemEntity = app.Create(appDesc);

            app.RegisterComponentDescriptor(AzFramework::NetBindingSystemComponent::CreateDescriptor());
            app.RegisterComponentDescriptor(AzFramework::GameEntityContextComponent::CreateDescriptor());

            systemEntity->Init();
            systemEntity->CreateComponent<AZ::MemoryComponent>();
            systemEntity->CreateComponent<AZ::AssetManagerComponent>();
            systemEntity->CreateComponent<AzFramework::GameEntityContextComponent>();
            systemEntity->CreateComponent<AzFramework::NetBindingSystemComponent>();
            systemEntity->Activate();
            AzFramework::NetBindingHandlerBus::Handler::BusConnect();

            GridMate::GridMateDesc gridMateDesc;
            GridMate::IGridMate* gridMate = GridMate::GridMateCreate(gridMateDesc);
            GridMate::GridMateAllocatorMP::Descriptor allocDesc;
            allocDesc.m_stackRecordLevels = 15;
            allocDesc.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create(allocDesc);
            if (AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Get().GetRecords())
            {
                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Get().GetRecords()->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
            }
            GridMate::StartGridMateService<GridMate::LANSessionService>(gridMate, GridMate::SessionServiceDesc());
            GridMate::SessionEventBus::Handler::BusConnect(gridMate);

            // Test offline behavior
            {
                bool shouldBindToNetwork = true;
                EBUS_EVENT_RESULT(shouldBindToNetwork, AzFramework::NetBindingSystemBus, ShouldBindToNetwork);
                AZ_TEST_ASSERT(!shouldBindToNetwork);

                AzFramework::NetBindingContextSequence offlineContextSequence = 0xBADF00D;
                EBUS_EVENT_RESULT(offlineContextSequence, AzFramework::NetBindingSystemBus, GetCurrentContextSequence);
                AZ_TEST_ASSERT(offlineContextSequence == AzFramework::UnspecifiedNetBindingContextSequence);
            }

            // Test host-side behavior
            {
                m_receivedBindEvent = m_receivedUnbindEvent = false;

                // Host a session
                GridMate::CarrierDesc carrierDesc;
                carrierDesc.m_enableDisconnectDetection = true;
                GridMate::LANSessionParams sessionParams;
                sessionParams.m_numPublicSlots = 10;
                sessionParams.m_flags = 0;
                sessionParams.m_port = HOST_PORT;
                sessionParams.m_params[sessionParams.m_numParams].m_id = "filter";
                sessionParams.m_params[sessionParams.m_numParams].m_value = GridMate::Utils::GetMachineAddress();
                sessionParams.m_numParams++;
                m_session = gridMate->GetMultiplayerService()->HostSession(&sessionParams, carrierDesc);

                int nFrame = 0;
                while (m_session)
                {
                    if (nFrame == 10)
                    {
                        // Verify that BindToNetwork() has been called
                        AZ_TEST_ASSERT(m_receivedBindEvent);

                        // Verify that we have a valid context sequence
                        AzFramework::NetBindingContextSequence contextSequence1 = AzFramework::UnspecifiedNetBindingContextSequence;
                        EBUS_EVENT_RESULT(contextSequence1, AzFramework::NetBindingSystemBus, GetCurrentContextSequence);
                        AZ_TEST_ASSERT(contextSequence1 != AzFramework::UnspecifiedNetBindingContextSequence);

                        EBUS_EVENT(AzFramework::GameEntityContextRequestBus, ResetGameContext);

                        // Verify that the context sequence was incremented
                        AzFramework::NetBindingContextSequence contextSequence2 = contextSequence1;
                        EBUS_EVENT_RESULT(contextSequence2, AzFramework::NetBindingSystemBus, GetCurrentContextSequence);
                        AZ_TEST_ASSERT(contextSequence2 != AzFramework::UnspecifiedNetBindingContextSequence);
                        AZ_TEST_ASSERT(contextSequence2 > contextSequence1);

                        m_session->Leave(false);
                    }

                    app.Tick();
                    gridMate->Update();
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                    nFrame++;
                }

                // Verify that we should no longer bind to the network
                bool shouldBindToNetwork = true;
                EBUS_EVENT_RESULT(shouldBindToNetwork, AzFramework::NetBindingSystemBus, ShouldBindToNetwork);
                AZ_TEST_ASSERT(!shouldBindToNetwork);

                // Verify that the context sequence was reset to unspecified
                AzFramework::NetBindingContextSequence offlineContextSequence = 0xBADF00D;
                EBUS_EVENT_RESULT(offlineContextSequence, AzFramework::NetBindingSystemBus, GetCurrentContextSequence);
                AZ_TEST_ASSERT(offlineContextSequence == AzFramework::UnspecifiedNetBindingContextSequence);
            }

            // Test nonhost-side behavior by faking the behavior on the host side and then joining the host session.
            {
                m_receivedBindEvent = m_receivedUnbindEvent = false;

                // Host a session
                GridMate::CarrierDesc carrierDesc;
                carrierDesc.m_enableDisconnectDetection = true;
                GridMate::LANSessionParams sessionParams;
                sessionParams.m_numPublicSlots = 10;
                sessionParams.m_flags = 0;
                sessionParams.m_port = HOST_PORT;
                sessionParams.m_params[sessionParams.m_numParams].m_id = "filter";
                sessionParams.m_params[sessionParams.m_numParams].m_value = GridMate::Utils::GetMachineAddress();
                sessionParams.m_numParams++;
                GridMate::GridSession* hostSession = gridMate->GetMultiplayerService()->HostSession(&sessionParams, carrierDesc);

                // Add the fake context replica on the host and set the context sequence to 1
                GridMate::ReplicaPtr replica = GridMate::Replica::CreateReplica("Potato");
                FakeNetBindingContextChunk* contextChunk = GridMate::CreateReplicaChunk<FakeNetBindingContextChunk>();
                replica->AttachReplicaChunk(contextChunk);
                while (!hostSession->IsReady())
                {
                    gridMate->Update();
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                }
                hostSession->GetReplicaMgr()->AddMaster(replica);
                contextChunk->m_bindingContextSequence.Set(1);

                int nFrame = 0;
                while (m_session)
                {
                    if (nFrame == 10)
                    {
                        // Join the hosted session
                        GridMate::SessionIdInfo sessionInfo;
                        sessionInfo.m_sessionId = hostSession->GetId();
                        m_session = gridMate->GetMultiplayerService()->JoinSession(&sessionInfo, GridMate::JoinParams(), carrierDesc);
                    }

                    if (nFrame == 20)
                    {
                        // Verify that BindToNetwork() has been called
                        AZ_TEST_ASSERT(m_receivedBindEvent);

                        // Verify that we have a valid context sequence
                        AzFramework::NetBindingContextSequence contextSequence = AzFramework::UnspecifiedNetBindingContextSequence;
                        EBUS_EVENT_RESULT(contextSequence, AzFramework::NetBindingSystemBus, GetCurrentContextSequence);
                        AZ_TEST_ASSERT(contextSequence == contextChunk->m_bindingContextSequence.Get());

                        // Simulate a context switch on the host
                        contextChunk->m_bindingContextSequence.Set(contextChunk->m_bindingContextSequence.Get() + 1);
                    }

                    if (nFrame == 30)
                    {
                        // Verify that the context sequence was incremented
                        AzFramework::NetBindingContextSequence contextSequence = AzFramework::UnspecifiedNetBindingContextSequence;
                        EBUS_EVENT_RESULT(contextSequence, AzFramework::NetBindingSystemBus, GetCurrentContextSequence);
                        AZ_TEST_ASSERT(contextSequence == contextChunk->m_bindingContextSequence.Get());

                        hostSession->Leave(false);
                    }

                    app.Tick();
                    gridMate->Update();
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                    nFrame++;
                }

                // Verify that we should no longer bind to the network
                bool shouldBindToNetwork = true;
                EBUS_EVENT_RESULT(shouldBindToNetwork, AzFramework::NetBindingSystemBus, ShouldBindToNetwork);
                AZ_TEST_ASSERT(!shouldBindToNetwork);

                // Verify that the context sequence was reset to unspecified
                AzFramework::NetBindingContextSequence offlineContextSequence = 0xBADF00D;
                EBUS_EVENT_RESULT(offlineContextSequence, AzFramework::NetBindingSystemBus, GetCurrentContextSequence);
                AZ_TEST_ASSERT(offlineContextSequence == AzFramework::UnspecifiedNetBindingContextSequence);
            }

            // Clean up
            GridMate::SessionEventBus::Handler::BusDisconnect();
            GridMate::GridMateDestroy(gridMate);
            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
            AzFramework::NetBindingHandlerBus::Handler::BusDisconnect();
            app.Destroy();
        }

        static const int HOST_PORT = 5000;

        GridMate::GridSession* m_session;
        bool m_receivedBindEvent;
        bool m_receivedUnbindEvent;
    };

    /*
    * NetBindingFeatureTest (requires two instances)
    */
    class NetBindingFeatureTest
        : public GridMate::SessionEventBus::Handler
    {
    public:
        void OnSessionCreated(GridMate::GridSession* session) override
        {
            if (session == m_session)
            {
                if (session->IsHost())
                {
                    EBUS_EVENT(AzFramework::NetBindingSystemBus, OnNetworkSessionActivated, session);
                }
            }
        }

        void OnSessionJoined(GridMate::GridSession* session) override
        {
            if (session == m_session)
            {
                EBUS_EVENT(AzFramework::NetBindingSystemBus, OnNetworkSessionActivated, session);
            }
        }

        void OnSessionDelete(GridMate::GridSession* session)
        {
            if (session == m_session)
            {
                EBUS_EVENT(AzFramework::NetBindingSystemBus, OnNetworkSessionDeactivated, session);
                m_session = nullptr;
            }
        }

        void OnGridSearchComplete(GridMate::GridSearch* results) override
        {
            if (results == m_search)
            {
                GridMate::CarrierDesc carrierDesc;
                carrierDesc.m_enableDisconnectDetection = true;

                // Create an entity before we get in the session
                AZ_TracePrintf("NetBinding", "Spawning master entity...\n");
                AZ::Entity* newEntity = nullptr;
                newEntity = aznew Entity;
                newEntity->CreateComponent<NetBindingTestComponent>();
                newEntity->CreateComponent<AzFramework::NetBindingComponent>();
                newEntity->Init();
                newEntity->Activate();
                m_entities.push_back(newEntity);

                if (results->GetNumResults() == 0)
                {
                    // Host a session instead
                    GridMate::LANSessionParams sessionParams;
                    sessionParams.m_numPublicSlots = 10;
                    sessionParams.m_flags = 0;
                    sessionParams.m_port = HOST_PORT;
                    sessionParams.m_params[sessionParams.m_numParams].m_id = "filter";
                    sessionParams.m_params[sessionParams.m_numParams].m_value = GridMate::Utils::GetMachineAddress();
                    sessionParams.m_numParams++;
                    m_session = m_gridMate->GetMultiplayerService()->HostSession(&sessionParams, carrierDesc);

                    m_search->Release();
                }
                else
                {
                    // Join the session
                    GridMate::JoinParams joinParams;
                    m_session = m_gridMate->GetMultiplayerService()->JoinSession(results->GetResult(0), joinParams, carrierDesc);
                }
                m_search = nullptr;
            }
        }

        void OnMemberJoined(GridMate::GridSession* session, GridMate::GridMember* member) override
        {
            if (session == m_session)
            {
                if (session->IsHost())
                {
                    if (member != session->GetMyMember())
                    {
                        // Spawn an entity after session creation
                        AZ_TracePrintf("NetBinding", "Spawning master entity...\n");
                        AZ::Entity* newEntity = nullptr;
                        EBUS_EVENT_RESULT(newEntity, AzFramework::GameEntityContextRequestBus, CreateGameEntity, "ReplicatedEntity2");
                        newEntity->CreateComponent<NetBindingTestComponent>();
                        newEntity->CreateComponent<AzFramework::NetBindingComponent>();
                        newEntity->Init();
                        newEntity->Activate();
                        m_entities.push_back(newEntity);
                    }
                }
            }
        }

        void run()
        {
            m_gridMate = nullptr;
            m_session = nullptr;

            AZ::ComponentApplication app;
            AZ::ComponentApplication::Descriptor appDesc;
            AZ::Entity* systemEntity = app.Create(appDesc);

            app.RegisterComponentDescriptor(AzFramework::NetBindingSystemComponent::CreateDescriptor());
            app.RegisterComponentDescriptor(AzFramework::NetBindingComponent::CreateDescriptor());
            app.RegisterComponentDescriptor(NetBindingTestComponent::CreateDescriptor());
            app.RegisterComponentDescriptor(AzFramework::GameEntityContextComponent::CreateDescriptor());

            systemEntity->Init();
            systemEntity->CreateComponent<AZ::MemoryComponent>();
            systemEntity->CreateComponent<AZ::AssetManagerComponent>();
            systemEntity->CreateComponent<AzFramework::GameEntityContextComponent>();
            systemEntity->CreateComponent<AzFramework::NetBindingSystemComponent>();
            systemEntity->Activate();

            GridMate::GridMateDesc gridMateDesc;
            m_gridMate = GridMate::GridMateCreate(gridMateDesc);

            GridMate::GridMateAllocatorMP::Descriptor allocDesc;
            allocDesc.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create(allocDesc);

            GridMate::StartGridMateService<GridMate::LANSessionService>(m_gridMate, GridMate::SessionServiceDesc());

            GridMate::SessionEventBus::Handler::BusConnect(m_gridMate);

            // Search for an existing session
            // If a session is not found, we will host a session from within the search callback.
            {
                GridMate::LANSearchParams searchParams;
                searchParams.m_serverPort = HOST_PORT;
                searchParams.m_params[searchParams.m_numParams].m_id = "filter";
                searchParams.m_params[searchParams.m_numParams].m_value = GridMate::Utils::GetMachineAddress();
                searchParams.m_params[searchParams.m_numParams].m_op = GridMate::GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                searchParams.m_numParams++;
                m_search = m_gridMate->GetMultiplayerService()->StartGridSearch(&searchParams);

                while (m_search)
                {
                    m_gridMate->Update();
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                }
            }

            // Tick for a while
            //static int nTicks = 100;
            for (int i = 0; m_session; ++i)
            {
                if (m_session->IsHost())
                {
                    if (i > 4000 && m_session->GetNumberOfMembers() == 1)
                    {
                        m_session->Leave(false);
                    }
                }

                m_gridMate->Update();
                app.Tick();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
            }

            GridMate::SessionEventBus::Handler::BusDisconnect();

            GridMate::GridMateDestroy(m_gridMate);

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();

            for (AZ::Entity* entity : m_entities)
            {
                AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
                EBUS_EVENT_ID_RESULT(contextId, entity->GetId(), AzFramework::EntityIdContextQueryBus, GetOwningContextId);
                if (contextId.IsNull())
                {
                    delete entity;
                }
                else
                {
                    EBUS_EVENT(AzFramework::GameEntityContextRequestBus, DestroyGameEntity, entity);
                }
            }

            app.Destroy();
        }

        static const int HOST_PORT = 6000;

        GridMate::IGridMate* m_gridMate;
        GridMate::GridSession* m_session;
        GridMate::GridSearch* m_search;
        AZStd::fixed_vector<AZ::Entity*, 10> m_entities;
    };
#endif
}

AZ_TEST_SUITE(NetBinding)
//AZ_TEST(UnitTest::NetBindingSystemComponentLifecycleTest)
//AZ_TEST(UnitTest::NetBindingFeatureTest)
AZ_TEST_SUITE_END
