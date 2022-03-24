/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/SystemBus.h>

#include <Blast/BlastSystemBus.h>
#include <Mocks/BlastMocks.h>

#include <NvBlastExtPxManager.h>

using testing::_;
using testing::Eq;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;

namespace Blast
{
    class BlastFamilyTest
        : public testing::Test
        , public FastScopedAllocatorsBase
    {
    protected:
        void SetUp() override
        {
            NvBlastActorDesc actorDesc{1, nullptr, 1, nullptr};

            m_fakeActorFactory = AZStd::make_shared<FakeActorFactory>(3);
            m_fakeEntityProvider = AZStd::make_shared<FakeEntityProvider>(3);
            m_mockPxAsset = AZStd::make_unique<FakeExtPxAsset>(actorDesc);
            m_asset = AZStd::make_unique<BlastAsset>(m_mockPxAsset.get());
            m_blastMaterial = AZStd::make_unique<Material>(MaterialConfiguration());

            m_systemHandler = AZStd::make_shared<MockBlastSystemBusHandler>();
            m_mockTkFramework = AZStd::make_unique<MockTkFramework>();
            m_mockTkFamily = AZStd::make_unique<MockTkFamily>();
            m_mockTkAsset = AZStd::make_unique<MockTkAsset>();
            m_mockListener = AZStd::make_unique<MockBlastListener>();
        }

        void TearDown() override
        {
            m_fakeActorFactory = nullptr;
            m_systemHandler = nullptr;
        }

        AZStd::shared_ptr<FakeActorFactory> m_fakeActorFactory;
        AZStd::shared_ptr<FakeEntityProvider> m_fakeEntityProvider;
        AZStd::unique_ptr<FakeExtPxAsset> m_mockPxAsset;
        AZStd::unique_ptr<BlastAsset> m_asset;
        AZStd::unique_ptr<Material> m_blastMaterial;

        AZStd::shared_ptr<MockBlastSystemBusHandler> m_systemHandler;
        AZStd::unique_ptr<MockTkFramework> m_mockTkFramework;
        AZStd::unique_ptr<MockTkFamily> m_mockTkFamily;
        AZStd::unique_ptr<MockTkAsset> m_mockTkAsset;
        AZStd::unique_ptr<MockBlastListener> m_mockListener;
    };

    TEST_F(BlastFamilyTest, FamilySpawnsAndDespawns_SUITE_sandbox)
    {
        AZStd::unique_ptr<BlastFamily> blastFamily;
        BlastActorConfiguration blastActorConfiguration;

        // BlastFamily::Create expectations
        {
            EXPECT_CALL(*m_systemHandler, GetTkFramework()).Times(1).WillOnce(Return(m_mockTkFramework.get()));
            EXPECT_CALL(*m_mockPxAsset, getTkAsset()).Times(1).WillOnce(testing::ReturnRef(*m_mockTkAsset.get()));
            EXPECT_CALL(*m_mockTkFramework, createActor(_))
                .Times(1)
                .WillOnce(Return(&m_fakeActorFactory->m_mockActors[0]->GetTkActor()));
            EXPECT_CALL(*m_fakeActorFactory->m_mockActors[0]->m_tkActor, getFamily())
                .Times(1)
                .WillOnce(ReturnRef(*m_mockTkFamily.get()));
            EXPECT_CALL(*m_fakeActorFactory, CalculateComponents(_))
                .Times(3)
                .WillRepeatedly(Return(AZStd::vector<AZ::Uuid>()));

            BlastFamilyDesc familyDesc
                {*m_asset,
                 m_mockListener.get(),
                 nullptr,
                 Physics::MaterialId(),
                 m_blastMaterial.get(),
                 m_fakeActorFactory,
                 m_fakeEntityProvider,
                 blastActorConfiguration};

            blastFamily = BlastFamily::Create(familyDesc);
        }

        // BlastFamily::Spawn expectations
        {
            int actorCount = 1;
            EXPECT_CALL(*m_mockTkFamily, getActorCount()).Times(1).WillOnce(Return(actorCount));
            EXPECT_CALL(*m_mockTkFamily, getActors(_, Eq(actorCount), _))
                .Times(1)
                .WillOnce(testing::Invoke(
                    [this](auto buffer, auto actorCount, auto indexStart)
                    {
                        buffer[indexStart] = m_fakeActorFactory->m_mockActors[0]->m_tkActor.get();
                        return actorCount;
                    }));
            EXPECT_CALL(*m_mockListener, OnActorCreated(Ref(*blastFamily), Ref(*m_fakeActorFactory->m_mockActors[0])))
                .Times(1);
            EXPECT_CALL(*m_mockTkFamily, addListener(_)).Times(1);
            EXPECT_CALL(
                *m_fakeActorFactory,
                CalculateVisibleChunks(
                    Ref<BlastFamily&>(*blastFamily),
                    Ref<Nv::Blast::TkActor&>(*m_fakeActorFactory->m_mockActors[0]->m_tkActor)))
                .Times(1);
            EXPECT_CALL(
                *m_fakeActorFactory,
                CalculateIsStatic(Ref(*blastFamily), Ref(*m_fakeActorFactory->m_mockActors[0]->m_tkActor), _))
                .Times(1)
                .WillOnce(Return(false));
            EXPECT_CALL(
                *m_fakeActorFactory, CalculateIsLeafChunk(Ref(*m_fakeActorFactory->m_mockActors[0]->m_tkActor), _))
                .Times(1)
                .WillOnce(Return(false));

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            blastFamily->Spawn(transform);
        }

        // BlastFamily::HandleEvents
        {
            Nv::Blast::TkSplitEvent splitEvent;
            AZStd::vector<Nv::Blast::TkActor*> children(2);
            children[0] = m_fakeActorFactory->m_mockActors[1]->m_tkActor.get();
            children[1] = m_fakeActorFactory->m_mockActors[2]->m_tkActor.get();
            splitEvent.children = children.data();
            splitEvent.numChildren = 2;
            splitEvent.parentData = {
                m_mockTkFamily.get(), reinterpret_cast<void*>(m_fakeActorFactory->m_mockActors[0]), 0};
            Nv::Blast::TkEvent tkEvent;
            tkEvent.type = Nv::Blast::TkEvent::Split;
            tkEvent.payload = reinterpret_cast<void*>(&splitEvent);

            EXPECT_CALL(*m_fakeActorFactory, CalculateVisibleChunks(Ref<BlastFamily&>(*blastFamily), _)).Times(2);
            EXPECT_CALL(*m_fakeActorFactory, CalculateIsStatic(Ref(*blastFamily), _, _))
                .Times(2)
                .WillRepeatedly(Return(false));
            EXPECT_CALL(*m_fakeActorFactory, CalculateIsLeafChunk(_, _)).Times(2).WillRepeatedly(Return(false));

            EXPECT_CALL(*m_mockListener, OnActorDestroyed(_, _)).Times(1);
            EXPECT_CALL(*m_mockListener, OnActorCreated(_, _)).Times(2);

            blastFamily->HandleEvents(&tkEvent, 1);
        }

        // BlastFamily::~BlastFamily
        {
            EXPECT_CALL(*m_mockTkFamily, removeListener(_)).Times(1);
            EXPECT_CALL(*m_mockListener, OnActorDestroyed(_, _)).Times(2);

            blastFamily.reset();
        }
    }
} // namespace Blast
