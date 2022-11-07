/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Components/ClothComponentMesh/ClothComponentMesh.h>

#include <UnitTestHelper.h>
#include <ActorHelper.h>
#include <Integration/Components/ActorComponent.h>

namespace UnitTest
{
    //! Fixture to setup entity with actor component and the tests data.
    class NvClothComponentMesh
        : public ::testing::Test
    {
    public:
        const AZStd::string MeshNodeName = "cloth_node";

        const AZStd::vector<AZ::Vector3> MeshVertices = {{
            AZ::Vector3(-1.0f, 0.0f, 0.0f),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f)
        }};

        const AZStd::vector<NvCloth::SimIndexType> MeshIndices = {{
            0, 1, 2
        }};

        const AZStd::vector<VertexSkinInfluences> MeshSkinningInfo = {{
            VertexSkinInfluences{ SkinInfluence(0, 1.0f) },
            VertexSkinInfluences{ SkinInfluence(0, 1.0f) },
            VertexSkinInfluences{ SkinInfluence(0, 1.0f) }
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

    void NvClothComponentMesh::SetUp()
    {
        m_entity = AZStd::make_unique<AZ::Entity>();
        m_entity->CreateComponent<AzFramework::TransformComponent>();
        m_actorComponent = m_entity->CreateComponent<EMotionFX::Integration::ActorComponent>();
        m_entity->Init();
        m_entity->Activate();
    }

    void NvClothComponentMesh::TearDown()
    {
        m_entity->Deactivate();
        m_actorComponent = nullptr;
        m_entity.reset();
    }

    TEST_F(NvClothComponentMesh, ClothComponentMesh_DefaultConstructor_ReturnsEmptyRenderData)
    {
        AZ::EntityId entityId;
        NvCloth::ClothComponentMesh clothComponentMesh(entityId, {});

        const auto& renderData = clothComponentMesh.GetRenderData();

        EXPECT_TRUE(renderData.m_particles.empty());
        EXPECT_TRUE(renderData.m_tangents.empty());
        EXPECT_TRUE(renderData.m_bitangents.empty());
        EXPECT_TRUE(renderData.m_normals.empty());
    }

    TEST_F(NvClothComponentMesh, ClothComponentMesh_InitWithEmptyActor_ReturnsEmptyRenderData)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), {});

        const auto& renderData = clothComponentMesh.GetRenderData();

        EXPECT_TRUE(renderData.m_particles.empty());
        EXPECT_TRUE(renderData.m_tangents.empty());
        EXPECT_TRUE(renderData.m_bitangents.empty());
        EXPECT_TRUE(renderData.m_normals.empty());
    }

    TEST_F(NvClothComponentMesh, ClothComponentMesh_InitWithActorWithNoMesh_ReturnsEmptyRenderData)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(MeshNodeName);
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        const auto& renderData = clothComponentMesh.GetRenderData();

        EXPECT_TRUE(renderData.m_particles.empty());
        EXPECT_TRUE(renderData.m_tangents.empty());
        EXPECT_TRUE(renderData.m_bitangents.empty());
        EXPECT_TRUE(renderData.m_normals.empty());
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_InitWithEntityActorWithNoClothData_TriggersError)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        AZ_TEST_START_TRACE_SUPPRESSION;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // Expect 1 error
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_InitWithEntityActor_ReturnsValidRenderData)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        const auto& renderData = clothComponentMesh.GetRenderData();

        EXPECT_EQ(renderData.m_particles.size(), MeshVertices.size());
        EXPECT_EQ(renderData.m_particles.size(), MeshClothData.size());
        for (size_t i = 0; i < renderData.m_particles.size(); ++i)
        {
            EXPECT_THAT(renderData.m_particles[i].GetAsVector3(), IsCloseTolerance(MeshVertices[i], Tolerance));
            EXPECT_NEAR(renderData.m_particles[i].GetW(), MeshClothData[i].GetR(), ToleranceU8);
        }

        EXPECT_THAT(renderData.m_tangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisX(), Tolerance)));
        EXPECT_THAT(renderData.m_bitangents, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisY(), Tolerance)));
        EXPECT_THAT(renderData.m_normals, ::testing::Each(IsCloseTolerance(AZ::Vector3::CreateAxisZ(), Tolerance)));
    }

    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_TickClothSystem_RunningSimulationVerticesGoDown)
    {
        {
            const float height = 4.7f;
            const float radius = 1.2f;
            const auto collider = CreateCapsuleCollider(MeshNodeName, height, radius);

            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->AddClothCollider(collider);
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        const AZStd::vector<NvCloth::SimParticleFormat> particlesBefore = clothComponentMesh.GetRenderData().m_particles;

        // Ticking Cloth System updates all its solvers
        for (size_t i = 0; i < 300.0f; ++i)
        {
            const float deltaTimeSim = 1.0f / 60.0f;
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
                deltaTimeSim,
                AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));
        }

        const AZStd::vector<NvCloth::SimParticleFormat> particlesAfter = clothComponentMesh.GetRenderData().m_particles;

        EXPECT_EQ(particlesAfter.size(), particlesBefore.size());
        for (size_t i = 0; i < particlesAfter.size(); ++i)
        {
            EXPECT_LT(particlesAfter[i].GetZ(), particlesBefore[i].GetZ());
        }
    }

    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_UpdateConfigurationInvalidEntity_ReturnEmptyRenderData)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        AZ::EntityId newEntityId;
        clothComponentMesh.UpdateConfiguration(newEntityId, clothConfig);

        const auto& renderData = clothComponentMesh.GetRenderData();

        EXPECT_TRUE(renderData.m_particles.empty());
        EXPECT_TRUE(renderData.m_tangents.empty());
        EXPECT_TRUE(renderData.m_bitangents.empty());
        EXPECT_TRUE(renderData.m_normals.empty());
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_UpdateConfigurationDifferentEntity_ReturnsRenderDataFromNewEntity)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        const AZStd::vector<AZ::Vector3> newMeshVertices = {{
            AZ::Vector3(-2.3f, 0.0f, 0.0f),
            AZ::Vector3(4.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, -1.0f, 0.0f)
        }};

        auto newEntity = AZStd::make_unique<AZ::Entity>();
        newEntity->CreateComponent<AzFramework::TransformComponent>();
        auto* newActorComponent = newEntity->CreateComponent<EMotionFX::Integration::ActorComponent>();
        newEntity->Init();
        newEntity->Activate();
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test2");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(newMeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            newActorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        clothComponentMesh.UpdateConfiguration(newActorComponent->GetEntityId(), clothConfig);

        const auto& renderData = clothComponentMesh.GetRenderData();

        EXPECT_EQ(renderData.m_particles.size(), newMeshVertices.size());
        EXPECT_EQ(renderData.m_particles.size(), MeshClothData.size());
        for (size_t i = 0; i < renderData.m_particles.size(); ++i)
        {
            EXPECT_THAT(renderData.m_particles[i].GetAsVector3(), IsCloseTolerance(newMeshVertices[i], Tolerance));
            EXPECT_NEAR(renderData.m_particles[i].GetW(), MeshClothData[i].GetR(), ToleranceU8);
        }
    }

    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_UpdateConfigurationInvalidMeshNode_ReturnEmptyRenderData)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        clothConfig.m_meshNode = "unknown_cloth_mesh_node";
        clothComponentMesh.UpdateConfiguration(m_actorComponent->GetEntityId(), clothConfig);

        const auto& renderData = clothComponentMesh.GetRenderData();

        EXPECT_TRUE(renderData.m_particles.empty());
        EXPECT_TRUE(renderData.m_tangents.empty());
        EXPECT_TRUE(renderData.m_bitangents.empty());
        EXPECT_TRUE(renderData.m_normals.empty());
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_UpdateConfigurationNewMeshNode_ReturnsRenderDataFromNewMeshNode)
    {
        const AZStd::string meshNode2Name = "cloth_node_2";

        const AZStd::vector<AZ::Vector3> mesh2Vertices = {{
            AZ::Vector3(-2.3f, 0.0f, 0.0f),
            AZ::Vector3(4.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, -1.0f, 0.0f)
        }};

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            auto meshNode2Index = actor->AddJoint(meshNode2Name);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->SetMesh(LodLevel, meshNode2Index, CreateEMotionFXMesh(mesh2Vertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        clothConfig.m_meshNode = meshNode2Name;
        clothComponentMesh.UpdateConfiguration(m_actorComponent->GetEntityId(), clothConfig);

        const auto& renderData = clothComponentMesh.GetRenderData();

        EXPECT_EQ(renderData.m_particles.size(), mesh2Vertices.size());
        EXPECT_EQ(renderData.m_particles.size(), MeshClothData.size());
        for (size_t i = 0; i < renderData.m_particles.size(); ++i)
        {
            EXPECT_THAT(renderData.m_particles[i].GetAsVector3(), IsCloseTolerance(mesh2Vertices[i], Tolerance));
            EXPECT_NEAR(renderData.m_particles[i].GetW(), MeshClothData[i].GetR(), ToleranceU8);
        }
    }

    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_UpdateConfigurationInvertingGravity_RunningSimulationVerticesGoUp)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        const AZStd::vector<NvCloth::SimParticleFormat> particlesBefore = clothComponentMesh.GetRenderData().m_particles;

        clothConfig.m_gravityScale = -1.0f;
        clothComponentMesh.UpdateConfiguration(m_actorComponent->GetEntityId(), clothConfig);

        // Ticking Cloth System updates all its solvers
        for (size_t i = 0; i < 300.0f; ++i)
        {
            const float deltaTimeSim = 1.0f / 60.0f;
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
                deltaTimeSim,
                AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));
        }

        const AZStd::vector<NvCloth::SimParticleFormat> particlesAfter = clothComponentMesh.GetRenderData().m_particles;

        EXPECT_EQ(particlesAfter.size(), particlesBefore.size());
        for (size_t i = 0; i < particlesAfter.size(); ++i)
        {
            EXPECT_GT(particlesAfter[i].GetZ(), particlesBefore[i].GetZ());
        }
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // At the moment, CreateAssetFromActor fills only Actor to the ActorAsset, but not the RenderActor,
    // because of that the AtomModel is not created and OnModelReady is not called.
    TEST_F(NvClothComponentMesh, DISABLED_ClothComponentMesh_ModifyMesh_RenderMeshIsUpdated)
    {
        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(MeshNodeName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, MeshSkinningInfo, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            m_actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        NvCloth::ClothConfiguration clothConfig;
        clothConfig.m_meshNode = MeshNodeName;

        NvCloth::ClothComponentMesh clothComponentMesh(m_actorComponent->GetEntityId(), clothConfig);

        // Ticking Cloth System updates all its solvers
        for (size_t i = 0; i < 300.0f; ++i)
        {
            const float deltaTimeSim = 1.0f / 60.0f;
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
                deltaTimeSim,
                AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));
        }

        /*
        CryRenderMeshStub renderMesh(MeshVertices);

        LmbrCentral::MeshModificationNotificationBus::Event(
            m_actorComponent->GetEntityId(),
            &LmbrCentral::MeshModificationNotificationBus::Events::ModifyMesh,
            LodLevel,
            0,
            &renderMesh);

        const AZStd::vector<NvCloth::SimParticleFormat>& clothParticles = clothComponentMesh.GetRenderData().m_particles;
        const AZStd::vector<Vec3>& renderMeshPositions = renderMesh.m_positions;

        EXPECT_EQ(renderMeshPositions.size(), clothParticles.size());
        for (size_t i = 0; i < renderMeshPositions.size(); ++i)
        {
            EXPECT_THAT(LYVec3ToAZVec3(renderMeshPositions[i]), IsCloseTolerance(clothParticles[i].GetAsVector3(), Tolerance));
        }
        */
    }
} // namespace UnitTest
