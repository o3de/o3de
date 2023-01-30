/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MockInterfaces.h>
#include <RecastNavigationSystemComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Console/Console.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <Components/DetourNavigationComponent.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <Components/RecastNavigationPhysXProviderComponent.h>
#include <PhysX/MockPhysicsShape.h>
#include <PhysX/MockSceneInterface.h>
#include <PhysX/MockSimulatedBody.h>

namespace RecastNavigationTests
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

    class NavigationTest
        : public ::UnitTest::LeakDetectionFixture
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
        unique_ptr<AZ::NameDictionary> m_nameDictionary;

        void SetUp() override
        {
            ::UnitTest::LeakDetectionFixture::SetUp();

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());

            m_nameDictionary = AZStd::make_unique<AZ::NameDictionary>();
            AZ::Interface<AZ::NameDictionary>::Register(m_nameDictionary.get());

            // register components involved in testing
            m_descriptors = AZStd::make_unique<AZStd::vector<AZ::ComponentDescriptor*>>();
            m_sc = AZStd::make_unique<AZ::SerializeContext>();
            m_sc->CreateEditContext();
            m_bc = AZStd::make_unique<AZ::BehaviorContext>();
            RegisterComponent<RecastNavigation::RecastNavigationMeshComponent>();
            RegisterComponent<RecastNavigation::RecastNavigationPhysXProviderComponent>();
            RegisterComponent<MockShapeComponent>();
            RegisterComponent<AZ::EventSchedulerSystemComponent>();
            RegisterComponent<RecastNavigation::RecastNavigationSystemComponent>();
            RegisterComponent<RecastNavigation::DetourNavigationComponent>();

            m_timeSystem = AZStd::make_unique<NiceMock<AZ::MockTimeSystem>>();
            m_mockSceneInterface = AZStd::make_unique<NiceMock<UnitTest::MockSceneInterface>>();
            m_hit = AZStd::make_unique<AzPhysics::SceneQueryHit>();
            m_mockPhysicsShape = AZStd::make_unique<NiceMock<UnitTest::MockPhysicsShape>>();
            m_mockSimulatedBody = AZStd::make_unique<NiceMock<UnitTest::MockSimulatedBody>>();
        }

        void TearDown() override
        {
            m_mockSimulatedBody = {};
            m_mockPhysicsShape = {};
            m_hit = {};
            m_mockSceneInterface = {};
            m_timeSystem = {};

            for (AZ::ComponentDescriptor* descriptor : *m_descriptors)
            {
                delete descriptor;
            }
            m_descriptors = {};

            m_sc = {};
            m_bc = {};

            AZ::Interface<AZ::NameDictionary>::Unregister(m_nameDictionary.get());
            m_nameDictionary.reset();

            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console = {};
            ::UnitTest::LeakDetectionFixture::TearDown();
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
        void PopulateEntity(AZ::Entity& e)
        {
            e.SetId(AZ::EntityId{ 1 });
            e.CreateComponent<AZ::EventSchedulerSystemComponent>();
            e.CreateComponent<RecastNavigation::RecastNavigationSystemComponent>();
            m_mockShapeComponent = e.CreateComponent<MockShapeComponent>();
            e.CreateComponent<RecastNavigation::RecastNavigationPhysXProviderComponent>();
            e.CreateComponent<RecastNavigation::RecastNavigationMeshComponent>(RecastNavigation::RecastNavigationMeshConfig{});
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
    };

    TEST_F(NavigationTest, GetNativeNavMesh)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        AZStd::shared_ptr<RecastNavigation::NavMeshQuery> navMeshQuery;
        RecastNavigationMeshRequestBus::EventResult(navMeshQuery, e.GetId(), &RecastNavigationMeshRequests::GetNavigationObject);
        RecastNavigation::NavMeshQuery::LockGuard lock(*navMeshQuery);
        /*
         * We updated the navigation mesh using a blocking call. We should have access to the native Recast object now.
         */
        EXPECT_NE(lock.GetNavMesh(), nullptr);
    }

    TEST_F(NavigationTest, TestAgainstEmptyPhysicalBody)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        {
            m_hit->m_resultFlags = AzPhysics::SceneQuery::EntityId;
            m_hit->m_entityId = AZ::EntityId{ 1 };
            m_hit->m_shape = m_mockPhysicsShape.get();

            ON_CALL(*m_mockSceneInterface, QueryScene(_, _)).WillByDefault(Invoke([this]
            (AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequest* request)
                {
                    const AzPhysics::OverlapRequest* overlapRequest = static_cast<const AzPhysics::OverlapRequest*>(request);
                    overlapRequest->m_unboundedOverlapHitCallback({ *m_hit });
                    return AzPhysics::SceneQueryHits();
                }));

            ON_CALL(*m_mockSceneInterface, GetSimulatedBodyFromHandle(_, _)).WillByDefault(Invoke([]
            (AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle)
                {
                    return nullptr; // empty physical body
                }));
        }

        /*
         * Corner case, when a collider doesn't have a physical body for reason. Just don't fail.
         */

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        AZStd::shared_ptr<RecastNavigation::NavMeshQuery> navMeshQuery;
        RecastNavigationMeshRequestBus::EventResult(navMeshQuery, e.GetId(), &RecastNavigationMeshRequests::GetNavigationObject);
        RecastNavigation::NavMeshQuery::LockGuard lock(*navMeshQuery);

        EXPECT_NE(lock.GetNavQuery(), nullptr);
    }

    TEST_F(NavigationTest, BlockingTest)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
        /*
         * Verify the notification EBus is called when a navigation mesh is updated.
         */
        EXPECT_EQ(wait.m_updatedCalls, 1);
    }

    TEST_F(NavigationTest, BlockingTestWithDebugDraw)
    {
        Entity e;
        {
            // custom entity construction
            e.SetId(AZ::EntityId{ 1 });
            e.CreateComponent<EventSchedulerSystemComponent>();
            m_mockShapeComponent = e.CreateComponent<MockShapeComponent>();

            /*
             * There is no way to test debug draw but tell the provider to attempt to debug draw anyway. Just don't crash.
             */
            e.CreateComponent<RecastNavigation::RecastNavigationPhysXProviderComponent>();

            e.CreateComponent<RecastNavigation::RecastNavigationMeshComponent>();
        }
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
    }

    TEST_F(NavigationTest, BlockingNonIndexedWithDebugDraw)
    {
        Entity e;
        {
            // custom entity construction
            e.SetId(AZ::EntityId{ 1 });
            e.CreateComponent<EventSchedulerSystemComponent>();
            m_mockShapeComponent = e.CreateComponent<MockShapeComponent>();

            /*
             * There is no way to test debug draw but tell the provider to attempt to debug draw anyway. Just don't crash.
             */
            e.CreateComponent<RecastNavigation::RecastNavigationPhysXProviderComponent>();

            e.CreateComponent<RecastNavigation::RecastNavigationMeshComponent>();
        }
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                /*
                 * Testing with non-indexed triangle data. No way to verify, though. This test must not crash, though.
                 */
                AddTestGeometry(vertices, indices, false);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
    }

    /*
     * Run update navigation mesh twice with indexed triangle data.
     */
    TEST_F(NavigationTest, BlockingTestRerun)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
    }

    /*
     * Run update navigation mesh twice with no data.
     */
    TEST_F(NavigationTest, BlockingOnEmptyRerun)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
    }

    /*
     * Exercise debug ticking code.
     */
    TEST_F(NavigationTest, BlockingTestNonIndexedGeometry)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, false);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.1f, AZ::ScriptTimePoint{});
    }

    /*
     * Exercise debug ticking code with indexed data.
     */
    TEST_F(NavigationTest, TickingDebugDraw)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        MockDebug debug;
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.1f, AZ::ScriptTimePoint{});
    }

    /*
     * Exercise API rarely used by Recast
     */
    TEST_F(NavigationTest, DirectTestOnDebugDrawQuad)
    {
        RecastNavigationDebugDraw debugDraw;
        MockDebug debug;

        debugDraw.begin(DU_DRAW_QUADS);
        debugDraw.vertex(0, 0, 0, 0, 0, 0);
        debugDraw.vertex(0, 1, 0, 0, 0, 0);
        debugDraw.vertex(1, 1, 0, 0, 0, 0);
        debugDraw.vertex(1, 0, 0, 0, 0, 0);
        debugDraw.end();
    }

    /*
     * Exercise API rarely used by Recast
     */
    TEST_F(NavigationTest, DirectTestOnDebugDrawLines)
    {
        RecastNavigationDebugDraw debugDraw(true);
        MockDebug debug;

        const float pos[] = { 0, 0, 0 };
        const float uv[] = { 0, 0, 0 };
        debugDraw.begin(DU_DRAW_LINES);
        debugDraw.vertex(pos, 0, uv);
        debugDraw.vertex(pos, 0, uv);
        debugDraw.end();
    }

    /*
     * Exercise API rarely used by Recast
     */
    TEST_F(NavigationTest, DirectTestOnDebugDrawWithoutDebugDisplayRequests)
    {
        RecastNavigationDebugDraw debugDraw(true);

        const float pos[] = { 0, 0, 0 };
        const float uv[] = { 0, 0, 0 };
        debugDraw.begin(DU_DRAW_POINTS);
        debugDraw.texture(true);
        debugDraw.vertex(pos, 0, uv);
        debugDraw.end();
    }

    /*
     * Basic find path test.
     */
    TEST_F(NavigationTest, FindPathTestDetaultDetourSettings)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>();
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        AZStd::vector<AZ::Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0, 0), AZ::Vector3(2.f, 2, 0));

        EXPECT_GT(waypoints.size(), 0);
    }

    /*
     * Basic find path test.
     */
    TEST_F(NavigationTest, FindPathTest)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(e.GetId(), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        AZStd::vector<AZ::Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0, 0), AZ::Vector3(2.f, 2, 0));

        EXPECT_GT(waypoints.size(), 0);
    }

    /*
     * Test with one of the point being way outside of the range of the navigation mesh.
     */
    TEST_F(NavigationTest, FindPathToOutOfBoundsDestination)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(e.GetId(), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        AZStd::vector<AZ::Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0, 0), AZ::Vector3(2000.f, 2000, 0));

        EXPECT_EQ(waypoints.size(), 0);
    }

    /*
     * Corner case, test on empty data.
     */
    TEST_F(NavigationTest, FindPathOnEmptyNavMesh)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(AZ::EntityId(1337/*pointing to a non-existing entity*/), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        AZStd::vector<AZ::Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0, 0), AZ::Vector3(2.f, 2, 0));

        EXPECT_EQ(waypoints.size(), 0);
    }

    /*
     * Corner case. Invalid entities.
     */
    TEST_F(NavigationTest, FindPathBetweenInvalidEntities)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(e.GetId(), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        AZStd::vector<AZ::Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenEntities,
            AZ::EntityId(), AZ::EntityId());

        EXPECT_EQ(waypoints.size(), 0);
    }

    /*
     * Corner case. Empty nav mesh.
     */
    TEST_F(NavigationTest, FindPathBetweenEntitiesOnEmptyNavMesh)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(e.GetId(), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        MockTransforms mockTransforms({ AZ::EntityId(1), AZ::EntityId(2) });

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        AZStd::vector<AZ::Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenEntities,
            AZ::EntityId(1), AZ::EntityId(2));

        EXPECT_EQ(waypoints.size(), 0);
    }

    /*
     * Just for code coverage!
     */
    TEST_F(NavigationTest, RecastNavigationMeshComponentControllerTests)
    {
        RecastNavigation::RecastNavigationMeshComponentController common;
        EXPECT_EQ(strcmp(common.TYPEINFO_Name(), "RecastNavigationMeshComponentController"), 0);
    }

    /*
     * Just for code coverage!
     */
    TEST_F(NavigationTest, RecastNavigationNotificationHandler)
    {
        RecastNavigation::RecastNavigationNotificationHandler handler;
        handler.OnNavigationMeshUpdated(AZ::EntityId(1));
    }

    /*
     * Just for code coverage!
     */
    TEST_F(NavigationTest, RecastNavigationPhysXProviderComponentController)
    {
        RecastNavigation::RecastNavigationPhysXProviderComponentController test;
        EXPECT_EQ(strcmp(test.TYPEINFO_Name(), "RecastNavigationPhysXProviderComponentController"), 0);
    }

    TEST_F(NavigationTest, DISABLED_AsyncOnNavigationMeshUpdatedIsCalled)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
        wait.BlockUntilCalled();
    }

    TEST_F(NavigationTest, DISABLED_AsyncDeactivateRightAfterCallingUpdate)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
        // Don't wait, deactivate the entity now.

        /*
         * If everything goes well, the entity will shutdown without a crash. With a bad design,
         * one of tile will be sent to a deactivate component. Note, RecastNavigationMeshComponent deactivates first while
         * RecastNavigationPhysXProviderComponent might still try to send it tile data.
         */
    }

    TEST_F(NavigationTest, DISABLED_AsyncEmpty)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
        wait.BlockUntilCalled();
    }

    // Disabling this test to unblock AR while an investigation is in progress.
    TEST_F(NavigationTest, DISABLED_AsyncRerun)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        for (int i = 1; i <= 2; ++i)
        {
            const Wait wait(AZ::EntityId(1));
            RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
            wait.BlockUntilCalled();
        }
    }

    TEST_F(NavigationTest, DISABLED_AsyncSecondWhileFirstIsInProgress)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
        wait.BlockUntilCalled();

        EXPECT_EQ(wait.m_updatedCalls, 1);
    }

    TEST_F(NavigationTest, DISABLED_AsyncManyUpdatesWhileFirstIsInProgressStressTest)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
        for (int i = 0; i < 9'001; ++i)
        {
            RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
        }
        wait.BlockUntilCalled();

        // Only one of those updates was done.
        EXPECT_EQ(wait.m_updatedCalls, 1);
    }

    TEST_F(NavigationTest, DISABLED_BlockingCallAfterAsync)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
        wait.BlockUntilCalled();

        // Only one of those updates was done.
        EXPECT_EQ(wait.m_updatedCalls, 1);
    }

    TEST_F(NavigationTest, DISABLED_BlockingCallAfterAsyncReturnsFalse)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        bool result = false;
        RecastNavigationMeshRequestBus::EventResult(result, e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);
        EXPECT_EQ(result, true);

        RecastNavigationMeshRequestBus::EventResult(result, e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
        EXPECT_EQ(result, false);
        wait.BlockUntilCalled();
    }

    TEST_F(NavigationTest, DISABLED_FindPathRightAfterUpdateAsync)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(e.GetId(), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);

        AZStd::vector<AZ::Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0.f, 0.f), AZ::Vector3(2.f, 2.f, 0.f));
        // We should not get the path yet since the async update operation is still in progress.
        EXPECT_EQ(waypoints.size(), 0);

        wait.BlockUntilCalled();
    }

    TEST_F(NavigationTest, CollectGeometryCornerCaseZeroTileSize)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(e.GetId(), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        AZStd::vector<AZStd::shared_ptr<RecastNavigation::TileGeometry>> tiles;
        RecastNavigation::RecastNavigationProviderRequestBus::EventResult(tiles, e.GetId(),
            &RecastNavigation::RecastNavigationProviderRequests::CollectGeometry,
            0.f, 0.f);

        EXPECT_EQ(tiles.size(), 0);
    }

    TEST_F(NavigationTest, DetourSetNavMeshEntity)
    {
        Entity e;
        PopulateEntity(e);
        DetourNavigationComponent* detour = e.CreateComponent<DetourNavigationComponent>();
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        detour->SetNavigationMeshEntity(AZ::EntityId(999)/*Doesn't exist*/);
        AZStd::vector<AZ::Vector3> waypoints = detour->FindPathBetweenPositions(AZ::Vector3(0.f, 0.f, 0.f), AZ::Vector3(2.f, 2.f, 0.f));
        EXPECT_EQ(waypoints.size(), 0);

        detour->SetNavigationMeshEntity(AZ::EntityId(1)/*The right entity*/);
        waypoints = detour->FindPathBetweenPositions(AZ::Vector3(0.f, 0.f, 0.f), AZ::Vector3(2.f, 2.f, 0.f));
        EXPECT_GE(waypoints.size(), 1);
    }

    TEST_F(NavigationTest, NavUpdateThenDeleteCollidersThenUpdateAgainThenFindPathShouldFail)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(e.GetId(), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, const AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));
        
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        AZStd::vector<AZ::Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0.f, 0.f), AZ::Vector3(2.f, 2.f, 0.f));
        EXPECT_GT(waypoints.size(), 1);

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([]
        (
            [[maybe_unused]] AZStd::vector<AZ::Vector3>& vertices,
            [[maybe_unused]] AZStd::vector<AZ::u32>& indices,
            [[maybe_unused]] const AZ::Aabb*)
            {
                // Act as if there colliders are gone.
            }));
        
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        waypoints.clear();
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0.f, 0.f), AZ::Vector3(2.f, 2.f, 0.f));
        EXPECT_EQ(waypoints.size(), 0);
    }
}
