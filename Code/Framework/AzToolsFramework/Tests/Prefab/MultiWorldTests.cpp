/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Render/IntersectorInterface.h>
#include <AzFramework/Scene/Scene.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabSaveLoadHandler.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using AzToolsFramework::EditorEntityContextRequestBus;
    using AzToolsFramework::EditorEntityContextRequests;
    using AzToolsFramework::PrefabEditorEntityOwnershipInterface;

    class MultiWorldTests
        : public PrefabTestFixture
    {
    protected:
        static constexpr AzFramework::ViewportId ViewportA = 10;
        static constexpr AzFramework::ViewportId ViewportB = 11;
        static constexpr AzFramework::ViewportId ViewportNeverBound = 12;

        inline static const char* LevelPathA = "test/worldA.prefab";
        inline static const char* LevelPathB = "test/worldB.prefab";
        inline static const char* NestedPathA = "test/worldA_nested.prefab";

        void GenerateWorldTemplates()
        {
            AZ::Entity* nestedEntityA = CreateEntity("NestedA");
            AZ::Entity* entityInWorldB = CreateEntity("EntityInWorldB");
            ASSERT_TRUE(nestedEntityA != nullptr);
            ASSERT_TRUE(entityInWorldB != nullptr);
            AddRequiredEditorComponents({ nestedEntityA->GetId(), entityInWorldB->GetId() });

            AZStd::unique_ptr<Instance> nestedInstanceA =
                m_prefabSystemComponent->CreatePrefab({ nestedEntityA }, {}, AZ::IO::PathView(NestedPathA));
            ASSERT_TRUE(nestedInstanceA);

            AZStd::unique_ptr<Instance> levelInstanceA = m_prefabSystemComponent->CreatePrefab(
                {}, MakeInstanceList(AZStd::move(nestedInstanceA)), AZ::IO::PathView(LevelPathA));
            ASSERT_TRUE(levelInstanceA);

            AZStd::unique_ptr<Instance> levelInstanceB =
                m_prefabSystemComponent->CreatePrefab({ entityInWorldB }, {}, AZ::IO::PathView(LevelPathB));
            ASSERT_TRUE(levelInstanceB);

            levelInstanceA.reset();
            levelInstanceB.reset();

            ASSERT_NE(m_prefabSystemComponent->GetTemplateIdFromFilePath(AZ::IO::PathView(LevelPathA)), InvalidTemplateId);
            ASSERT_NE(m_prefabSystemComponent->GetTemplateIdFromFilePath(AZ::IO::PathView(LevelPathB)), InvalidTemplateId);
        }

        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();

            m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            ASSERT_TRUE(m_prefabFocusInterface != nullptr);

            m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
            ASSERT_TRUE(m_prefabFocusPublicInterface != nullptr);

            m_viewportEditorModeTracker = AZ::Interface<AzToolsFramework::ViewportEditorModeTrackerInterface>::Get();
            ASSERT_TRUE(m_viewportEditorModeTracker != nullptr);

            EditorEntityContextRequestBus::BroadcastResult(m_editorWorld, &EditorEntityContextRequests::GetEditorEntityContextId);
            ASSERT_FALSE(m_editorWorld.IsNull());

            GenerateWorldTemplates();
        }

        void TearDownEditorFixtureImpl() override
        {
            for (const AzFramework::ViewportId viewportId : { ViewportA, ViewportB, ViewportNeverBound })
            {
                EditorEntityContextRequestBus::Broadcast(
                    &EditorEntityContextRequests::BindViewportToWorld, viewportId, AzFramework::EntityContextId::CreateNull());
            }

            PrefabTestFixture::TearDownEditorFixtureImpl();
        }

        AzFramework::EntityContextId m_editorWorld = AzFramework::EntityContextId::CreateNull();

        PrefabFocusInterface* m_prefabFocusInterface = nullptr;
        PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
        AzToolsFramework::ViewportEditorModeTrackerInterface* m_viewportEditorModeTracker = nullptr;
    };

    TEST_F(MultiWorldTests, UnboundViewportShowsTheEditorWorld)
    {
        AzFramework::EntityContextId viewportWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            viewportWorld, &EditorEntityContextRequests::GetViewportWorld, ViewportNeverBound);

        EXPECT_EQ(viewportWorld, m_editorWorld);
        EXPECT_FALSE(viewportWorld.IsNull());
    }

    TEST_F(MultiWorldTests, BindingAViewportToAWorldShowsThatWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        ASSERT_FALSE(worldA.IsNull());
        EXPECT_NE(worldA, m_editorWorld);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);

        AzFramework::EntityContextId viewportWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            viewportWorld, &EditorEntityContextRequests::GetViewportWorld, ViewportA);
        EXPECT_EQ(viewportWorld, worldA);
    }

    TEST_F(MultiWorldTests, BindingToANullWorldUnbindsTheViewport)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);

        EditorEntityContextRequestBus::Broadcast(
            &EditorEntityContextRequests::BindViewportToWorld, ViewportA, AzFramework::EntityContextId::CreateNull());

        AzFramework::EntityContextId viewportWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            viewportWorld, &EditorEntityContextRequests::GetViewportWorld, ViewportA);
        EXPECT_EQ(viewportWorld, m_editorWorld);
    }

    TEST_F(MultiWorldTests, BindingToTheEditorWorldShowsTheEditorWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, m_editorWorld);

        AzFramework::EntityContextId viewportWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            viewportWorld, &EditorEntityContextRequests::GetViewportWorld, ViewportA);
        EXPECT_EQ(viewportWorld, m_editorWorld);

        PrefabEditorEntityOwnershipInterface* ownershipService = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipService, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        EXPECT_EQ(ownershipService, nullptr);
    }

    TEST_F(MultiWorldTests, TwoDifferentLevelsProduceTwoWorlds)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));

        AzFramework::EntityContextId worldB = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldB, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathB));

        EXPECT_FALSE(worldA.IsNull());
        EXPECT_FALSE(worldB.IsNull());
        EXPECT_NE(worldA, worldB);
    }

    TEST_F(MultiWorldTests, LoadingTheEditorsOwnLevelReturnsTheEditorWorld)
    {
        AzFramework::EntityContextId worldId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldId, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView("UnitTestRoot.prefab"));

        EXPECT_EQ(worldId, m_editorWorld);
    }

    TEST_F(MultiWorldTests, LoadingAnAlreadyLoadedLevelReturnsTheSameWorld)
    {
        AzFramework::EntityContextId firstLoad = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            firstLoad, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, firstLoad);

        AzFramework::EntityContextId secondLoad = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            secondLoad, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));

        EXPECT_FALSE(firstLoad.IsNull());
        EXPECT_EQ(firstLoad, secondLoad);
    }

    TEST_F(MultiWorldTests, TwoViewportsCanShowTheSameWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, worldA);

        AzFramework::EntityContextId viewportAWorld = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityContextId viewportBWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            viewportAWorld, &EditorEntityContextRequests::GetViewportWorld, ViewportA);
        EditorEntityContextRequestBus::BroadcastResult(
            viewportBWorld, &EditorEntityContextRequests::GetViewportWorld, ViewportB);

        EXPECT_EQ(viewportAWorld, worldA);
        EXPECT_EQ(viewportBWorld, worldA);
    }

    TEST_F(MultiWorldTests, AWorldSurvivesWhileAnotherViewportStillShowsIt)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, worldA);

        EditorEntityContextRequestBus::Broadcast(
            &EditorEntityContextRequests::BindViewportToWorld, ViewportA, AzFramework::EntityContextId::CreateNull());

        PrefabEditorEntityOwnershipInterface* ownershipService = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipService, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        EXPECT_NE(ownershipService, nullptr);

        AzFramework::EntityContextId viewportBWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            viewportBWorld, &EditorEntityContextRequests::GetViewportWorld, ViewportB);
        EXPECT_EQ(viewportBWorld, worldA);
    }

    TEST_F(MultiWorldTests, UnbindingTheLastViewportDestroysTheWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);

        EXPECT_TRUE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(worldA));

        EditorEntityContextRequestBus::Broadcast(
            &EditorEntityContextRequests::BindViewportToWorld, ViewportA, AzFramework::EntityContextId::CreateNull());

        PrefabEditorEntityOwnershipInterface* ownershipService = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipService, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        EXPECT_EQ(ownershipService, nullptr);

        EXPECT_FALSE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(worldA));

        const AzToolsFramework::ViewportEditorModesInterface* editorModes =
            m_viewportEditorModeTracker->GetViewportEditorModes({ worldA });
        EXPECT_TRUE(editorModes == nullptr || !editorModes->IsModeActive(AzToolsFramework::ViewportEditorMode::Default));
    }

    TEST_F(MultiWorldTests, RebindingAViewportDestroysTheWorldItLeft)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityContextId worldB = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::BroadcastResult(
            worldB, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathB));

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldB);

        PrefabEditorEntityOwnershipInterface* serviceForWorldA = nullptr;
        PrefabEditorEntityOwnershipInterface* serviceForWorldB = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            serviceForWorldA, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        EditorEntityContextRequestBus::BroadcastResult(
            serviceForWorldB, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldB);

        EXPECT_EQ(serviceForWorldA, nullptr);
        EXPECT_NE(serviceForWorldB, nullptr);
    }

    TEST_F(MultiWorldTests, ActiveWorldIsTheFocusedViewportsWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityContextId worldB = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::BroadcastResult(
            worldB, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathB));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, worldB);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);
        EXPECT_EQ(AzToolsFramework::GetActiveWorldId(), worldA);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportB);
        EXPECT_EQ(AzToolsFramework::GetActiveWorldId(), worldB);
    }

    TEST_F(MultiWorldTests, FocusingAnUnboundViewportMakesTheEditorWorldActive)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportNeverBound);
        EXPECT_EQ(AzToolsFramework::GetActiveWorldId(), m_editorWorld);

        EditorEntityContextRequestBus::Broadcast(
            &EditorEntityContextRequests::SetFocusedViewport, AzFramework::InvalidViewportId);
        EXPECT_EQ(AzToolsFramework::GetActiveWorldId(), m_editorWorld);
    }

    TEST_F(MultiWorldTests, RebindingTheFocusedViewportChangesTheActiveWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);
        EXPECT_EQ(AzToolsFramework::GetActiveWorldId(), m_editorWorld);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EXPECT_EQ(AzToolsFramework::GetActiveWorldId(), worldA);
    }

    TEST_F(MultiWorldTests, EachWorldHasItsOwnUndoStack)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityContextId worldB = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::BroadcastResult(
            worldB, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathB));
        ASSERT_NE(worldA, worldB);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, worldB);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);
        AzToolsFramework::UndoSystem::UndoStack* stackA = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            stackA, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportB);
        AzToolsFramework::UndoSystem::UndoStack* stackB = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            stackB, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);

        EXPECT_NE(stackA, nullptr);
        EXPECT_NE(stackB, nullptr);
        EXPECT_NE(stackA, stackB);
    }

    TEST_F(MultiWorldTests, ViewportsShowingTheSameWorldShareAnUndoStack)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, worldA);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);
        AzToolsFramework::UndoSystem::UndoStack* stackA = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            stackA, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportB);
        AzToolsFramework::UndoSystem::UndoStack* stackB = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            stackB, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);

        EXPECT_NE(stackA, nullptr);
        EXPECT_EQ(stackA, stackB);
    }

    TEST_F(MultiWorldTests, EachWorldHasItsOwnOwnershipServiceAndScene)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityContextId worldB = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::BroadcastResult(
            worldB, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathB));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, worldB);

        PrefabEditorEntityOwnershipInterface* serviceForEditorWorld = nullptr;
        PrefabEditorEntityOwnershipInterface* serviceForWorldA = nullptr;
        PrefabEditorEntityOwnershipInterface* serviceForWorldB = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            serviceForEditorWorld, &EditorEntityContextRequests::GetWorldEntityOwnershipService, m_editorWorld);
        EditorEntityContextRequestBus::BroadcastResult(
            serviceForWorldA, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        EditorEntityContextRequestBus::BroadcastResult(
            serviceForWorldB, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldB);

        EXPECT_EQ(serviceForEditorWorld, AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get());
        EXPECT_NE(serviceForWorldA, nullptr);
        EXPECT_NE(serviceForWorldB, nullptr);
        EXPECT_NE(serviceForWorldA, serviceForEditorWorld);
        EXPECT_NE(serviceForWorldA, serviceForWorldB);

        AZStd::shared_ptr<AzFramework::Scene> sceneForEditorWorld;
        AZStd::shared_ptr<AzFramework::Scene> sceneForWorldA;
        AZStd::shared_ptr<AzFramework::Scene> sceneForWorldB;
        EditorEntityContextRequestBus::BroadcastResult(
            sceneForEditorWorld, &EditorEntityContextRequests::GetWorldScene, m_editorWorld);
        EditorEntityContextRequestBus::BroadcastResult(
            sceneForWorldA, &EditorEntityContextRequests::GetWorldScene, worldA);
        EditorEntityContextRequestBus::BroadcastResult(
            sceneForWorldB, &EditorEntityContextRequests::GetWorldScene, worldB);

        EXPECT_NE(sceneForWorldA, nullptr);
        EXPECT_NE(sceneForWorldB, nullptr);
        EXPECT_NE(sceneForWorldA, sceneForEditorWorld);
        EXPECT_NE(sceneForWorldA, sceneForWorldB);
        EXPECT_EQ(sceneForWorldA, AzFramework::EntityContext::FindContainingScene(worldA));
        EXPECT_EQ(sceneForWorldB, AzFramework::EntityContext::FindContainingScene(worldB));
    }

    TEST_F(MultiWorldTests, NullWorldIdResolvesToTheActiveWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);

        EXPECT_EQ(AzToolsFramework::ResolveWorldId(AzFramework::EntityContextId::CreateNull()), worldA);
        EXPECT_EQ(AzToolsFramework::ResolveWorldId(m_editorWorld), m_editorWorld);
        EXPECT_EQ(AzToolsFramework::ResolveWorldId(worldA), worldA);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportNeverBound);
        EXPECT_EQ(AzToolsFramework::ResolveWorldId(AzFramework::EntityContextId::CreateNull()), m_editorWorld);
    }

    TEST_F(MultiWorldTests, WorldLevelPathNamesTheLevelAndIsEmptyForTheEditorWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);

        AZStd::string levelPathForWorldA;
        AZStd::string levelPathForEditorWorld;
        AZStd::string levelPathForUnknownWorld;
        EditorEntityContextRequestBus::BroadcastResult(
            levelPathForWorldA, &EditorEntityContextRequests::GetWorldLevelPath, worldA);
        EditorEntityContextRequestBus::BroadcastResult(
            levelPathForEditorWorld, &EditorEntityContextRequests::GetWorldLevelPath, m_editorWorld);
        EditorEntityContextRequestBus::BroadcastResult(
            levelPathForUnknownWorld, &EditorEntityContextRequests::GetWorldLevelPath,
            AzFramework::EntityContextId::CreateRandom());

        EXPECT_EQ(levelPathForWorldA, LevelPathA);
        EXPECT_TRUE(levelPathForEditorWorld.empty());
        EXPECT_TRUE(levelPathForUnknownWorld.empty());
    }

    TEST_F(MultiWorldTests, EntitiesCreatedInAWorldBelongToThatWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);

        PrefabEditorEntityOwnershipInterface* ownershipService = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipService, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        ASSERT_NE(ownershipService, nullptr);

        InstanceOptionalReference rootInstance = ownershipService->GetRootPrefabInstance();
        ASSERT_TRUE(rootInstance.has_value());

        const AZ::EntityId entityInWorldA = CreateEditorEntity("OwnedByWorldA", rootInstance->get().GetContainerEntityId());
        ASSERT_TRUE(entityInWorldA.IsValid());

        EXPECT_EQ(AzToolsFramework::GetEntityWorldId(entityInWorldA), worldA);

        bool isEditorEntity = false;
        EditorEntityContextRequestBus::BroadcastResult(
            isEditorEntity, &EditorEntityContextRequests::IsEditorEntity, entityInWorldA);
        EXPECT_TRUE(isEditorEntity);
    }

    TEST_F(MultiWorldTests, EachWorldKeepsItsOwnFocus)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityContextId worldB = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::BroadcastResult(
            worldB, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathB));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, worldB);

        PrefabEditorEntityOwnershipInterface* ownershipServiceForWorldA = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipServiceForWorldA, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        ASSERT_NE(ownershipServiceForWorldA, nullptr);

        InstanceOptionalReference rootOfWorldA = ownershipServiceForWorldA->GetRootPrefabInstance();
        ASSERT_TRUE(rootOfWorldA.has_value());

        AZ::EntityId nestedContainerInWorldA;
        rootOfWorldA->get().GetNestedInstances(
            [&nestedContainerInWorldA](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedContainerInWorldA = nestedInstance->GetContainerEntityId();
            });
        ASSERT_TRUE(nestedContainerInWorldA.IsValid());

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);
        ASSERT_TRUE(m_prefabFocusPublicInterface->FocusOnOwningPrefab(nestedContainerInWorldA).IsSuccess());

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportB);

        EXPECT_EQ(m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(worldA), nestedContainerInWorldA);
        EXPECT_NE(m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(worldB), nestedContainerInWorldA);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);
        EXPECT_EQ(m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(worldA), nestedContainerInWorldA);
    }

    TEST_F(MultiWorldTests, ModifiedWorldTemplateIdsTracksDirtySecondaryWorldsOnly)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityContextId worldB = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::BroadcastResult(
            worldB, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathB));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, worldB);

        PrefabEditorEntityOwnershipInterface* serviceForEditorWorld = nullptr;
        PrefabEditorEntityOwnershipInterface* serviceForWorldA = nullptr;
        PrefabEditorEntityOwnershipInterface* serviceForWorldB = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            serviceForEditorWorld, &EditorEntityContextRequests::GetWorldEntityOwnershipService, m_editorWorld);
        EditorEntityContextRequestBus::BroadcastResult(
            serviceForWorldA, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        EditorEntityContextRequestBus::BroadcastResult(
            serviceForWorldB, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldB);

        m_prefabSystemComponent->SetTemplateDirtyFlag(serviceForEditorWorld->GetRootPrefabTemplateId(), true);
        m_prefabSystemComponent->SetTemplateDirtyFlag(serviceForWorldA->GetRootPrefabTemplateId(), false);
        m_prefabSystemComponent->SetTemplateDirtyFlag(serviceForWorldB->GetRootPrefabTemplateId(), false);

        AZStd::vector<TemplateId> modifiedTemplateIds;
        EditorEntityContextRequestBus::BroadcastResult(
            modifiedTemplateIds, &EditorEntityContextRequests::GetModifiedWorldTemplateIds);
        EXPECT_TRUE(modifiedTemplateIds.empty());

        m_prefabSystemComponent->SetTemplateDirtyFlag(serviceForWorldB->GetRootPrefabTemplateId(), true);

        EditorEntityContextRequestBus::BroadcastResult(
            modifiedTemplateIds, &EditorEntityContextRequests::GetModifiedWorldTemplateIds);
        ASSERT_EQ(modifiedTemplateIds.size(), 1u);
        EXPECT_EQ(modifiedTemplateIds[0], serviceForWorldB->GetRootPrefabTemplateId());
        EXPECT_NE(modifiedTemplateIds[0], serviceForEditorWorld->GetRootPrefabTemplateId());
    }

    TEST_F(MultiWorldTests, IsPrefabSourcePathAcceptsOnlyPrefabExtensions)
    {
        EXPECT_TRUE(PrefabSaveHandler::IsPrefabSourcePath("Levels/Test/Test.prefab"));
        EXPECT_TRUE(PrefabSaveHandler::IsPrefabSourcePath("Test.PREFAB"));
        EXPECT_FALSE(PrefabSaveHandler::IsPrefabSourcePath("Levels/Test/Test.spawnable"));
        EXPECT_FALSE(PrefabSaveHandler::IsPrefabSourcePath("Test.prefab.bak"));
        EXPECT_FALSE(PrefabSaveHandler::IsPrefabSourcePath(""));
    }

    TEST_F(MultiWorldTests, OpeningANewLevelWhileAnotherWorldIsOpenRebuildsTheEditorWorld)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::ResetEditorContext);

        m_prefabEditorEntityOwnershipInterface->CreateNewLevelPrefab("UnitTestRoot2.prefab", "");

        InstanceOptionalReference rootOfEditorWorld = m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        ASSERT_TRUE(rootOfEditorWorld.has_value());
        EXPECT_NE(rootOfEditorWorld->get().GetTemplateId(), InvalidTemplateId);
        EXPECT_TRUE(rootOfEditorWorld->get().GetContainerEntityId().IsValid());

        PrefabEditorEntityOwnershipInterface* ownershipServiceForWorldA = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipServiceForWorldA, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        ASSERT_NE(ownershipServiceForWorldA, nullptr);
        InstanceOptionalReference rootOfWorldA = ownershipServiceForWorldA->GetRootPrefabInstance();
        ASSERT_TRUE(rootOfWorldA.has_value());

        EXPECT_EQ(
            m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(m_editorWorld),
            rootOfEditorWorld->get().GetContainerEntityId());
        EXPECT_EQ(
            m_prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(worldA),
            rootOfWorldA->get().GetContainerEntityId());
    }

    TEST_F(MultiWorldTests, ResettingTheEditorContextKeepsTheOtherWorldsEntitiesInTheOutlinerModel)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);

        PrefabEditorEntityOwnershipInterface* ownershipServiceForWorldA = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipServiceForWorldA, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        ASSERT_NE(ownershipServiceForWorldA, nullptr);

        InstanceOptionalReference rootOfWorldA = ownershipServiceForWorldA->GetRootPrefabInstance();
        ASSERT_TRUE(rootOfWorldA.has_value());

        const AZ::EntityId rootContainerOfWorldA = rootOfWorldA->get().GetContainerEntityId();
        ASSERT_TRUE(rootContainerOfWorldA.IsValid());
        ASSERT_TRUE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(rootContainerOfWorldA));

        AzFramework::EntityContextId editorWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(editorWorld, &EditorEntityContextRequests::GetEditorEntityContextId);

        PrefabEditorEntityOwnershipInterface* ownershipServiceForEditorWorld = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipServiceForEditorWorld, &EditorEntityContextRequests::GetWorldEntityOwnershipService, editorWorld);
        ASSERT_NE(ownershipServiceForEditorWorld, nullptr);

        InstanceOptionalReference rootOfEditorWorld = ownershipServiceForEditorWorld->GetRootPrefabInstance();
        ASSERT_TRUE(rootOfEditorWorld.has_value());

        const AZ::EntityId entityInEditorWorld = CreateEditorEntity("OwnedByEditorWorld", rootOfEditorWorld->get().GetContainerEntityId());
        ASSERT_TRUE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(entityInEditorWorld));

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::ResetEditorContext);

        EXPECT_TRUE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(rootContainerOfWorldA))
            << "The outliner model dropped another world's root prefab when the editor world reset";
        EXPECT_FALSE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(entityInEditorWorld))
            << "The outliner model kept the editor world's entities after the editor world reset";
    }

    TEST_F(MultiWorldTests, ResettingTheEditorContextKeepsASecondaryWorldsRootPrefabInTheOutlinerModel)
    {
        AzFramework::EntityContextId prefabWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            prefabWorld, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(NestedPathA));
        ASSERT_FALSE(prefabWorld.IsNull());
        ASSERT_NE(prefabWorld, m_editorWorld);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, prefabWorld);

        PrefabEditorEntityOwnershipInterface* ownershipService = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipService, &EditorEntityContextRequests::GetWorldEntityOwnershipService, prefabWorld);
        ASSERT_NE(ownershipService, nullptr);

        InstanceOptionalReference rootOfPrefabWorld = ownershipService->GetRootPrefabInstance();
        ASSERT_TRUE(rootOfPrefabWorld.has_value());

        const AZ::EntityId rootContainer = rootOfPrefabWorld->get().GetContainerEntityId();
        ASSERT_TRUE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(rootContainer));

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::ResetEditorContext);

        EXPECT_TRUE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(rootContainer))
            << "The outliner model dropped the prefab editor world's root prefab when the editor world reset";

        AzToolsFramework::EditorEntityContextNotificationBus::Broadcast(
            &AzToolsFramework::EditorEntityContextNotification::OnEntityStreamLoadBegin);

        EXPECT_TRUE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(rootContainer))
            << "The outliner model dropped the prefab editor world's root prefab when the editor world loaded a level";

        AZ::EntityId rootParent = AZ::EntityId(AZ::EntityId::InvalidEntityId);
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            rootParent, rootContainer, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
        EXPECT_FALSE(rootParent.IsValid());

        AzToolsFramework::EntityIdList rootsInOutliner;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            rootsInOutliner, AZ::EntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);
        EXPECT_NE(AZStd::find(rootsInOutliner.begin(), rootsInOutliner.end(), rootContainer), rootsInOutliner.end())
            << "The prefab editor world's root prefab is no longer a root entry in the outliner";
    }

    TEST_F(MultiWorldTests, DestroyingASecondaryWorldKeepsInstancesOfThatPrefabInOtherWorlds)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        ASSERT_FALSE(worldA.IsNull());
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);

        PrefabEditorEntityOwnershipInterface* ownershipServiceForWorldA = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipServiceForWorldA, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        ASSERT_NE(ownershipServiceForWorldA, nullptr);

        InstanceOptionalReference rootOfWorldA = ownershipServiceForWorldA->GetRootPrefabInstance();
        ASSERT_TRUE(rootOfWorldA.has_value());

        AZ::EntityId nestedContainerInWorldA;
        rootOfWorldA->get().GetNestedInstances(
            [&nestedContainerInWorldA](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedContainerInWorldA = nestedInstance->GetContainerEntityId();
            });
        ASSERT_TRUE(nestedContainerInWorldA.IsValid());
        ASSERT_TRUE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(nestedContainerInWorldA));

        AzFramework::EntityContextId prefabWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            prefabWorld, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(NestedPathA));
        ASSERT_FALSE(prefabWorld.IsNull());
        ASSERT_NE(prefabWorld, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportB, prefabWorld);

        const AZ::IO::Path nestedPath = m_prefabLoaderInterface->GenerateRelativePath(AZ::IO::PathView(NestedPathA));
        const TemplateId nestedTemplateId = m_prefabSystemComponent->GetTemplateIdFromFilePath(nestedPath);
        ASSERT_NE(nestedTemplateId, InvalidTemplateId);

        EditorEntityContextRequestBus::Broadcast(
            &EditorEntityContextRequests::BindViewportToWorld, ViewportB, AzFramework::EntityContextId::CreateNull());
        PropagateAllTemplateChanges();

        EXPECT_NE(m_prefabSystemComponent->GetTemplateIdFromFilePath(nestedPath), InvalidTemplateId)
            << "Closing the prefab editor's world removed the shared template of the prefab it was editing";
        EXPECT_TRUE(AzToolsFramework::EditorEntityInfoRequestBus::HasHandlers(nestedContainerInWorldA))
            << "Closing the prefab editor's world deleted that prefab's instance from another world's level";

        AZ::EntityId nestedContainerAfterClose;
        rootOfWorldA->get().GetNestedInstances(
            [&nestedContainerAfterClose](AZStd::unique_ptr<Instance>& nestedInstance)
            {
                nestedContainerAfterClose = nestedInstance->GetContainerEntityId();
            });
        EXPECT_EQ(nestedContainerAfterClose, nestedContainerInWorldA)
            << "The level's nested prefab instance did not survive closing the prefab editor's world";
    }

    TEST_F(MultiWorldTests, ResettingTheEditorContextLeavesTheOtherWorldsIntact)
    {
        AzFramework::EntityContextId worldA = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            worldA, &EditorEntityContextRequests::LoadWorld, AZ::IO::PathView(LevelPathA));
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::BindViewportToWorld, ViewportA, worldA);
        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::SetFocusedViewport, ViewportA);

        PrefabEditorEntityOwnershipInterface* ownershipServiceBeforeReset = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipServiceBeforeReset, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        ASSERT_NE(ownershipServiceBeforeReset, nullptr);

        const TemplateId worldATemplateId = ownershipServiceBeforeReset->GetRootPrefabTemplateId();
        ASSERT_NE(worldATemplateId, InvalidTemplateId);
        ASSERT_TRUE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(worldA));

        EditorEntityContextRequestBus::Broadcast(&EditorEntityContextRequests::ResetEditorContext);

        AzFramework::EntityContextId viewportWorld = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(
            viewportWorld, &EditorEntityContextRequests::GetViewportWorld, ViewportA);
        EXPECT_EQ(viewportWorld, worldA);

        PrefabEditorEntityOwnershipInterface* ownershipServiceAfterReset = nullptr;
        EditorEntityContextRequestBus::BroadcastResult(
            ownershipServiceAfterReset, &EditorEntityContextRequests::GetWorldEntityOwnershipService, worldA);
        ASSERT_NE(ownershipServiceAfterReset, nullptr);

        EXPECT_TRUE(m_prefabSystemComponent->FindTemplate(worldATemplateId).has_value());
        EXPECT_EQ(ownershipServiceAfterReset->GetRootPrefabTemplateId(), worldATemplateId);
        EXPECT_TRUE(ownershipServiceAfterReset->GetRootPrefabInstance().has_value());
        EXPECT_TRUE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(worldA));
    }
} // namespace UnitTest
