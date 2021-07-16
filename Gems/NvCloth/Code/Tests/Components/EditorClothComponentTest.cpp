/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <ISystem.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <Components/ClothComponent.h>
#include <Components/EditorClothComponent.h>

#include <ActorHelper.h>
#include <Integration/Editor/Components/EditorActorComponent.h>

namespace NvCloth
{
    namespace Internal
    {
        extern const char* const StatusMessageSelectNode;
        extern const char* const StatusMessageNoAsset;
        extern const char* const StatusMessageNoClothNodes;
    }
}

namespace UnitTest
{
    //! Sets up a mock global environment to
    //! change between server and client.
    class NvClothEditorClothComponent
        : public ::testing::Test
    {
    public:
        const AZStd::string JointRootName = "root_node";
        const AZStd::string MeshNodeName = "cloth_mesh_node";

        const AZStd::vector<AZ::Vector3> MeshVertices = {{
            AZ::Vector3(-1.0f, 0.0f, 0.0f),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f)
        }};
        
        const AZStd::vector<NvCloth::SimIndexType> MeshIndices = {{
            0, 1, 2
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

        static void SetUpTestCase();
        static void TearDownTestCase();

    protected:
        AZStd::unique_ptr<AZ::Entity> CreateInactiveEditorEntity(const char* entityName);
        AZStd::unique_ptr<AZ::Entity> CreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity);

    private:
        static AZStd::unique_ptr<SSystemGlobalEnvironment> s_mockGEnv;
        static SSystemGlobalEnvironment* s_previousGEnv;
    };

    AZStd::unique_ptr<SSystemGlobalEnvironment> NvClothEditorClothComponent::s_mockGEnv;
    SSystemGlobalEnvironment* NvClothEditorClothComponent::s_previousGEnv = nullptr;

    void NvClothEditorClothComponent::SetUpTestCase()
    {
        // override global environment
        s_previousGEnv = gEnv;
        s_mockGEnv = AZStd::make_unique<SSystemGlobalEnvironment>();
        gEnv = s_mockGEnv.get();

#if !defined(CONSOLE)
        // Set environment to not be a server by default.
        gEnv->SetIsDedicated(false);
#endif
    }

    void NvClothEditorClothComponent::TearDownTestCase()
    {
        // restore global environment
        gEnv = s_previousGEnv;
        s_mockGEnv.reset();
        s_previousGEnv = nullptr;
    }

    AZStd::unique_ptr<AZ::Entity> NvClothEditorClothComponent::CreateInactiveEditorEntity(const char* entityName)
    {
        AZ::Entity* entity = nullptr;
        UnitTest::CreateDefaultEditorEntity(entityName, &entity);
        entity->Deactivate();
        return AZStd::unique_ptr<AZ::Entity>(entity);
    }

    AZStd::unique_ptr<AZ::Entity> NvClothEditorClothComponent::CreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity)
    {
        auto gameEntity = AZStd::make_unique<AZ::Entity>();
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::PreExportEntity, *editorEntity, *gameEntity);
        gameEntity->Init();
        gameEntity->Activate();
        return gameEntity;
    }

    TEST_F(NvClothEditorClothComponent, EditorClothComponent_DependencyMissing_EntityIsInvalid)
    {
        auto entity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        entity->CreateComponent<NvCloth::EditorClothComponent>();

        // the entity should not be in a valid state because the cloth component requires a mesh or an actor component
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(NvClothEditorClothComponent, EditorClothComponent_ActorDependencySatisfied_EntityIsValid)
    {
        auto entity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        entity->CreateComponent<NvCloth::EditorClothComponent>();
        entity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();

        // the entity should be in a valid state because the cloth component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(NvClothEditorClothComponent, EditorClothComponent_MultipleClothComponents_EntityIsValid)
    {

        auto entity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        entity->CreateComponent<NvCloth::EditorClothComponent>();
        entity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();

        // the cloth component should be compatible with multiple cloth components
        entity->CreateComponent<NvCloth::EditorClothComponent>();
        entity->CreateComponent<NvCloth::EditorClothComponent>();

        // the entity should be in a valid state because the cloth component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(NvClothEditorClothComponent, EditorClothComponent_ClothWithActor_CorrectRuntimeComponents)
    {
        // create an editor entity with a cloth component and an actor component
        auto editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        editorEntity->CreateComponent<NvCloth::EditorClothComponent>();
        editorEntity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();
        editorEntity->Activate();

        auto gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<NvCloth::ClothComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<EMotionFX::Integration::ActorComponent>() != nullptr);
    }

    TEST_F(NvClothEditorClothComponent, EditorClothComponent_OnActivationNoMeshCreated_ReturnsMeshNodeListWithNoAssetMessage)
    {
        auto editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        auto* editorClothComponent = editorEntity->CreateComponent<NvCloth::EditorClothComponent>();
        editorEntity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();
        editorEntity->Activate();

        const NvCloth::MeshNodeList& meshNodeList = editorClothComponent->GetMeshNodeList();

        ASSERT_EQ(meshNodeList.size(), 1);
        EXPECT_TRUE(meshNodeList[0] == NvCloth::Internal::StatusMessageNoAsset);
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothEditorClothComponent, DISABLED_EditorClothComponent_OnMeshCreatedWithEmptyActor_ReturnsMeshNodeListWithNoClothMessage)
    {
        auto editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        auto* editorClothComponent = editorEntity->CreateComponent<NvCloth::EditorClothComponent>();
        auto* editorActorComponent = editorEntity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();
        editorEntity->Activate();

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->FinishSetup();

            editorActorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        const NvCloth::MeshNodeList& meshNodeList = editorClothComponent->GetMeshNodeList();

        ASSERT_EQ(meshNodeList.size(), 1);
        EXPECT_TRUE(meshNodeList[0] == NvCloth::Internal::StatusMessageNoClothNodes);
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothEditorClothComponent, DISABLED_EditorClothComponent_OnMeshCreatedWithActorWithoutClothMesh_ReturnsMeshNodeListWithNoClothMessage)
    {
        auto editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        auto* editorClothComponent = editorEntity->CreateComponent<NvCloth::EditorClothComponent>();
        auto* editorActorComponent = editorEntity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();
        editorEntity->Activate();

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto jointRootIndex = actor->AddJoint(JointRootName);
            actor->SetMesh(LodLevel, jointRootIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, {}, MeshUVs));
            actor->FinishSetup();

            editorActorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        const NvCloth::MeshNodeList& meshNodeList = editorClothComponent->GetMeshNodeList();

        ASSERT_EQ(meshNodeList.size(), 1);
        EXPECT_TRUE(meshNodeList[0] == NvCloth::Internal::StatusMessageNoClothNodes);
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothEditorClothComponent, DISABLED_EditorClothComponent_OnMeshCreatedWithActorWithClothMesh_ReturnsValidMeshNodeList)
    {
        auto editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        auto* editorClothComponent = editorEntity->CreateComponent<NvCloth::EditorClothComponent>();
        auto* editorActorComponent = editorEntity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();
        editorEntity->Activate();

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(JointRootName);
            auto meshNodeIndex = actor->AddJoint(MeshNodeName, AZ::Transform::CreateIdentity(), JointRootName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, {}, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            editorActorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        const NvCloth::MeshNodeList& meshNodeList = editorClothComponent->GetMeshNodeList();

        ASSERT_EQ(meshNodeList.size(), 2);
        EXPECT_TRUE(meshNodeList[0] == NvCloth::Internal::StatusMessageSelectNode);
        EXPECT_TRUE(meshNodeList[1] == MeshNodeName);
    }

    TEST_F(NvClothEditorClothComponent, EditorClothComponent_OnMeshCreatedWithActorWithNoBackstop_ReturnsEmptyMeshNodesWithBackstopData)
    {
        // [inverse mass, motion constrain radius, backstop offset, backstop radius]
        const AZStd::vector<AZ::Color> meshClothDataNoBackstop = {{
            AZ::Color(0.75f, 1.0f, 0.5f, 0.0f),
            AZ::Color(1.0f, 1.0f, 0.5f, 0.0f),
            AZ::Color(0.25f, 1.0f, 0.5f, 0.0f)
        }};

        auto editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        auto* editorClothComponent = editorEntity->CreateComponent<NvCloth::EditorClothComponent>();
        auto* editorActorComponent = editorEntity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();
        editorEntity->Activate();

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(JointRootName);
            auto meshNodeIndex = actor->AddJoint(MeshNodeName, AZ::Transform::CreateIdentity(), JointRootName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, {}, MeshUVs/*, meshClothDataNoBackstop*/));
            actor->FinishSetup();

            editorActorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        const auto& meshNodesWithBackstopData = editorClothComponent->GetMeshNodesWithBackstopData();

        EXPECT_TRUE(meshNodesWithBackstopData.empty());
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothEditorClothComponent, DISABLED_EditorClothComponent_OnMeshCreatedWithActorWithBackstop_ReturnsValidMeshNodesWithBackstopData)
    {
        auto editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        auto* editorClothComponent = editorEntity->CreateComponent<NvCloth::EditorClothComponent>();
        auto* editorActorComponent = editorEntity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();
        editorEntity->Activate();

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(JointRootName);
            auto meshNodeIndex = actor->AddJoint(MeshNodeName, AZ::Transform::CreateIdentity(), JointRootName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, {}, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            editorActorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        const auto& meshNodesWithBackstopData = editorClothComponent->GetMeshNodesWithBackstopData();

        EXPECT_EQ(meshNodesWithBackstopData.size(), 1);
        EXPECT_TRUE(meshNodesWithBackstopData.find(MeshNodeName) != meshNodesWithBackstopData.end());
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // Editor Cloth component now uses the new AZ::Render::MeshComponentNotificationBus::OnModelReady
    // notification and this test does not setup a model yet.
    TEST_F(NvClothEditorClothComponent, DISABLED_EditorClothComponent_OnModelPreDestroy_ReturnsMeshNodeListWithNoAssetMessage)
    {
        auto editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        auto* editorClothComponent = editorEntity->CreateComponent<NvCloth::EditorClothComponent>();
        auto* editorActorComponent = editorEntity->CreateComponent<EMotionFX::Integration::EditorActorComponent>();
        editorEntity->Activate();

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            actor->AddJoint(JointRootName);
            auto meshNodeIndex = actor->AddJoint(MeshNodeName, AZ::Transform::CreateIdentity(), JointRootName);
            actor->SetMesh(LodLevel, meshNodeIndex, CreateEMotionFXMesh(MeshVertices, MeshIndices, {}, MeshUVs/*, MeshClothData*/));
            actor->FinishSetup();

            editorActorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        editorClothComponent->OnModelPreDestroy();

        const NvCloth::MeshNodeList& meshNodeList = editorClothComponent->GetMeshNodeList();
        const auto& meshNodesWithBackstopData = editorClothComponent->GetMeshNodesWithBackstopData();

        ASSERT_EQ(meshNodeList.size(), 1);
        EXPECT_TRUE(meshNodeList[0] == NvCloth::Internal::StatusMessageNoAsset);
        EXPECT_TRUE(meshNodesWithBackstopData.empty());
    }
} // namespace UnitTest
