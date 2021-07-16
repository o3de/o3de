/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhysXTestFixtures.h"
#include "PhysXTestUtil.h"
#include "PhysXTestCommon.h"

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Physics/Utils.h>
#include <AzCore/Math/MathReflection.h>
#include <AzFramework/Entity/EntityContext.h>

namespace PhysX
{
    static AZStd::map<AZStd::string, EntityPtr> s_testEntities;

    AZ::EntityId GetTestEntityId(const char* name)
    {
        if (auto it = s_testEntities.find(name);
            it != s_testEntities.end())
        {
            return it->second->GetId();
        }

        return AZ::EntityId();
    }

    // this allows EXPECT_TRUE to be exposed to the behavior context and used inside blocks of lua code which
    // are executed in tests
    void ExpectTrue(bool check)
    {
        EXPECT_TRUE(check);
    }

    class PhysXScriptTest
        : public PhysXDefaultWorldTest
    {
    public:
        AZ_TYPE_INFO(PhysXScriptTest, "{337A9DB4-ACF7-42A7-92E5-48A9FF14B49C}");

        void SetUp() override
        {
            PhysXDefaultWorldTest::SetUp();

            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            AZ::Entity::Reflect(m_behaviorContext.get());
            AZ::MathReflect(m_behaviorContext.get());
            AzFramework::EntityContext::Reflect(m_behaviorContext.get());
            Physics::ReflectionUtils::ReflectPhysicsApi(m_behaviorContext.get());
            m_behaviorContext->Method("ExpectTrue", &ExpectTrue);
            m_behaviorContext->Method("GetTestEntityId", &GetTestEntityId);
            m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
            m_scriptContext->BindTo(m_behaviorContext.get());
        }

        void TearDown() override
        {
            s_testEntities.clear();

            m_scriptContext.reset();
            m_behaviorContext.reset();

            PhysXDefaultWorldTest::TearDown();
        }

        AZ::BehaviorContext* GetBehaviorContext()
        {
            return m_behaviorContext.get();
        }

        AZ::ScriptContext* GetScriptContext()
        {
            return m_scriptContext.get();
        }

    private:
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext;
    };

    TEST_F(PhysXScriptTest, SimulatedBodyRaycast_RaycastNotIntersectingBox_ReturnsNoHits)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                request = RayCastRequest()
                request.Start = Vector3(5.0, 0.0, 5.0)
                request.Direction = Vector3(0.0, 0.0, -1.0)
                request.Distance = 10.0
                hit = SimulatedBodyComponentRequestBus.Event.RayCast(boxId, request)
                ExpectTrue(hit.EntityId == EntityId())
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, SimulatedBodyRaycast_RaycastIntersectingBox_ReturnsHitOnBox)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                request = RayCastRequest()
                request.Start = Vector3(0.0, 0.0, 5.0)
                request.Direction = Vector3(0.0, 0.0, -1.0)
                request.Distance = 10.0
                hit = SimulatedBodyComponentRequestBus.Event.RayCast(boxId, request)
                ExpectTrue(hit.EntityId == boxId)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, SimulatedBodyRaycast_RaycastNonInteractingCollisionFilters_ReturnsNoHits)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                request = RayCastRequest()
                request.Start = Vector3(0.0, 0.0, 5.0)
                request.Direction = Vector3(0.0, 0.0, -1.0)
                request.Distance = 10.0
                request.Collision = CollisionGroup("None")
                hit = SimulatedBodyComponentRequestBus.Event.RayCast(boxId, request)
                ExpectTrue(hit.EntityId == EntityId())
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, SceneRayCast_RaycastNotIntersectingBox_ReturnsNoHits)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)
                request = RayCastRequest()
                request.Start = Vector3(5.0, 0.0, 5.0)
                request.Direction = Vector3(0.0, 0.0, -1.0)
                request.Distance = 10.0
                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 0)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, SceneRayCast_RaycastIntersectingBox_ReturnsHitOnBox)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)
                request = RayCastRequest()
                request.Start = Vector3(0.0, 0.0, 5.0)
                request.Direction = Vector3(0.0, 0.0, -1.0)
                request.Distance = 10.0
                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 1)
                hit = hits.HitArray[1] -- lua uses 1-indexing
                ExpectTrue(hit.EntityId == boxId)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, SceneRayCast_MultipleHitRaycastIntersectingBoxes_ReturnsMultipleHits)
    {
        s_testEntities.insert(
            {
                "Box1",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box1")
            });
        s_testEntities.insert(
            {
                "Box2",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateAxisZ(-5.0f), "Box2")
            });

        const char luaCode[] =
            R"(
                box1Id = GetTestEntityId("Box1")
                box2Id = GetTestEntityId("Box2")
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)
                request = RayCastRequest()
                request.Start = Vector3(0.0, 0.0, 5.0)
                request.Direction = Vector3(0.0, 0.0, -1.0)
                request.Distance = 10.0
                request.ReportMultipleHits = true
                hits = scene:QueryScene(request)
                numHits = hits.HitArray:Size()
                box1Hit = false
                box2Hit = false
                for hitIndex = 1, numHits do
                    box1Hit = box1Hit or hits.HitArray[hitIndex].EntityId == box1Id
                    box2Hit = box2Hit or hits.HitArray[hitIndex].EntityId == box2Id
                end
                ExpectTrue(box1Hit)
                ExpectTrue(box2Hit)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, SceneRaycast_RaycastNonInteractingCollisionFilters_ReturnsNoHits)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)
                request = RayCastRequest()
                request.Start = Vector3(0.0, 0.0, 5.0)
                request.Direction = Vector3(0.0, 0.0, -1.0)
                request.Distance = 10.0
                request.Collision = CollisionGroup("None")
                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 0)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, BoxCast_NotIntersectingBox_ReturnsNoHits)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                boxDimensions = Vector3(1.0, 1.0, 1.0)
                startPose = Transform.CreateTranslation(Vector3(0.0, 0.0, 5.0))
                direction = Vector3(-1.0, 0.0, 0.0)
                distance = 10.0
                queryType = 0
                collisionGroup = CollisionGroup("All")
                request = CreateBoxCastRequest(boxDimensions, startPose, direction, distance, queryType, collisionGroup)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 0)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, BoxCast_IntersectingBox_ReturnsHitOnBox)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                boxDimensions = Vector3(1.0, 1.0, 1.0)
                startPose = Transform.CreateTranslation(Vector3(0.0, 0.0, 5.0))
                direction = Vector3(0.0, 0.0, -1.0)
                distance = 10.0
                queryType = 0
                collisionGroup = CollisionGroup("All")
                request = CreateBoxCastRequest(boxDimensions, startPose, direction, distance, queryType, collisionGroup)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 1)
                hit = hits.HitArray[1] -- lua uses 1-indexing
                ExpectTrue(hit.EntityId == boxId)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, BoxCast_NonInteractingCollisionFilters_ReturnsNoHits)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                boxDimensions = Vector3(1.0, 1.0, 1.0)
                startPose = Transform.CreateTranslation(Vector3(0.0, 0.0, 5.0))
                direction = Vector3(0.0, 0.0, -1.0)
                distance = 10.0
                queryType = 0
                collisionGroup = CollisionGroup("None")
                request = CreateBoxCastRequest(boxDimensions, startPose, direction, distance, queryType, collisionGroup)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 0)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, SphereCast_IntersectingBox_ReturnsHitOnBox)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                radius = 2.0
                startPose = Transform.CreateTranslation(Vector3(0.0, 0.0, 5.0))
                direction = Vector3(0.0, 0.0, -1.0)
                distance = 10.0
                queryType = 0
                collisionGroup = CollisionGroup("All")
                request = CreateSphereCastRequest(radius, startPose, direction, distance, queryType, collisionGroup)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 1)
                hit = hits.HitArray[1] -- lua uses 1-indexing
                ExpectTrue(hit.EntityId == boxId)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, CapsuleCast_IntersectingBox_ReturnsHitOnBox)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                radius = 0.5
                height = 2.0
                startPose = Transform.CreateTranslation(Vector3(0.0, 0.0, 5.0))
                direction = Vector3(0.0, 0.0, -1.0)
                distance = 10.0
                queryType = 0
                collisionGroup = CollisionGroup("All")
                request = CreateCapsuleCastRequest(radius, height, startPose, direction, distance, queryType, collisionGroup)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 1)
                hit = hits.HitArray[1] -- lua uses 1-indexing
                ExpectTrue(hit.EntityId == boxId)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, BoxOverlap_NotIntersectingBox_ReturnsNoHits)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                boxDimensions = Vector3(1.0, 1.0, 1.0)
                pose = Transform.CreateTranslation(Vector3(0.0, 0.0, 5.0))
                request = CreateBoxOverlapRequest(boxDimensions, pose)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 0)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, BoxOverlap_IntersectingBox_ReturnsHitOnBox)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                boxDimensions = Vector3(1.0, 1.0, 1.0)
                pose = Transform.CreateTranslation(Vector3(0.0, 0.0, 0.0))
                request = CreateBoxOverlapRequest(boxDimensions, pose)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 1)
                hit = hits.HitArray[1] -- lua uses 1-indexing
                ExpectTrue(hit.EntityId == boxId)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, SphereOverlap_IntersectingBox_ReturnsHitOnBox)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                radius = 0.5
                pose = Transform.CreateTranslation(Vector3(0.0, 0.0, 0.0))
                request = CreateSphereOverlapRequest(radius, pose)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 1)
                hit = hits.HitArray[1] -- lua uses 1-indexing
                ExpectTrue(hit.EntityId == boxId)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }

    TEST_F(PhysXScriptTest, CapsuleOverlap_IntersectingBox_ReturnsHitOnBox)
    {
        s_testEntities.insert(
            {
                "Box",
                TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(GetDefaultSceneHandle(), AZ::Vector3::CreateZero(), "Box")
            });

        const char luaCode[] =
            R"(
                boxId = GetTestEntityId("Box")
                physicsSystem = GetPhysicsSystem()
                sceneHandle = physicsSystem:GetSceneHandle(DefaultPhysicsSceneName)
                scene = physicsSystem:GetScene(sceneHandle)

                height = 2.0
                radius = 0.5
                pose = Transform.CreateTranslation(Vector3(0.0, 0.0, 0.0))
                request = CreateCapsuleOverlapRequest(height, radius, pose)

                hits = scene:QueryScene(request)
                ExpectTrue(hits.HitArray:Size() == 1)
                hit = hits.HitArray[1] -- lua uses 1-indexing
                ExpectTrue(hit.EntityId == boxId)
            )";

        EXPECT_TRUE(GetScriptContext()->Execute(luaCode));
    }
} // namespace PhysX
