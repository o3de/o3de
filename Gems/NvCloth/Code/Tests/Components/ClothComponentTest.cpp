/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Components/ClothComponent.h>

#include <ActorHelper.h>
#include <Integration/Components/ActorComponent.h>

namespace UnitTest
{
    class NvClothComponent
        : public ::testing::Test
    {
    protected:
        AZStd::unique_ptr<AZ::Entity> CreateClothActorEntity(const NvCloth::ClothConfiguration& clothConfiguration);
        bool IsConnectedToMeshComponentNotificationBus(NvCloth::ClothComponent* clothComponent) const;
    };

    AZStd::unique_ptr<AZ::Entity> NvClothComponent::CreateClothActorEntity(const NvCloth::ClothConfiguration& clothConfiguration)
    {
        AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>();
        entity->CreateComponent<AzFramework::TransformComponent>();
        entity->CreateComponent<EMotionFX::Integration::ActorComponent>();
        entity->CreateComponent<NvCloth::ClothComponent>(clothConfiguration);
        entity->Init();
        return entity;
    }

    bool NvClothComponent::IsConnectedToMeshComponentNotificationBus(NvCloth::ClothComponent* clothComponent) const
    {
        return static_cast<AZ::Render::MeshComponentNotificationBus::Handler*>(clothComponent)->BusIsConnectedId(clothComponent->GetEntityId());
    }

    TEST_F(NvClothComponent, ClothComponent_WithoutDependencies_ReturnsMissingRequiredService)
    {
        AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>();
        entity->CreateComponent<NvCloth::ClothComponent>();
        entity->Init();

        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(NvClothComponent, ClothComponent_WithTransformAndActor_DependenciesAreMet)
    {
        AZStd::unique_ptr<AZ::Entity> entity = CreateClothActorEntity({});

        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(NvClothComponent, ClothComponent_WithoutMultiplayerGem_ConnectsToMeshComponentNotificationBusOnActivation)
    {
        AZStd::unique_ptr<AZ::Entity> entity = CreateClothActorEntity({});
        entity->Activate();

        auto* clothComponent = entity->FindComponent<NvCloth::ClothComponent>();

        EXPECT_TRUE(IsConnectedToMeshComponentNotificationBus(clothComponent));
    }

    TEST_F(NvClothComponent, ClothComponent_WithMultiplayerGem_Game_ConnectsToMeshComponentNotificationBusOnActivation)
    {
        // Fake that multiplayer gem is enabled by creating a local sv_isDedicated AZ_CVAR
        AZ::ConsoleDataWrapper<bool, ConsoleThreadSafety<bool>> sv_isDedicated(false,
            nullptr, "sv_isDedicated", "", AZ::ConsoleFunctorFlags::DontReplicate);

        AZStd::unique_ptr<AZ::Entity> entity = CreateClothActorEntity({});
        entity->Activate();

        auto* clothComponent = entity->FindComponent<NvCloth::ClothComponent>();

        EXPECT_TRUE(IsConnectedToMeshComponentNotificationBus(clothComponent));
    }

    TEST_F(NvClothComponent, ClothComponent_WithMultiplayerGem_Server_DoesNotConnectToMeshComponentNotificationBusOnActivation)
    {
        // Fake that multiplayer gem is enabled by creating a local sv_isDedicated AZ_CVAR
        AZ::ConsoleDataWrapper<bool, ConsoleThreadSafety<bool>> sv_isDedicated(true,
            nullptr, "sv_isDedicated", "", AZ::ConsoleFunctorFlags::DontReplicate);

        AZStd::unique_ptr<AZ::Entity> entity = CreateClothActorEntity({});
        entity->Activate();

        auto* clothComponent = entity->FindComponent<NvCloth::ClothComponent>();

        EXPECT_FALSE(IsConnectedToMeshComponentNotificationBus(clothComponent));
    }

    TEST_F(NvClothComponent, ClothComponent_OneEntityWithTwoClothComponents_BothConnectToMeshComponentNotificationBusOnActivation)
    {
        AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>();
        entity->CreateComponent<AzFramework::TransformComponent>();
        entity->CreateComponent<EMotionFX::Integration::ActorComponent>();
        auto* clothComponent1 = entity->CreateComponent<NvCloth::ClothComponent>();
        auto* clothComponent2 = entity->CreateComponent<NvCloth::ClothComponent>();
        entity->Init();
        entity->Activate();

        EXPECT_TRUE(IsConnectedToMeshComponentNotificationBus(clothComponent1));
        EXPECT_TRUE(IsConnectedToMeshComponentNotificationBus(clothComponent2));
    }

    TEST_F(NvClothComponent, ClothComponent_AfterDeactivation_IsNotConnectedToMeshComponentNotificationBus)
    {
        AZStd::unique_ptr<AZ::Entity> entity = CreateClothActorEntity({});
        entity->Activate();

        auto* clothComponent = entity->FindComponent<NvCloth::ClothComponent>();

        EXPECT_TRUE(IsConnectedToMeshComponentNotificationBus(clothComponent));

        entity->Deactivate();

        EXPECT_FALSE(IsConnectedToMeshComponentNotificationBus(clothComponent));
    }

    // [TODO LYN-1891]
    // Revisit when Cloth Component Mesh works with Actors adapted to Atom models.
    // At the moment, CreateAssetFromActor fills only Actor to the ActorAsset, but not the RenderActor,
    // because of that the AtomModel is not created and OnModelReady is not called.
    TEST_F(NvClothComponent, DISABLED_ClothComponent_WithActorSetup_ReturnsValidClothComponentMesh)
    {
        const AZStd::string meshNodeName = "cloth_mesh_node";

        const AZStd::vector<AZ::Vector3> meshVertices = {{
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            AZ::Vector3(-1.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f)
        }};
        
        const AZStd::vector<NvCloth::SimIndexType> meshIndices = {{
            0, 1, 2
        }};
        
        const AZStd::vector<VertexSkinInfluences> meshSkinningInfo = {{
            VertexSkinInfluences{ SkinInfluence(0, 1.0f) },
            VertexSkinInfluences{ SkinInfluence(0, 1.0f) },
            VertexSkinInfluences{ SkinInfluence(0, 1.0f) }
        }};
        
        const AZStd::vector<AZ::Vector2> meshUVs = {{
            AZ::Vector2(1.0f, 1.0f),
            AZ::Vector2(0.0f, 1.0f),
            AZ::Vector2(0.5f, 0.0f)
        }};
        
        // [inverse mass, motion constrain radius, backstop offset, backstop radius]
        const AZStd::vector<AZ::Color> meshClothData = {{
            AZ::Color(0.75f, 0.6f, 0.5f, 0.1f),
            AZ::Color(1.0f, 0.16f, 0.1f, 1.0f),
            AZ::Color(0.25f, 1.0f, 0.9f, 0.5f)
        }};

        const AZ::u32 lodLevel = 0;

        NvCloth::ClothConfiguration clothConfiguration;
        clothConfiguration.m_meshNode = meshNodeName;

        AZStd::unique_ptr<AZ::Entity> entity = CreateClothActorEntity(clothConfiguration);
        entity->Activate();

        auto* actorComponent = entity->FindComponent<EMotionFX::Integration::ActorComponent>();
        auto* clothComponent = entity->FindComponent<NvCloth::ClothComponent>();

        {
            auto actor = AZStd::make_unique<ActorHelper>("actor_test");
            auto meshNodeIndex = actor->AddJoint(meshNodeName);
            actor->SetMesh(lodLevel, meshNodeIndex, CreateEMotionFXMesh(meshVertices, meshIndices, meshSkinningInfo, meshUVs/*, meshClothData*/));
            actor->FinishSetup();

            actorComponent->SetActorAsset(CreateAssetFromActor(AZStd::move(actor)));
        }

        EXPECT_TRUE(clothComponent->GetClothComponentMesh() != nullptr);
    }
} // namespace UnitTest
