/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StdAfx.h>

#include <gmock/gmock.h>
#include <gtest/gtest_prod.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzTest/AzTest.h>
#include <Blast/BlastSystemBus.h>
#include <Family/ActorRenderManager.h>
#include <MockMeshFeatureProcessor.h>
#include <Mocks/BlastMocks.h>

using testing::_;
using testing::Eq;
using testing::NaggyMock;
using testing::Return;
using testing::StrictMock;

namespace Blast
{
    // Helper class to make it possible to make test a friend without polluting the tested class itself
    class TestableActorRenderManager : public ActorRenderManager
    {
    public:
        TestableActorRenderManager(
            AZ::Render::MeshFeatureProcessorInterface* meshFeatureProcessor, BlastMeshData* meshData,
            AZ::EntityId entityId, uint32_t chunkCount, AZ::Vector3 scale)
            : ActorRenderManager(meshFeatureProcessor, meshData, entityId, chunkCount, scale)
        {
        }

        FRIEND_TEST(ActorRenderManagerTest, KeepsTrackOfAndRendersChunks_SUITE_sandbox);
    };

    class ActorRenderManagerTest
        : public testing::Test
        , public FastScopedAllocatorsBase
    {
    protected:
        void SetUp() override
        {
            m_chunkCount = 2;
            m_mockMeshData = AZStd::make_shared<MockBlastMeshData>();
            m_mockMeshFeatureProcessor = AZStd::make_shared<UnitTest::MockMeshFeatureProcessor>();
            m_actorFactory = AZStd::make_unique<FakeActorFactory>(1);
            m_fakeEntityProvider = AZStd::make_shared<FakeEntityProvider>(m_chunkCount);

            m_actorFactory->m_mockActors[0]->m_chunkIndices = {0, 1};
        }

        AZStd::shared_ptr<MockBlastMeshData> m_mockMeshData;
        AZStd::shared_ptr<UnitTest::MockMeshFeatureProcessor> m_mockMeshFeatureProcessor;
        AZStd::unique_ptr<FakeActorFactory> m_actorFactory;
        AZStd::shared_ptr<FakeEntityProvider> m_fakeEntityProvider;

        uint32_t m_chunkCount;
    };

    TEST_F(ActorRenderManagerTest, KeepsTrackOfAndRendersChunks_SUITE_sandbox)
    {
        MockTransformBusHandler transformHandler;

        for (const auto id : m_fakeEntityProvider->m_createdEntityIds)
        {
            transformHandler.Connect(id);
        }
        AZStd::unique_ptr<TestableActorRenderManager> actorRenderManager;

        AZ::EntityId entityId;

        // ActorRenderManager::ActorRenderManager
        {
            AZ::Data::Asset<AZ::RPI::ModelAsset> asset{AZ::Data::AssetLoadBehavior::NoLoad};
            EXPECT_CALL(*m_mockMeshData, GetMeshAsset(_)).Times(m_chunkCount).WillRepeatedly(testing::ReturnRef(asset));

            actorRenderManager = AZStd::make_unique<TestableActorRenderManager>(
                m_mockMeshFeatureProcessor.get(), m_mockMeshData.get(), entityId, m_chunkCount,
                AZ::Vector3::CreateOne());
            EXPECT_EQ(actorRenderManager->m_chunkMeshHandles.size(), m_chunkCount);
        }

        // ActorRenderManager::OnActorCreated
        {
            EXPECT_CALL(
                *m_mockMeshFeatureProcessor, AcquireMesh(_, testing::A<const AZ::Render::MaterialAssignmentMap&>()))
                .Times(aznumeric_cast<int>(m_actorFactory->m_mockActors[0]->GetChunkIndices().size()))
                .WillOnce(Return(testing::ByMove(AZ::Render::MeshFeatureProcessorInterface::MeshHandle())))
                .WillOnce(Return(testing::ByMove(AZ::Render::MeshFeatureProcessorInterface::MeshHandle())));

            actorRenderManager->OnActorCreated(*m_actorFactory->m_mockActors[0]);
            for (auto chunkId : m_actorFactory->m_mockActors[0]->m_chunkIndices)
            {
                EXPECT_EQ(
                    actorRenderManager->m_chunkActors[chunkId],
                    static_cast<BlastActor*>(m_actorFactory->m_mockActors[0]));
            }
        }

        // ActorRenderManager::SyncMeshes
        {
            EXPECT_CALL(*m_mockMeshFeatureProcessor, SetTransform(_, _, _))
                .Times(aznumeric_cast<int>(m_actorFactory->m_mockActors[0]->GetChunkIndices().size()));
            actorRenderManager->SyncMeshes();
        }

        // ActorRenderManager::OnActorDestroyed
        {
            EXPECT_CALL(*m_mockMeshFeatureProcessor, ReleaseMesh(_))
                .Times(aznumeric_cast<int>(m_actorFactory->m_mockActors[0]->GetChunkIndices().size()))
                .WillRepeatedly(Return(true));

            actorRenderManager->OnActorDestroyed(*m_actorFactory->m_mockActors[0]);
            for (auto chunkId : m_actorFactory->m_mockActors[0]->m_chunkIndices)
            {
                EXPECT_EQ(actorRenderManager->m_chunkActors[chunkId], nullptr);
            }
        }
    }
} // namespace Blast
