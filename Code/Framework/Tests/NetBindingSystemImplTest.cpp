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
#include <AzFramework/Network/NetBindingSystemImpl.h>
#include <AzFramework/Network/NetBindable.h>
#include <AzFramework/Network/NetBindingSystemComponent.h>

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <GridMate/Serialize/CompressionMarshal.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <AzCore/Asset/AssetManager.h>
#include "NetBindingMocks.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-spec-builders.h>
#include <AzCore/Slice/SliceComponent.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;
    using namespace GridMate;

    class NetBindingWithSlicesTest
        : public ScopedAllocatorSetupFixture
    {
    public:
        const NetBindingContextSequence k_fakeContextSeq = 1;
        const AZ::SliceComponent::SliceInstanceId k_fakeSliceInstanceId = Uuid::CreateRandom();
        const AZ::SliceComponent::SliceInstanceId k_fakeSliceInstanceId_Another = Uuid::CreateRandom();

        SliceInstantiationTicket m_sliceTicket = SliceInstantiationTicket(EntityContextId::CreateName("Test"), 1);
        const Data::AssetId k_fakeAssetId = Data::AssetId(Uuid::CreateRandom(), 0);

        const EntityId k_fakeEntityId_One = EntityId(9001);
        const ReplicaId k_repId_One = 1001;
        const EntityId k_fakeEntityId_Two = EntityId(9002);
        const ReplicaId k_repId_Two = 1002;

        AZStd::unique_ptr<NetBindingSystemImpl> m_netBindingImpl;
        AZStd::unique_ptr<MockComponentApplication> m_componentApplication;
        AZStd::unique_ptr<SerializeContext> m_applicationContext;

        AZStd::unique_ptr<MockGameEntityContext> m_gameEntityMock;
        AZStd::unique_ptr<MockReplicaManager> m_replicaManagerMock;
        ReplicaPtr m_replicaMock;

        ComponentDescriptor* m_netBindingSystemComponentDescriptor = nullptr;

        AZStd::intrusive_ptr<MockNetBindingSystemContextData> m_contextChunkMock;

        MockAssetHandler* m_myAssetHandlerAndCatalog = nullptr; // owned by AssetManager
        AZStd::unique_ptr<MockAsset> m_fakeAsset;

        const float k_wayOverSliceTimeout = NetBindingSystemImpl::s_sliceBindingTimeout.count() * 2.f;
        const float k_smallStep = 0.1f;

        void SetUpFakeAssetManager()
        {
            using namespace testing;

            const Data::AssetManager::Descriptor desc;
            Data::AssetManager::Create(desc);

            m_myAssetHandlerAndCatalog = aznew NiceMock<MockAssetHandler>;

            ON_CALL(*m_myAssetHandlerAndCatalog, CreateAsset(_, _))
                .WillByDefault(Invoke([this](const Data::AssetId&, const Data::AssetType&) -> Data::AssetPtr
            {
                m_fakeAsset = AZStd::make_unique<NiceMock<MockAsset>>(k_fakeAssetId);
                return m_fakeAsset.get();
            }));

            ON_CALL(*m_myAssetHandlerAndCatalog, DestroyAsset(_))
                .WillByDefault(Invoke([this](const Data::AssetPtr asset) 
            {
                EXPECT_EQ(asset, m_fakeAsset.get());
                m_fakeAsset.reset();
            }));

            Data::AssetManager::Instance().RegisterHandler(m_myAssetHandlerAndCatalog, AzTypeInfo<DynamicSliceAsset>::Uuid());
            Data::AssetManager::Instance().RegisterHandler(m_myAssetHandlerAndCatalog, AzTypeInfo<MockAsset>::Uuid());
        }

        void SetUp() override
        {
            using namespace testing;

            m_applicationContext.reset(aznew SerializeContext());

            AllocatorInstance<GridMateAllocatorMP>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            DefaultValue<SliceInstantiationTicket>::Set(m_sliceTicket);

            m_gameEntityMock = AZStd::make_unique<NiceMock<MockGameEntityContext>>();
            m_componentApplication = AZStd::make_unique<NiceMock<MockComponentApplication>>();

            ON_CALL(*m_componentApplication, GetSerializeContext())
                .WillByDefault(Invoke([this]()
            {
                return m_applicationContext.get();
            }));

            ON_CALL(*m_gameEntityMock, GetGameEntityContextId())
                .WillByDefault(Return(EntityContextId::CreateRandom()));

            m_netBindingSystemComponentDescriptor = NetBindingSystemComponent::CreateDescriptor();

            ReplicaChunkDescriptorTable::Get().RegisterChunkType<MockNetBindingSystemContextData>();
            m_contextChunkMock.reset(CreateReplicaChunk<NiceMock<MockNetBindingSystemContextData>>());

            ON_CALL(*m_contextChunkMock, ShouldBindToNetwork())
                .WillByDefault(Return(true));

            m_replicaManagerMock = AZStd::make_unique<NiceMock<MockReplicaManager>>();

            ON_CALL(*m_contextChunkMock, GetReplicaManager())
                .WillByDefault(Invoke([this]()
            {
                return m_replicaManagerMock.get();
            }));

            m_replicaMock = Replica::CreateReplica("unittest");

            ON_CALL(*m_replicaManagerMock, FindReplica(_))
                .WillByDefault(Invoke([this](ReplicaId id) -> ReplicaPtr
            {
                AZ_UNUSED(id);
                return m_replicaMock;
            }));

            ON_CALL(*m_contextChunkMock, OnReplicaActivate(_))
                .WillByDefault(Invoke(m_contextChunkMock.get(), &MockNetBindingSystemContextData::Base_OnReplicaActivate));

            m_netBindingImpl = AZStd::make_unique<AzFramework::NetBindingSystemImpl>();
            m_netBindingImpl->Init();

            m_contextChunkMock->OnReplicaActivate(ReplicaContext(nullptr, TimeContext()));

            SetUpFakeAssetManager();
        }

        void TearDown() override
        {
            Data::AssetManager::Destroy();

            m_replicaMock.reset();
            m_replicaManagerMock.reset();
            m_contextChunkMock.reset();

            m_fakeAsset.reset();
            m_netBindingImpl->Shutdown();
            m_netBindingImpl.reset();

            ReplicaChunkDescriptorTable::Get().UnregisterReplicaChunkDescriptor(ReplicaChunkClassId(MockNetBindingSystemContextData::GetChunkName()));

            m_netBindingSystemComponentDescriptor->ReleaseDescriptor();

            m_componentApplication.reset();
            m_gameEntityMock.reset();

            AllocatorInstance<GridMateAllocatorMP>::Destroy();
            AllocatorInstance<ThreadPoolAllocator>::Destroy();

            m_applicationContext.reset();
        }
    };

    TEST_F(NetBindingWithSlicesTest, SameSliceInstanceId_InstantiateDynamicSlice_CallOnce)
    {
        using namespace testing;

        EXPECT_CALL(*m_gameEntityMock, InstantiateDynamicSlice(_, _, _))
            .Times(1);
        EXPECT_CALL(*m_gameEntityMock, CancelDynamicSliceInstantiation(_))
            .Times(1);

        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId; // both mock entities come from the same slice

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_Two;
            spawnContext.m_staticEntityId = k_fakeEntityId_Two;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId; // both mock entities come from the same slice

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_Two, spawnContext);
        }

        // this should kick off NetBindingSystemImpl::ProcessBindRequests
        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
    }

    TEST_F(NetBindingWithSlicesTest, DifferentSliceInstanceId_InstantiateDynamicSlice_CalledTwice)
    {
        using namespace testing;

        EXPECT_CALL(*m_gameEntityMock, InstantiateDynamicSlice(_, _, _))
            .Times(2);
        EXPECT_CALL(*m_gameEntityMock, CancelDynamicSliceInstantiation(_))
            .Times(2);

        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId;

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_Two;
            spawnContext.m_staticEntityId = k_fakeEntityId_Two;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId_Another; // different slice entity

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_Two, spawnContext);
        }

        // this should kick off NetBindingSystemImpl::ProcessBindRequests
        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
    }

    TEST_F(NetBindingWithSlicesTest, AssetManagerDestroyed_InstantiateDynamicSlice_NotCalled)
    {
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId; // both mock entities come from the same slice

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }

        Data::AssetManager::Destroy();

        // this should kick off NetBindingSystemImpl::ProcessBindRequests, but InstantiateDynamicSlice will not be called
        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
    }

    class ExtendedBindingWithSlicesTest
        : public NetBindingWithSlicesTest
    {
    public:
        void SetUp() override
        {
            NetBindingWithSlicesTest::SetUp();
        }

        void TearDown() override
        {
            using namespace testing;

            EXPECT_CALL(*m_gameEntityMock, DestroyGameEntity(k_fakeEntityId_One))
                .Times(AtMost(1));
            EXPECT_CALL(*m_gameEntityMock, DestroyGameEntity(k_fakeEntityId_Two))
                .Times(AtMost(1));

            NetBindingWithSlicesTest::TearDown();
        }

        class InstantiateMockSlice
        {
        public:
            explicit InstantiateMockSlice(ExtendedBindingWithSlicesTest* parent)
            {
                using namespace testing;

                m_mockSliceRef = AZStd::make_unique<MockSliceReference>();
                m_mockSliceInstance = AZStd::make_unique<MockSliceInstance>();

                // container owns the entities and will delete them
                auto mockContainer = AZStd::make_unique<SliceComponent::InstantiatedContainer>();

                auto binding1 = AZStd::make_unique<NiceMock<MockBindingComponent>>();
                mockContainer->m_entities.push_back(CreateMockEntity(parent->k_fakeEntityId_One, binding1.release()));

                auto binding2 = AZStd::make_unique<NiceMock<MockBindingComponent>>();
                mockContainer->m_entities.push_back(CreateMockEntity(parent->k_fakeEntityId_Two, binding2.release()));

                m_mockSliceInstance->SetMockInstantiatedContainer(mockContainer.release());

                SliceComponent::SliceInstanceAddress sliceInstanceAddress(m_mockSliceRef.get(), m_mockSliceInstance.get());

                // This will pass our mock slice to NetBindingSystem
                EBUS_EVENT_ID(parent->m_sliceTicket, SliceInstantiationResultBus, OnSlicePreInstantiate, parent->k_fakeAssetId, sliceInstanceAddress);
                EBUS_EVENT_ID(parent->m_sliceTicket, SliceInstantiationResultBus, OnSliceInstantiated, parent->k_fakeAssetId, sliceInstanceAddress);
            }

            Entity* CreateMockEntity(const EntityId& id, Component* optional = nullptr)
            {
                using namespace testing;

                auto mock = AZStd::make_unique<NiceMock<MockEntity>>();
                mock->SetId(EntityId(id));
                if (optional)
                {
                    mock->AddComponent(optional); // entity owns the component
                }
                ON_CALL(*mock, Init())
                    .WillByDefault(Invoke(mock.get(), &MockEntity::Base_Init));
                mock->Init();

                ON_CALL(*mock, Activate())
                    .WillByDefault(Invoke(mock.get(), &MockEntity::Base_Activate));

                ON_CALL(*mock, Deactivate())
                    .WillByDefault(Invoke(mock.get(), &MockEntity::Base_Deactivate));

                return mock.release();
            }

            AZStd::unique_ptr<MockSliceReference> m_mockSliceRef;
            AZStd::unique_ptr<MockSliceInstance> m_mockSliceInstance;
        };

        AZStd::unique_ptr<InstantiateMockSlice> m_slice;

        void CreateMockSlice()
        {
            m_slice = AZStd::make_unique<InstantiateMockSlice>(this);
        }
    };

    TEST_F(ExtendedBindingWithSlicesTest, ActiveSlice_EntitiesThatWerentBounded_StayDeactivated)
    {
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId;

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
        CreateMockSlice();

        MockEntity* mock1 = static_cast<MockEntity*>(m_componentApplication->FindEntity(k_fakeEntityId_One));
        EXPECT_CALL(*mock1, Activate()).
            Times(1);

        MockEntity* mock2 = static_cast<MockEntity*>(m_componentApplication->FindEntity(k_fakeEntityId_Two));
        EXPECT_CALL(*mock2, Activate()).
            Times(0);

        EXPECT_CALL(*m_gameEntityMock, DestroyGameEntity(k_fakeEntityId_Two))
            .Times(0);

        // Now it should time out the slice handler and the second entity should remain deactivated since we didn't give binding request for it
        EBUS_EVENT(AZ::TickBus, OnTick, k_wayOverSliceTimeout, AZ::ScriptTimePoint());

    }

    TEST_F(ExtendedBindingWithSlicesTest, ActiveSlice_SpawnSecondEntity_AfterLongDelay_InSameSlicenInstance)
    {
        EXPECT_CALL(*m_gameEntityMock, DestroyGameEntity(k_fakeEntityId_Two))
            .Times(0);

        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId;

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
        CreateMockSlice();

        MockEntity* mock2 = static_cast<MockEntity*>(m_componentApplication->FindEntity(k_fakeEntityId_Two));
        EXPECT_CALL(*mock2, Activate()).
            Times(0);

        // This should not trigger removal of the second entity yet
        auto halfTimeoutInSeconds = AZStd::chrono::seconds(NetBindingSystemImpl::s_sliceBindingTimeout).count() * 10.f;
        EBUS_EVENT(AZ::TickBus, OnTick, halfTimeoutInSeconds, AZ::ScriptTimePoint());

        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_Two;
            spawnContext.m_staticEntityId = k_fakeEntityId_Two;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId; // both mock entities come from the same slice

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_Two, spawnContext);
        }

        EXPECT_CALL(*mock2, Activate()).
            Times(1);

        // This should give net binding system time to bind the second entity
        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());

        // Let the slice timeout, this should lead to no destruction since both entities ought to have been bound by now
        EBUS_EVENT(AZ::TickBus, OnTick, k_wayOverSliceTimeout, AZ::ScriptTimePoint());
    }

    TEST_F(ExtendedBindingWithSlicesTest, ActiveSlice_DespawnLastEntity_DespawnWholeSliceAfterTimeout)
    {
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId;

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
        CreateMockSlice();

        MockEntity* mock1 = static_cast<MockEntity*>(m_componentApplication->FindEntity(k_fakeEntityId_One));
        EXPECT_CALL(*mock1, Activate()).
            Times(1);

        // This should not trigger removal of the second entity yet
        auto halfTimeoutInSeconds = AZStd::chrono::seconds(NetBindingSystemImpl::s_sliceBindingTimeout).count() * 10.f;
        EBUS_EVENT(AZ::TickBus, OnTick, halfTimeoutInSeconds, AZ::ScriptTimePoint());

        EXPECT_CALL(*mock1, Deactivate()).
            Times(1);
        EBUS_EVENT(NetBindingSystemBus, UnbindGameEntity, k_fakeEntityId_One, k_fakeSliceInstanceId);

        EXPECT_CALL(*m_gameEntityMock, DestroyGameEntity(k_fakeEntityId_One))
            .Times(1);
        EXPECT_CALL(*m_gameEntityMock, DestroyGameEntity(k_fakeEntityId_Two))
            .Times(1);

        EBUS_EVENT(AZ::TickBus, OnTick, k_wayOverSliceTimeout, AZ::ScriptTimePoint());
    }

    TEST_F(ExtendedBindingWithSlicesTest, ActiveSlice_DespawnLastEntityBeforeSliceInstantiation_DespawnWholeSlice)
    {
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId;

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());

        EBUS_EVENT(NetBindingSystemBus, UnbindGameEntity, k_fakeEntityId_One, k_fakeSliceInstanceId);

        CreateMockSlice();

        MockEntity* mock1 = static_cast<MockEntity*>(m_componentApplication->FindEntity(k_fakeEntityId_One));
        EXPECT_CALL(*mock1, Activate()).
            Times(0);

        auto halfTimeoutInSeconds = AZStd::chrono::seconds(NetBindingSystemImpl::s_sliceBindingTimeout).count() * 10.f;
        EBUS_EVENT(AZ::TickBus, OnTick, halfTimeoutInSeconds, AZ::ScriptTimePoint());

        EBUS_EVENT(AZ::TickBus, OnTick, k_wayOverSliceTimeout, AZ::ScriptTimePoint());
    }

    TEST_F(ExtendedBindingWithSlicesTest, ActiveSlice_ReuseEntity)
    {
        EXPECT_CALL(*m_gameEntityMock, DestroyGameEntity(k_fakeEntityId_Two))
            .Times(0);

        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId;

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_Two;
            spawnContext.m_staticEntityId = k_fakeEntityId_Two;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId; // both mock entities come from the same slice

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_Two, spawnContext);
        }

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
        CreateMockSlice();

        MockEntity* mock2 = static_cast<MockEntity*>(m_componentApplication->FindEntity(k_fakeEntityId_Two));
        EXPECT_CALL(*mock2, Activate()).
            Times(1);

        EBUS_EVENT(AZ::TickBus, OnTick, k_wayOverSliceTimeout, AZ::ScriptTimePoint());

        EXPECT_CALL(*mock2, Deactivate()).
            Times(1);

        // some time later the second entity goes away and comes back
        EBUS_EVENT(NetBindingSystemBus, UnbindGameEntity, k_fakeEntityId_Two, k_fakeSliceInstanceId);
        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());

        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_Two;
            spawnContext.m_staticEntityId = k_fakeEntityId_Two;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId; // both mock entities come from the same slice

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_Two, spawnContext);
        }

        // The same entity should be activated for the second time
        EXPECT_CALL(*mock2, Activate()).
            Times(1); // Note, Google Mock treats each expect_call separately and satisfies them separately. That's why it's 1 here, despite being a second call.

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
    }

    TEST_F(ExtendedBindingWithSlicesTest, SliceFailedToSpawn)
    {
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId;

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }
        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());

        EBUS_EVENT_ID(m_sliceTicket, SliceInstantiationResultBus, OnSliceInstantiationFailed, k_fakeAssetId);

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());

        EXPECT_TRUE(m_componentApplication->FindEntity(k_fakeEntityId_One) == nullptr);
    }

    TEST_F(ExtendedBindingWithSlicesTest, SliceSpawned_AfterTimeout)
    {
        {
            NetBindingSliceContext spawnContext;
            spawnContext.m_contextSequence = k_fakeContextSeq;
            spawnContext.m_sliceAssetId = k_fakeAssetId;
            spawnContext.m_runtimeEntityId = k_fakeEntityId_One;
            spawnContext.m_staticEntityId = k_fakeEntityId_One;
            spawnContext.m_sliceInstanceId = k_fakeSliceInstanceId;

            EBUS_EVENT(NetBindingSystemBus, SpawnEntityFromSlice, k_repId_One, spawnContext);
        }

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
        EBUS_EVENT(AZ::TickBus, OnTick, k_wayOverSliceTimeout, AZ::ScriptTimePoint());

        CreateMockSlice();

        MockEntity* mock1 = static_cast<MockEntity*>(m_componentApplication->FindEntity(k_fakeEntityId_One));
        EXPECT_CALL(*mock1, Activate()).
            Times(1);

        EBUS_EVENT(AZ::TickBus, OnTick, k_smallStep, AZ::ScriptTimePoint());
    }
}
