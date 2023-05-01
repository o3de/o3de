/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MockInterfaces.h>
#include <RecastNavigationEditorSystemComponent.h>
#include <API/EditorPythonConsoleBus.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Console/Console.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <Components/DetourNavigationComponent.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <Components/RecastNavigationPhysXProviderComponent.h>
#include <EditorComponents/EditorDetourNavigationComponent.h>
#include <EditorComponents/EditorRecastNavigationMeshComponent.h>
#include <EditorComponents/EditorRecastNavigationPhysXProviderComponent.h>
#include <PhysX/MockPhysicsShape.h>
#include <PhysX/MockSceneInterface.h>
#include <PhysX/MockSimulatedBody.h>

namespace RecastNavigation
{
    using testing::_;
    using testing::Invoke;
    using testing::NiceMock;
    using testing::Return;
    using AZStd::unique_ptr;
    using AZ::Entity;
    using AZ::EventSchedulerSystemComponent;
    using RecastNavigation::RecastNavigationMeshRequestBus;
    using RecastNavigation::RecastNavigationMeshRequests;
    using RecastNavigation::RecastNavigationDebugDraw;
    using RecastNavigation::DetourNavigationRequests;
    using RecastNavigation::NavMeshQuery;
    using RecastNavigation::DetourNavigationComponent;
    using RecastNavigation::DetourNavigationRequestBus;
    using RecastNavigationTests::Wait;
    using RecastNavigationTests::MockShapeComponent;

    class EditorNavigationTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        unique_ptr<AZ::SerializeContext> m_sc;
        unique_ptr<AZ::BehaviorContext> m_bc;
        unique_ptr<AZStd::vector<AZ::ComponentDescriptor*>> m_descriptors;
        unique_ptr<AZ::MockTimeSystem> m_timeSystem;
        unique_ptr<UnitTest::MockSceneInterface> m_mockSceneInterface;
        unique_ptr<AzPhysics::SceneQueryHit> m_hit;
        unique_ptr<UnitTest::MockPhysicsShape> m_mockPhysicsShape;
        unique_ptr<UnitTest::MockSimulatedBody> m_mockSimulatedBody;
        unique_ptr<AZ::Console> m_console;

        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());

            // register components involved in testing
            m_descriptors = AZStd::make_unique<AZStd::vector<AZ::ComponentDescriptor*>>();
            m_sc = AZStd::make_unique<AZ::SerializeContext>();
            m_sc->CreateEditContext();
            m_bc = AZStd::make_unique<AZ::BehaviorContext>();

            RegisterComponent<RecastNavigationMeshComponent>();
            RegisterComponent<EditorRecastNavigationMeshComponent>();

            RegisterComponent<RecastNavigationPhysXProviderComponent>();
            RegisterComponent<EditorRecastNavigationPhysXProviderComponent>();

            RegisterComponent<DetourNavigationComponent>();
            RegisterComponent<EditorDetourNavigationComponent>();

            RegisterComponent<MockShapeComponent>();
            RegisterComponent<EventSchedulerSystemComponent>();
            RegisterComponent<RecastNavigationEditorSystemComponent>();

            m_timeSystem = AZStd::make_unique<NiceMock<AZ::MockTimeSystem>>();
            m_mockSceneInterface = AZStd::make_unique<NiceMock<UnitTest::MockSceneInterface>>();
            m_hit = AZStd::make_unique<AzPhysics::SceneQueryHit>();
            m_mockPhysicsShape = AZStd::make_unique<NiceMock<UnitTest::MockPhysicsShape>>();
            m_mockSimulatedBody = AZStd::make_unique<NiceMock<UnitTest::MockSimulatedBody>>();
        }

        void TearDown() override
        {
            m_mockSimulatedBody.reset();
            m_mockPhysicsShape.reset();
            m_hit.reset();
            m_mockSceneInterface.reset();
            m_timeSystem.reset();

            for (AZ::ComponentDescriptor* descriptor : *m_descriptors)
            {
                delete descriptor;
            }
            m_descriptors.reset();

            m_sc.reset();
            m_bc.reset();

            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();
            UnitTest::LeakDetectionFixture::TearDown();
        }


        template <typename T>
        void RegisterComponent()
        {
            AZ::ComponentDescriptor* item = T::CreateDescriptor();
            item->Reflect(m_sc.get());
            item->Reflect(m_bc.get());

            m_descriptors->push_back(item);
        }

        // helper method
        void PopulateEntity(Entity& e)
        {
            e.SetId(AZ::EntityId{ 1 });
            e.CreateComponent<EventSchedulerSystemComponent>();
            e.CreateComponent<RecastNavigationEditorSystemComponent>();
            m_mockShapeComponent = e.CreateComponent<MockShapeComponent>();
            e.CreateComponent<EditorRecastNavigationPhysXProviderComponent>(RecastNavigationPhysXProviderConfig{});
            e.CreateComponent<EditorDetourNavigationComponent>();

            m_editorRecastNavigationMeshComponent = e.CreateComponent<EditorRecastNavigationMeshComponent>(RecastNavigationMeshConfig{});
            m_editorRecastNavigationMeshComponent->SetEditorPreview(true);
        }

        void SetupNavigationMesh()
        {
            m_hit->m_resultFlags = AzPhysics::SceneQuery::EntityId;
            m_hit->m_entityId = AZ::EntityId{ 1 };
            m_hit->m_shape = m_mockPhysicsShape.get();

            // Fake result when querying PhysX world.
            ON_CALL(*m_mockSceneInterface, QueryScene(_, _)).WillByDefault(Invoke([this]
            (AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequest* request)
                {
                    const AzPhysics::OverlapRequest* overlapRequest = static_cast<const AzPhysics::OverlapRequest*>(request);
                    overlapRequest->m_unboundedOverlapHitCallback({ *m_hit });
                    return AzPhysics::SceneQueryHits();
                }));

            // Fake a simulated body within query results.
            ON_CALL(*m_mockSceneInterface, GetSimulatedBodyFromHandle(_, _)).WillByDefault(Invoke([this]
            (AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle)
                {
                    return m_mockSimulatedBody.get();
                }));
            // Provide a position and an orientation of a simulated body.
            ON_CALL(*m_mockSimulatedBody, GetOrientation()).WillByDefault(Return(AZ::Quaternion::CreateIdentity()));
            ON_CALL(*m_mockSimulatedBody, GetPosition()).WillByDefault(Return(AZ::Vector3::CreateZero()));
        }

        void ActivateEntity(Entity& e)
        {
            // Bring the entity online
            e.Init();
            e.Activate();
        }

        MockShapeComponent* m_mockShapeComponent = nullptr;
        EditorRecastNavigationMeshComponent* m_editorRecastNavigationMeshComponent = nullptr;

        // Test data
        void AddTestGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, bool indexed = true)
        {
            constexpr float size = 2.5f;
            const AZStd::vector<AZ::Vector3> boxVertices = {
                AZ::Vector3(-size, -size, -size),
                AZ::Vector3(size, -size, -size) ,
                AZ::Vector3(size, size, -size)  ,
                AZ::Vector3(-size, size, -size) ,
                AZ::Vector3(-size, -size, size) ,
                AZ::Vector3(size, -size, size)  ,
                AZ::Vector3(size, size, size)   ,
                AZ::Vector3(-size, size, size)
            };
            vertices.clear();
            vertices.insert(vertices.begin(), boxVertices.begin(), boxVertices.end());

            indices.clear();
            if (indexed)
            {
                const AZStd::vector<AZ::u32> boxIndices = {
                    /*0*/ 2,                    /*1*/     1,                    /*2*/     0,
                    /*3*/ 0,                    /*4*/     3,                    /*5*/     2,
                    /*6*/ 3,                    /*7*/     0,                    /*8*/     7,
                    /*9*/ 0,                    /*10*/ 4,                    /*11*/ 7,
                    /*12*/ 0,                    /*13*/ 1,                    /*14*/ 5,
                    /*15*/ 0,                    /*16*/ 5,                    /*17*/ 4,
                    /*18*/ 1,                    /*19*/ 2,                    /*20*/ 5,
                    /*21*/ 6,                    /*22*/ 5,                    /*23*/ 2,
                    /*24*/ 7,                    /*25*/ 2,                    /*26*/ 3,
                    /*27*/ 7,                    /*28*/ 6,                    /*29*/ 2,
                    /*30*/ 7,                    /*31*/ 4,                    /*32*/ 5,
                    /*33*/ 7,                    /*34*/ 5,                    /*35*/ 6,
                };
                indices.insert(indices.begin(), boxIndices.begin(), boxIndices.end());

                indices.push_back(2);
                indices.push_back(1);
                indices.push_back(0);
            }
        }

        void SetEditorMeshConfig(EditorRecastNavigationMeshComponent* component, bool autoUpdate)
        {
            component->SetEditorPreview(autoUpdate);

            component->OnConfigurationChanged();
        }

        void Tick(float time = 0.1f)
        {
            AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, time, AZ::ScriptTimePoint{});
        }

        void AddTestGeometry(bool indexed)
        {
            ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this, indexed]
            (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
                {
                    AddTestGeometry(vertices, indices, indexed);
                }));
        }
    };

    TEST_F(EditorNavigationTest, InEditorUpdateTick)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        AddTestGeometry(true);

        const Wait wait(AZ::EntityId(1));
        m_editorRecastNavigationMeshComponent->OnEditorUpdateTick();

        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_updatedCalls, 1);
    }

    TEST_F(EditorNavigationTest, InEditorDebugDrawTick)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        AddTestGeometry(true);

        const Wait wait(AZ::EntityId(1));
        m_editorRecastNavigationMeshComponent->OnEditorUpdateTick();

        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_updatedCalls, 1);

        Tick();
    }

    TEST_F(EditorNavigationTest, InEditorDebugDrawTickStopDebugDraw)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        AddTestGeometry(true);

        const Wait wait(AZ::EntityId(1));
        m_editorRecastNavigationMeshComponent->OnEditorUpdateTick();

        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_updatedCalls, 1);

        Tick();

        SetEditorMeshConfig(m_editorRecastNavigationMeshComponent, false);
    }

    TEST_F(EditorNavigationTest, InEditorSecondRun)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        AddTestGeometry(true);

        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 1500 }));
        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilCalled();
            EXPECT_EQ(wait.m_updatedCalls, 1);
        }

        // Advance time forward.
        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 3500 }));
        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilCalled();
            EXPECT_EQ(wait.m_updatedCalls, 1);
        }
    }

    TEST_F(EditorNavigationTest, InEditorEmptyWorld)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        const Wait wait(AZ::EntityId(1));
        m_editorRecastNavigationMeshComponent->OnEditorUpdateTick();

        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_updatedCalls, 1);
    }

    TEST_F(EditorNavigationTest, DeactivateRightAfterUpdateEvent)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();
        AddTestGeometry(true);

        const Wait wait(AZ::EntityId(1));
        m_editorRecastNavigationMeshComponent->OnEditorUpdateTick();

        wait.BlockUntilNavigationMeshRecalculating(AZ::TimeMs{ 100 });
        EXPECT_EQ(wait.m_recalculatingCalls, 1);

        e.Deactivate(); // The expectation is that that update is running on a thread as we deactivate here.

        wait.BlockUntilCalled(AZ::TimeMs{ 100 });
        EXPECT_EQ(wait.m_updatedCalls, 0);
    }

    TEST_F(EditorNavigationTest, BuildGameEntityFromEditorRecastNavigationPhysXProviderComponent)
    {
        Entity inEntity;
        auto* inComponent = inEntity.CreateComponent<EditorRecastNavigationPhysXProviderComponent>();

        Entity outEntity;
        inComponent->BuildGameEntity(&outEntity);

        EXPECT_NE(outEntity.FindComponent<RecastNavigation::RecastNavigationPhysXProviderComponent>(), nullptr);
    }

    TEST_F(EditorNavigationTest, BuildGameEntityFromEditorRecastNavigationMeshComponent)
    {
        Entity inEntity;
        auto* inComponent = inEntity.CreateComponent<EditorRecastNavigationMeshComponent>();

        Entity outEntity;
        inComponent->BuildGameEntity(&outEntity);

        EXPECT_NE(outEntity.FindComponent<RecastNavigation::RecastNavigationMeshComponent>(), nullptr);
    }

    TEST_F(EditorNavigationTest, BuildGameEntityFromEditorDetourNavigationComponent)
    {
        Entity inEntity;
        auto* inComponent = inEntity.CreateComponent<EditorDetourNavigationComponent>();

        Entity outEntity;
        inComponent->BuildGameEntity(&outEntity);

        EXPECT_NE(outEntity.FindComponent<DetourNavigationComponent>(), nullptr);
    }

    TEST_F(EditorNavigationTest, ActivateDeactivateThenTickToPreviewEditor)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        AddTestGeometry(true);

        e.Deactivate();
        e.Activate();

        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 1500 }));
        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilCalled();
            EXPECT_EQ(wait.m_updatedCalls, 1);
        }
    }

    TEST_F(EditorNavigationTest, ActivateRunThenDeactivateThenTickToPreviewEditor)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        AddTestGeometry(true);

        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 1500 }));
        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilCalled();
            EXPECT_EQ(wait.m_updatedCalls, 1);
        }

        e.Deactivate();
        e.Activate();

        // Advance time forward.
        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 3500 }));
        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilCalled();
            EXPECT_EQ(wait.m_updatedCalls, 1);
        }
    }

    TEST_F(EditorNavigationTest, DeactivateRightAfterRecalculatingEventThenActivateAndPreviewEditor)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();
        AddTestGeometry(true);

        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 1500 }));
        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilNavigationMeshRecalculating(AZ::TimeMs{ 100 });
            EXPECT_EQ(wait.m_recalculatingCalls, 1);
        }

        e.Deactivate(); // The expectation is that that update is running on a thread as we deactivate here.
        e.Activate();

        // Advance time forward.
        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 3500 }));
        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilCalled();
            EXPECT_EQ(wait.m_updatedCalls, 1);
        }
    }

    TEST_F(EditorNavigationTest, StartAsyncThenChangedNavigationMeshSettings)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();
        AddTestGeometry(true);

        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 1500 }));
        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilNavigationMeshRecalculating(AZ::TimeMs{ 100 });
            EXPECT_EQ(wait.m_recalculatingCalls, 1);
        }

        // This forces a rebuild of the navigation mesh as the configuration changed.
        m_editorRecastNavigationMeshComponent->OnConfigurationChanged();
        e.Deactivate();
    }

    TEST_F(EditorNavigationTest, AsyncThenChangeSettingsThenAsyncAgain)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();
        AddTestGeometry(true);

        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{ 1500 }));

        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilNavigationMeshRecalculating(AZ::TimeMs{ 100 });
            EXPECT_EQ(wait.m_recalculatingCalls, 1);
        }

        // This forces a rebuild of the navigation mesh as the configuration changed.
        m_editorRecastNavigationMeshComponent->OnConfigurationChanged();

        {
            const Wait wait(AZ::EntityId(1));
            Tick();
            wait.BlockUntilCalled(AZ::TimeMs{ 100 });
            EXPECT_EQ(wait.m_updatedCalls, 0);
        }
    }
}
