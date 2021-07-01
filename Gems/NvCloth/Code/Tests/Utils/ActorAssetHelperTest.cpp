/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Utils/MeshAssetHelper.h>

#include <UnitTestHelper.h>
#include <ActorHelper.h>
#include <Integration/Components/ActorComponent.h>

namespace UnitTest
{
    //! Fixture to setup entity with actor component and the tests data.
    class NvClothMeshAssetHelper
        : public ::testing::Test
    {
    public:
        const AZStd::string RootNodeName = "root_node";
        const AZStd::string MeshNode1Name = "cloth_node_1";
        const AZStd::string MeshNode2Name = "cloth_node_2";
        const AZStd::string OtherNodeName = "other_node";

        const AZStd::vector<AZ::Vector3> MeshVertices = {{
            AZ::Vector3(-1.0f, 0.0f, 0.0f),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f)
        }};
        
        const AZStd::vector<NvCloth::SimIndexType> MeshIndices = {{
            0, 1, 2
        }};
        
        const AZStd::vector<VertexSkinInfluences> MeshSkinningInfo = {{
            VertexSkinInfluences{ SkinInfluence(1, 1.0f) },
            VertexSkinInfluences{ SkinInfluence(1, 1.0f) },
            VertexSkinInfluences{ SkinInfluence(1, 1.0f) }
        }};
        
        const AZStd::vector<AZ::Vector2> MeshUVs = {{
            AZ::Vector2(0.0f, 0.0f),
            AZ::Vector2(1.0f, 0.0f),
            AZ::Vector2(0.5f, 1.0f)
        }};
        
        // [inverse mass, motion constrain radius, backstop offset, backstop radius]
        const AZStd::vector<AZ::Color> MeshClothData = {{
            AZ::Color(0.75f, 0.6f, 0.5f, 0.1f),
            AZ::Color(1.0f, 0.16f, 0.1f, 1.0f),
            AZ::Color(0.25f, 1.0f, 0.9f, 0.5f)
        }};

        const AZ::u32 LodLevel = 0;

    protected:
        // ::testing::Test overrides ...
        void SetUp() override;
        void TearDown() override;

        EMotionFX::Integration::ActorComponent* m_actorComponent = nullptr;

    private:
        AZStd::unique_ptr<AZ::Entity> m_entity;
    };

    void NvClothMeshAssetHelper::SetUp()
    {
        m_entity = AZStd::make_unique<AZ::Entity>();
        m_entity->CreateComponent<AzFramework::TransformComponent>();
        m_actorComponent = m_entity->CreateComponent<EMotionFX::Integration::ActorComponent>();
        m_entity->Init();
        m_entity->Activate();
    }

    void NvClothMeshAssetHelper::TearDown()
    {
        m_entity->Deactivate();
        m_actorComponent = nullptr;
        m_entity.reset();
    }

    TEST_F(NvClothMeshAssetHelper, MeshAssetHelper_CreateAssetHelperWithInvalidEntityId_ReturnsNull)
    {
        AZ::EntityId entityId;

        AZStd::unique_ptr<NvCloth::AssetHelper> assetHelper = NvCloth::AssetHelper::CreateAssetHelper(entityId);

        EXPECT_TRUE(assetHelper.get() == nullptr);
    }

    TEST_F(NvClothMeshAssetHelper, MeshAssetHelper_CreateAssetHelperWithValidEntityId_ReturnsValidMeshAssetHelper)
    {
        AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>();

        AZStd::unique_ptr<NvCloth::AssetHelper> assetHelper = NvCloth::AssetHelper::CreateAssetHelper(entity->GetId());

        EXPECT_TRUE(assetHelper.get() != nullptr);
        EXPECT_TRUE(azrtti_cast<NvCloth::MeshAssetHelper*>(assetHelper.get()) != nullptr);
    }

    TEST_F(NvClothMeshAssetHelper, MeshAssetHelper_CreateAssetHelperWithActor_ReturnsValidMeshAssetHelper)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::AssetHelper> assetHelper = NvCloth::AssetHelper::CreateAssetHelper(m_actorComponent->GetEntityId());

        EXPECT_TRUE(assetHelper.get() != nullptr);
        EXPECT_TRUE(azrtti_cast<NvCloth::MeshAssetHelper*>(assetHelper.get()) != nullptr);
    }

    TEST_F(NvClothMeshAssetHelper, MeshAssetHelper_GatherClothMeshNodesWithEmptyActor_ReturnsEmptyInfo)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::AssetHelper> assetHelper = NvCloth::AssetHelper::CreateAssetHelper(m_actorComponent->GetEntityId());

        NvCloth::MeshNodeList meshNodes;
        assetHelper->GatherClothMeshNodes(meshNodes);

        EXPECT_TRUE(meshNodes.empty());
    }

    TEST_F(NvClothMeshAssetHelper, MeshAssetHelper_ObtainClothMeshNodeInfoWithEmptyActor_ReturnsFalse)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::AssetHelper> assetHelper = NvCloth::AssetHelper::CreateAssetHelper(m_actorComponent->GetEntityId());

        NvCloth::MeshNodeInfo meshNodeInfo;
        NvCloth::MeshClothInfo meshClothInfo;
        bool infoObtained = assetHelper->ObtainClothMeshNodeInfo("", meshNodeInfo, meshClothInfo);

        EXPECT_FALSE(infoObtained);
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothMeshAssetHelper, DISABLED_MeshAssetHelper_GatherClothMeshNodesWithActor_ReturnsCorrectMeshNodeList)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(RootNodeName);
            auto meshNode1Index = actor->AddJoint(MeshNode1Name, AZ::Transform::CreateTranslation(AZ::Vector3(3.0f, -2.0f, 0.0f)), RootNodeName);
            auto otherNodeIndex = actor->AddJoint(OtherNodeName, AZ::Transform::CreateTranslation(AZ::Vector3(0.5f, 0.0f, 0.0f)), RootNodeName);
            auto meshNode2Index = actor->AddJoint(MeshNode2Name, AZ::Transform::CreateTranslation(AZ::Vector3(0.2f, 0.6f, 1.0f)), OtherNodeName);
            actor->SetMesh(LodLevel, meshNode1Index, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->SetMesh(LodLevel, otherNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices));
            actor->SetMesh(LodLevel, meshNode2Index, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::AssetHelper> assetHelper = NvCloth::AssetHelper::CreateAssetHelper(m_actorComponent->GetEntityId());

        NvCloth::MeshNodeList meshNodes;
        assetHelper->GatherClothMeshNodes(meshNodes);

        ASSERT_EQ(meshNodes.size(), 2);
        EXPECT_TRUE(meshNodes[0] == MeshNode1Name);
        EXPECT_TRUE(meshNodes[1] == MeshNode2Name);
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothMeshAssetHelper, DISABLED_MeshAssetHelper_ObtainClothMeshNodeInfoWithActor_ReturnsCorrectClothInfo)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(RootNodeName);
            auto meshNode1Index = actor->AddJoint(MeshNode1Name, AZ::Transform::CreateTranslation(AZ::Vector3(3.0f, -2.0f, 0.0f)), RootNodeName);
            auto otherNodeIndex = actor->AddJoint(OtherNodeName, AZ::Transform::CreateTranslation(AZ::Vector3(0.5f, 0.0f, 0.0f)), RootNodeName);
            auto meshNode2Index = actor->AddJoint(MeshNode2Name, AZ::Transform::CreateTranslation(AZ::Vector3(0.2f, 0.6f, 1.0f)), OtherNodeName);
            actor->SetMesh(LodLevel, meshNode1Index, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->SetMesh(LodLevel, otherNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices));
            actor->SetMesh(LodLevel, meshNode2Index, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::AssetHelper> assetHelper = NvCloth::AssetHelper::CreateAssetHelper(m_actorComponent->GetEntityId());

        NvCloth::MeshNodeInfo meshNodeInfo;
        NvCloth::MeshClothInfo meshClothInfo;
        bool infoObtained = assetHelper->ObtainClothMeshNodeInfo(MeshNode2Name, meshNodeInfo, meshClothInfo);

        EXPECT_TRUE(infoObtained);

        EXPECT_EQ(meshNodeInfo.m_lodLevel, LodLevel);
        ASSERT_EQ(meshNodeInfo.m_subMeshes.size(), 1);
        EXPECT_EQ(meshNodeInfo.m_subMeshes[0].m_primitiveIndex, 2);
        EXPECT_EQ(meshNodeInfo.m_subMeshes[0].m_verticesFirstIndex, 0);
        EXPECT_EQ(meshNodeInfo.m_subMeshes[0].m_numVertices, MeshVertices.size());
        EXPECT_EQ(meshNodeInfo.m_subMeshes[0].m_indicesFirstIndex, 0);
        EXPECT_EQ(meshNodeInfo.m_subMeshes[0].m_numIndices, MeshIndices.size());

        EXPECT_EQ(meshClothInfo.m_particles.size(), MeshVertices.size());
        EXPECT_EQ(meshClothInfo.m_particles.size(), MeshClothData.size());
        for (size_t i = 0; i < meshClothInfo.m_particles.size(); ++i)
        {
            EXPECT_THAT(meshClothInfo.m_particles[i].GetAsVector3(), IsCloseTolerance(MeshVertices[i], Tolerance));
            EXPECT_NEAR(meshClothInfo.m_particles[i].GetW(), MeshClothData[i].GetR(), ToleranceU8);
        }
        EXPECT_EQ(meshClothInfo.m_indices, MeshIndices);
        EXPECT_THAT(meshClothInfo.m_uvs, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), MeshUVs));
        EXPECT_EQ(meshClothInfo.m_motionConstraints.size(), MeshClothData.size());
        for (size_t i = 0; i < meshClothInfo.m_motionConstraints.size(); ++i)
        {
            EXPECT_NEAR(meshClothInfo.m_motionConstraints[i], MeshClothData[i].GetG(), ToleranceU8);
        }
        EXPECT_EQ(meshClothInfo.m_backstopData.size(), MeshClothData.size());
        for (size_t i = 0; i < meshClothInfo.m_backstopData.size(); ++i)
        {
            EXPECT_THAT(meshClothInfo.m_backstopData[i], IsCloseTolerance(AZ::Vector2(MeshClothData[i].GetB() * 2.0f - 1.0f, MeshClothData[i].GetA()), ToleranceU8));
        }
    }
} // namespace UnitTest
