/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StdAfx.h>

#include <gmock/gmock.h>

#include <Actor/BlastActorImpl.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Mocks/BlastMocks.h>
#include <NvBlastExtPxAsset.h>

using testing::_;
using testing::Eq;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;

namespace Blast
{
    class TestableBlastActor : public BlastActorImpl
    {
    public:
        TestableBlastActor(const BlastActorDesc& desc)
            : BlastActorImpl(desc)
        {
            Spawn();
        }

    protected:
        Physics::ColliderConfiguration CalculateColliderConfiguration(
            [[maybe_unused]] const AZ::Transform& transform, [[maybe_unused]] Physics::MaterialId material) override
        {
            return Physics::ColliderConfiguration();
        }
    };

    class BlastActorTest
        : public testing::Test
        , public FastScopedAllocatorsBase
    {
    protected:
        void SetUp() override
        {
            m_mockFamily = AZStd::make_unique<FakeBlastFamily>();
            m_mockTkActor = AZStd::make_unique<MockTkActor>();
            m_mockPhysicsSystemRequestsHandler = AZStd::make_unique<MockPhysicsSystemRequestsHandler>();
            m_mockPhysicsDefaultWorldRequestsHandler = AZStd::make_unique<MockPhysicsDefaultWorldRequestsHandler>();
            m_mockRigidBodyRequestBusHandler = AZStd::make_unique<MockRigidBodyRequestBusHandler>();
        }

        void TearDown() override {}

        AZStd::unique_ptr<FakeBlastFamily> m_mockFamily;
        AZStd::unique_ptr<MockTkActor> m_mockTkActor;
        AZStd::unique_ptr<MockTkAsset> m_mockTkAsset;
        AZStd::unique_ptr<MockPhysicsSystemRequestsHandler> m_mockPhysicsSystemRequestsHandler;
        AZStd::unique_ptr<MockPhysicsDefaultWorldRequestsHandler> m_mockPhysicsDefaultWorldRequestsHandler;
        AZStd::unique_ptr<MockRigidBodyRequestBusHandler> m_mockRigidBodyRequestBusHandler;

        AZStd::unique_ptr<BlastActor> m_blastActor;
    };

    TEST_F(BlastActorTest, CreatesShapes_GivenCorrectDesc_SUITE_sandbox)
    {
        // Initialize actor description
        AZStd::vector<uint32_t> chunkIndices;
        chunkIndices.push_back(0);

        AzPhysics::RigidBodyConfiguration configuration;
        auto entity = AZStd::make_shared<AZ::Entity>();
        AZ::EntityId entityId = entity->GetId();
        auto actorDesc = BlastActorDesc
            {m_mockFamily.get(),
             m_mockTkActor.get(),
             Physics::MaterialId(),
             AZ::Vector3::CreateZero(),
             AZ::Vector3::CreateZero(),
             configuration,
             chunkIndices,
             entity,
             false,
             false};

        // Initialize mock asset
        Nv::Blast::ExtPxChunk chunk{0, 1, false};
        Nv::Blast::ExtPxSubchunk subchunk{
            physx::PxTransform(0, 0, 0),
            physx::PxConvexMeshGeometry(nullptr),
        };
        m_mockFamily->m_pxAsset.m_chunks.push_back(chunk);
        m_mockFamily->m_pxAsset.m_subchunks.push_back(subchunk);

        AZStd::shared_ptr<MockShape> mockShape = AZStd::make_shared<MockShape>();
        auto rigidBody = AZStd::make_unique<FakeRigidBody>();

        // Connect mock bus handler
        m_mockRigidBodyRequestBusHandler->Connect(entityId);

        EXPECT_CALL(*m_mockPhysicsSystemRequestsHandler, CreateShape(_, _)).Times(1).WillOnce(Return(mockShape));
        EXPECT_CALL(*m_mockRigidBodyRequestBusHandler, GetRigidBody()).Times(1).WillOnce(Return(rigidBody.get()));

        m_blastActor.reset(aznew TestableBlastActor(actorDesc));
    }
} // namespace Blast
