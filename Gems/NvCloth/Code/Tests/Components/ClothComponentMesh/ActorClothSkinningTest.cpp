/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Components/ClothComponentMesh/ActorClothSkinning.h>
#include <Utils/AssetHelper.h>

#include <UnitTestHelper.h>
#include <ActorHelper.h>
#include <Integration/Components/ActorComponent.h>

namespace UnitTest
{
    //! Fixture to setup entity with actor component and the tests data.
    class NvClothActorClothSkinning
        : public ::testing::Test
    {
    public:
        const AZStd::string MeshNodeName = "cloth_mesh_node";

        const AZStd::vector<AZ::Vector3> MeshVertices = {{
            AZ::Vector3(-1.0f, 0.0f, 0.0f),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f)
        }};

        const AZStd::vector<NvCloth::SimParticleFormat> MeshParticles = {{
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(MeshVertices[0], 1.0f),
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(MeshVertices[1], 1.0f),
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(MeshVertices[2], 1.0f),
        }};
        
        const AZStd::vector<NvCloth::SimIndexType> MeshIndices = {{
            0, 1, 2
        }};
        
        const AZStd::vector<VertexSkinInfluences> MeshSkinningInfo = {{
            { {0, 1.0f} },
            { {0, 1.0f} },
            { {0, 1.0f} },
        }};

        const AZStd::vector<int> MeshRemappedVertices = {{
            0, 1, 2
        }};

        const AZ::u32 LodLevel = 0;

        const NvCloth::MeshNodeInfo MeshNodeInfo = {
            static_cast<int>(LodLevel),
            {{
                // One SubMesh
                {
                    0, // Primitive index
                    0, // First vertex
                    static_cast<int>(MeshVertices.size()), // Vertex count
                    0, // First index
                    static_cast<int>(MeshIndices.size()) // Index count
                }
            }}
        };

    protected:
        // ::testing::Test overrides ...
        void SetUp() override;
        void TearDown() override;

        EMotionFX::Integration::ActorComponent* m_actorComponent = nullptr;

    private:
        AZStd::unique_ptr<AZ::Entity> m_entity;
    };

    void NvClothActorClothSkinning::SetUp()
    {
        m_entity = AZStd::make_unique<AZ::Entity>();
        m_entity->CreateComponent<AzFramework::TransformComponent>();
        m_actorComponent = m_entity->CreateComponent<EMotionFX::Integration::ActorComponent>();
        m_entity->Init();
        m_entity->Activate();
    }

    void NvClothActorClothSkinning::TearDown()
    {
        m_entity->Deactivate();
        m_actorComponent = nullptr;
        m_entity.reset();
    }

    TEST_F(NvClothActorClothSkinning, ActorClothSkinning_CreateWithNoData_ReturnsNull)
    {
        AZ::EntityId entityId;
        AZStd::unique_ptr<NvCloth::ActorClothSkinning> actorClothSkinning =
            NvCloth::ActorClothSkinning::Create(entityId, {}, {}, 0, {});

        EXPECT_TRUE(actorClothSkinning.get() == nullptr);
    }

    TEST_F(NvClothActorClothSkinning, ActorClothSkinning_CreateWithDataButWithInvalidEntityId_ReturnsNull)
    {
        AZ::EntityId entityId;
        AZStd::unique_ptr<NvCloth::ActorClothSkinning> actorClothSkinning =
            NvCloth::ActorClothSkinning::Create(entityId, MeshNodeInfo, MeshParticles, MeshParticles.size(), MeshRemappedVertices);

        EXPECT_TRUE(actorClothSkinning.get() == nullptr);
    }

    TEST_F(NvClothActorClothSkinning, ActorClothSkinning_CreateWithEmptyActor_ReturnsNull)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothSkinning> actorClothSkinning =
            NvCloth::ActorClothSkinning::Create(m_actorComponent->GetEntityId(), {}, {}, 0, {});

        EXPECT_TRUE(actorClothSkinning.get() == nullptr);
    }

    TEST_F(NvClothActorClothSkinning, ActorClothSkinning_CreateWithActorWhoseMeshHasNoSkinningInfo_ReturnsNull)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothSkinning> actorClothSkinning =
            NvCloth::ActorClothSkinning::Create(m_actorComponent->GetEntityId(), MeshNodeInfo, MeshParticles, MeshParticles.size(), MeshRemappedVertices);

        EXPECT_TRUE(actorClothSkinning.get() == nullptr);
    }

    TEST_F(NvClothActorClothSkinning, DISABLED_ActorClothSkinning_CreateWithActor_ReturnsValidInstance)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothSkinning> actorClothSkinning =
            NvCloth::ActorClothSkinning::Create(m_actorComponent->GetEntityId(), MeshNodeInfo, MeshParticles, MeshParticles.size(), MeshRemappedVertices);

        EXPECT_TRUE(actorClothSkinning.get() != nullptr);
    }

    TEST_F(NvClothActorClothSkinning, DISABLED_ActorClothSkinning_UpdateAndApplyLinearSkinning_ModifiesVertices)
    {
        const AZ::Transform meshNodeTransform = AZ::Transform::CreateRotationY(AZ::DegToRad(90.0f));

        EMotionFX::Integration::ActorComponent::Configuration actorConfig;
        actorConfig.m_skinningMethod = EMotionFX::Integration::SkinningMethod::Linear;

        auto entity = AZStd::make_unique<AZ::Entity>();
        entity->CreateComponent<AzFramework::TransformComponent>();
        auto* actorComponent = entity->CreateComponent<EMotionFX::Integration::ActorComponent>(&actorConfig);
        entity->Init();
        entity->Activate();

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName, meshNodeTransform);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo));
            actor->FinishSetup();

            actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothSkinning> actorClothSkinning =
            NvCloth::ActorClothSkinning::Create(actorComponent->GetEntityId(), MeshNodeInfo, MeshParticles, MeshParticles.size(), MeshRemappedVertices);
        ASSERT_TRUE(actorClothSkinning.get() != nullptr);

        AZStd::vector<NvCloth::SimParticleFormat> skinnedClothParticles(MeshParticles.size(), NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f));

        actorClothSkinning->UpdateSkinning();
        actorClothSkinning->ApplySkinning(MeshParticles, skinnedClothParticles);

        EXPECT_THAT(skinnedClothParticles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), MeshParticles));

        // Update actor instance's joints transforms
        const AZ::Transform newMeshNodeTransform = AZ::Transform::CreateRotationY(AZ::DegToRad(180.0f));
        EMotionFX::Pose* currentPose = actorComponent->GetActorInstance()->GetTransformData()->GetCurrentPose();
        currentPose->SetLocalSpaceTransform(0, newMeshNodeTransform);
        actorComponent->GetActorInstance()->UpdateSkinningMatrices();

        AZStd::vector<NvCloth::SimParticleFormat> newSkinnedClothParticles(MeshParticles.size(), NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f));

        actorClothSkinning->UpdateSkinning();
        actorClothSkinning->ApplySkinning(MeshParticles, newSkinnedClothParticles);

        const AZ::Transform diffTransform = AZ::Transform::CreateRotationY(AZ::DegToRad(90.0f));
        const AZStd::vector<NvCloth::SimParticleFormat> clothParticlesResult = {{
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(diffTransform.TransformPoint(MeshVertices[0]), 1.0f),
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(diffTransform.TransformPoint(MeshVertices[1]), 1.0f),
            NvCloth::SimParticleFormat::CreateFromVector3AndFloat(diffTransform.TransformPoint(MeshVertices[2]), 1.0f),
        }};

        EXPECT_THAT(newSkinnedClothParticles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), clothParticlesResult));
    }

    TEST_F(NvClothActorClothSkinning, DISABLED_ActorClothSkinning_UpdateAndApplyDualQuatSkinning_ModifiesVertices)
    {
        const AZStd::string rootNodeName = "root_node";
        const AZStd::string meshNodeName = "cloth_mesh_node";
        
        const AZStd::vector<VertexSkinInfluences> meshSkinningInfo = {{
            { {1, 0.75f}, {0, 0.25f} },
            { {1, 0.75f}, {0, 0.25f} },
            { {1, 0.75f}, {0, 0.25f} },
        }};

        const AZ::Transform rootNodeTransform = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 53.0f, -65.0f));
        const AZ::Transform meshNodeTransform = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationY(AZ::DegToRad(36.0f)), AZ::Vector3(3.0f, -2.3f, 16.0f));

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(rootNodeName, rootNodeTransform);
            auto meshNodeIndex = actor->AddJoint(meshNodeName, meshNodeTransform, rootNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, meshSkinningInfo));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothSkinning> actorClothSkinning =
            NvCloth::ActorClothSkinning::Create(m_actorComponent->GetEntityId(), MeshNodeInfo, MeshParticles, MeshParticles.size(), MeshRemappedVertices);
        ASSERT_TRUE(actorClothSkinning.get() != nullptr);
        
        AZStd::vector<NvCloth::SimParticleFormat> skinnedClothParticles(MeshParticles.size(), NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f));

        actorClothSkinning->UpdateSkinning();
        actorClothSkinning->ApplySkinning(MeshParticles, skinnedClothParticles);

        EXPECT_THAT(skinnedClothParticles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), MeshParticles));

        // Update actor instance's joints transforms
        const AZ::Transform newJointRootTransform = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationZ(AZ::DegToRad(-32.0f)), AZ::Vector3(2.5f, -6.0f, 0.2f));
        const AZ::Transform newJointChildTransform = AZ::Transform::CreateTranslation(AZ::Vector3(-2.0f, 3.0f, 0.0f));
        EMotionFX::Pose* currentPose = m_actorComponent->GetActorInstance()->GetTransformData()->GetCurrentPose();
        currentPose->SetLocalSpaceTransform(0, newJointRootTransform);
        currentPose->SetLocalSpaceTransform(1, newJointChildTransform);
        m_actorComponent->GetActorInstance()->UpdateSkinningMatrices();

        AZStd::vector<NvCloth::SimParticleFormat> newSkinnedClothParticles(MeshParticles.size(), NvCloth::SimParticleFormat(0.0f, 0.0f, 0.0f, 1.0f));

        actorClothSkinning->UpdateSkinning();
        actorClothSkinning->ApplySkinning(MeshParticles, newSkinnedClothParticles);

        const AZStd::vector<NvCloth::SimParticleFormat> clothParticlesResult = {{
            NvCloth::SimParticleFormat(-48.4177f, -31.9446f, 45.2279f, 1.0f),
            NvCloth::SimParticleFormat(-46.9087f, -32.8876f, 46.1409f, 1.0f),
            NvCloth::SimParticleFormat(-47.1333f, -31.568f, 45.6844f, 1.0f),
        }};

        EXPECT_THAT(newSkinnedClothParticles, ::testing::Pointwise(ContainerIsCloseTolerance(Tolerance), clothParticlesResult));
    }

    TEST_F(NvClothActorClothSkinning, DISABLED_ActorClothSkinning_UpdateActorVisibility_ReturnsExpectedValues)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        AZStd::unique_ptr<NvCloth::ActorClothSkinning> actorClothSkinning =
            NvCloth::ActorClothSkinning::Create(m_actorComponent->GetEntityId(), MeshNodeInfo, MeshParticles, MeshParticles.size(), MeshRemappedVertices);
        ASSERT_TRUE(actorClothSkinning.get() != nullptr);

        EXPECT_FALSE(actorClothSkinning->IsActorVisible());
        EXPECT_FALSE(actorClothSkinning->WasActorVisible());

        m_actorComponent->GetActorInstance()->SetIsVisible(true);
        actorClothSkinning->UpdateActorVisibility();

        EXPECT_TRUE(actorClothSkinning->IsActorVisible());
        EXPECT_FALSE(actorClothSkinning->WasActorVisible());

        actorClothSkinning->UpdateActorVisibility();

        EXPECT_TRUE(actorClothSkinning->IsActorVisible());
        EXPECT_TRUE(actorClothSkinning->WasActorVisible());

        m_actorComponent->GetActorInstance()->SetIsVisible(false);
        actorClothSkinning->UpdateActorVisibility();

        EXPECT_FALSE(actorClothSkinning->IsActorVisible());
        EXPECT_TRUE(actorClothSkinning->WasActorVisible());

        actorClothSkinning->UpdateActorVisibility();

        EXPECT_FALSE(actorClothSkinning->IsActorVisible());
        EXPECT_FALSE(actorClothSkinning->WasActorVisible());
    }
} // namespace UnitTest
