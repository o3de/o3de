/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QtTest>
#include <QTreeView>
#include <QTreeWidget>

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/SimpleLODComponent.h>
#include <Integration/Rendering/RenderBackend.h>
#include <Integration/Rendering/RenderBackendManager.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/NodeHierarchyWidget.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/UIFixture.h>
#include <Editor/ReselectingTreeView.h>

#include <Mocks/ISystemMock.h>

namespace EMotionFX
{
    class LODSkinnedMeshFixture
        : public ::testing::WithParamInterface <int>
        , public UIFixture
    {
    public:
    };

    class LODSystemMock : public SystemMock
    {
    };

    class LODSkinnedMeshColorFixture
        : public UIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();

            m_app.RegisterComponentDescriptor(Integration::SimpleLODComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(Integration::ActorComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(AzFramework::TransformComponent::CreateDescriptor());

            m_envPrev = gEnv;
            m_env.pSystem = &m_data.m_system;
            gEnv = &m_env;
        }

        void TearDown() override
        {
            UIFixture::TearDown();
            gEnv = m_envPrev;
        }

        struct DataMembers
        {
            testing::NiceMock<LODSystemMock> m_system;
        };

        SSystemGlobalEnvironment* m_envPrev = nullptr;
        SSystemGlobalEnvironment m_env;
        DataMembers m_data;
    };

    AZ::Data::Asset<Integration::ActorAsset> CreateLODActor(int numLODs)
    {
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<PlaneActor>(actorAssetId, "LODSkinnedMeshTestsActor");

        // Modify the actor to have numLODs LOD levels.
        Actor* actor = actorAsset->GetActor();
        Mesh* lodMesh = actor->GetMesh(0, 0);
        
        for (int i = 1; i < numLODs; ++i)
        {
            actor->InsertLODLevel(i);
            actor->SetMesh(i, 0, lodMesh->Clone());
        }

        return AZStd::move(actorAsset);
    }

    class LODPropertyRowWidget
        : public AzToolsFramework::PropertyRowWidget
    {
    public:
        QLabel* GetDefaultLabel() { return m_defaultLabel; }
    };

    INSTANTIATE_TEST_CASE_P(LODSkinnedMeshFixtureTests, LODSkinnedMeshFixture, ::testing::Range<int>(1, 7));

    // TODO: Re-enabled the test when we can access viewport context in the SimpleLODComponent.
    TEST_F(LODSkinnedMeshColorFixture, DISABLED_CheckLODDistanceChange)
    {
        const int numLODs = 6;
        RecordProperty("test_case_id", "C29202698");

        AZ::EntityId entityId(740216387);

        auto gameEntity = AZStd::make_unique<AZ::Entity>();
        gameEntity->SetId(entityId);

        AZ::Data::AssetId actorAssetId("{85D3EF54-7400-43F8-8A40-F6BCBF534E54}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset = CreateLODActor(numLODs);

        gameEntity->CreateComponent<AzFramework::TransformComponent>();
        Integration::ActorComponent::Configuration actorConf;
        actorConf.m_actorAsset = actorAsset;
        Integration::ActorComponent* actorComponent = gameEntity->CreateComponent<Integration::ActorComponent>(&actorConf);

        Integration::SimpleLODComponent::Configuration lodConf;
        lodConf.GenerateDefaultValue(numLODs);
        gameEntity->CreateComponent<Integration::SimpleLODComponent>(&lodConf);

        gameEntity->Init();
        gameEntity->Activate();

        actorComponent->SetActorAsset(actorAsset);

        ActorInstance* actorInstance = actorComponent->GetActorInstance();
        EXPECT_TRUE(actorInstance);

        // Tick!
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.0f, AZ::ScriptTimePoint{});

        EXPECT_EQ(actorInstance->GetLODLevel(), 0);

        Vec3 newVec{ 0,30,0 };

        // Tick!
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.0f, AZ::ScriptTimePoint{});

        actorInstance->UpdateTransformations(0.0f);

        EXPECT_EQ(actorInstance->GetLODLevel(), 3);

        newVec.y = 50;

        // Tick!
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.0f, AZ::ScriptTimePoint{});

        actorInstance->UpdateTransformations(0.0f);

        EXPECT_EQ(actorInstance->GetLODLevel(), 5);

        gameEntity->Deactivate();
    }
}
