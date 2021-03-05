/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/World.h>
#include <PhysX/ColliderComponentBus.h>

namespace Physics
{
    static auto GetEntityInRayCastHitCallBack = [](AZ::EntityId entityId)
    {
        return [entityId](const RayCastHit& hit)
        {
            return hit.m_body->GetEntityId() == entityId;
        };
    };

    TEST_F(GenericPhysicsInterfaceTest, World_CreateNewWorld_ReturnsNewWorld)
    {
        EXPECT_TRUE(CreateTestWorld() != nullptr);
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstNothing_ReturnsNoHits)
    {
        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        RayCastHit hit;
        WorldRequestBus::BroadcastResult(hit, &WorldRequests::RayCast, request);

        EXPECT_FALSE(hit);
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstSphere_ReturnsHits)
    {
        auto sphereEntity = AddSphereEntity(AZ::Vector3(0.0f), 10.0f);

        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        RayCastHit hit;
        WorldRequestBus::BroadcastResult(hit, &WorldRequests::RayCast, request);

        EXPECT_TRUE(hit);

        bool hitsIncludeSphereEntity = (hit.m_body->GetEntityId() == sphereEntity->GetId());
        EXPECT_TRUE(hitsIncludeSphereEntity);

        delete sphereEntity;
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstSphere_ReturnsCorrectShapeAndMaterial)
    {
        auto sphereEntity = AZStd::shared_ptr<AZ::Entity>(AddSphereEntity(AZ::Vector3(0.0f), 10.0f));

        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        RayCastHit result;
        WorldRequestBus::BroadcastResult(result, &WorldRequests::RayCast, request);

        ASSERT_TRUE(result);

        RigidBody* rigidBody;
        Physics::RigidBodyRequestBus::EventResult(rigidBody, sphereEntity->GetId(), &RigidBodyRequestBus::Events::GetRigidBody);

        ASSERT_NE(rigidBody->GetShape(0), nullptr);
        ASSERT_NE(result.m_material, nullptr);
        ASSERT_EQ(result.m_shape, rigidBody->GetShape(0).get());
        ASSERT_EQ(result.m_material, rigidBody->GetShape(0).get()->GetMaterial().get());

        const AZStd::string& typeName = result.m_material->GetSurfaceTypeName();
        ASSERT_EQ(typeName, AZStd::string("Default"));
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstStaticObject_ReturnsHits)
    {
        auto boxEntity = AZStd::shared_ptr<AZ::Entity>(AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(10.0f, 10.0f, 10.0f)));

        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        RayCastHit result;
        WorldRequestBus::BroadcastResult(result, &WorldRequests::RayCast, request);

        EXPECT_TRUE(result);

        bool hitsIncludeEntity = (result.m_body->GetEntityId() == boxEntity->GetId());
        EXPECT_TRUE(hitsIncludeEntity);
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_CastAgainstFilteredSpheres_ReturnsHits)
    {
        auto entity1 = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f, CollisionLayer(0));
        auto entity2 = AddCapsuleEntity(AZ::Vector3(0.0f, 0.0f, 20.0f), 10.0f, 2.0f, CollisionLayer(1));
        auto entity3 = AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 30.0f), AZ::Vector3(20.0f, 20.0f, 20.0f), CollisionLayer(2));

        CollisionGroup group = CollisionGroup::All;
        group.SetLayer(CollisionLayer(0), true);
        group.SetLayer(CollisionLayer(1), false);
        group.SetLayer(CollisionLayer(2), true);

        RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
        request.m_distance = 200.0f;
        request.m_collisionGroup = group;

        AZStd::vector<RayCastHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::RayCastMultiple, request);

        ASSERT_TRUE(hits.size() == 2);
        EXPECT_TRUE(hits[1].m_body->GetEntityId() == entity1->GetId());
        EXPECT_TRUE(hits[0].m_body->GetEntityId() == entity3->GetId());

        delete entity1;
        delete entity2;
        delete entity3;
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_AgainstStaticOnly_ReturnsStaticBox)
    {
        auto dynamicSphere = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f);
        auto staticBox = AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 30.0f), AZ::Vector3(20.0f, 20.0f, 20.0f));

        RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
        request.m_queryType = Physics::QueryType::Static;

        AZStd::vector<RayCastHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::RayCastMultiple, request);

        ASSERT_EQ(hits.size(), 1);
        ASSERT_EQ(hits[0].m_body->GetEntityId(), staticBox->GetId());

        delete dynamicSphere;
        delete staticBox;
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_AgainstDynamicOnly_ReturnsDynamicSphere)
    {
        auto dynamicSphere = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f);
        auto staticBox = AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 30.0f), AZ::Vector3(20.0f, 20.0f, 20.0f));

        RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
        request.m_queryType = Physics::QueryType::Dynamic;

        AZStd::vector<RayCastHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::RayCastMultiple, request);

        ASSERT_EQ(hits.size(), 1);
        ASSERT_EQ(hits[0].m_body->GetEntityId(), dynamicSphere->GetId());

        delete dynamicSphere;
        delete staticBox;
    }

    TEST_F(GenericPhysicsInterfaceTest, RayCast_AgainstStaticAndDynamic_ReturnsBothObjects)
    {
        auto dynamicSphere = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f);
        auto staticBox = AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 30.0f), AZ::Vector3(20.0f, 20.0f, 20.0f));

        RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
        request.m_queryType = Physics::QueryType::StaticAndDynamic;

        AZStd::vector<RayCastHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::RayCastMultiple, request);

        ASSERT_EQ(hits.size(), 2);
        ASSERT_EQ(hits[0].m_body->GetEntityId(), staticBox->GetId());
        ASSERT_EQ(hits[1].m_body->GetEntityId(), dynamicSphere->GetId());

        delete dynamicSphere;
        delete staticBox;
    }
    TEST_F(GenericPhysicsInterfaceTest, RayCast_AgainstMultipleTouchAndBlockHits_ReturnsClosestBlockAndTouches)
    {
        auto dynamicSphere = AddSphereEntity(AZ::Vector3(20.0f, 0.0f, 0.0f), 10.0f);
        auto staticBox = AddStaticBoxEntity(AZ::Vector3(40.0f, 0.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 5.0f));
        auto blockingSphere = AddSphereEntity(AZ::Vector3(60.0f, 0.0f, 0.0f), 5.0f);
        auto blockingBox = AddStaticBoxEntity(AZ::Vector3(80.0f, 0.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 5.0f));
        auto farSphere = AddSphereEntity(AZ::Vector3(120.0f, 0.0f, 0.0f), 10.0f);

        RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_queryType = Physics::QueryType::StaticAndDynamic;
        request.m_filterCallback = [&](const Physics::WorldBody* body, [[maybe_unused]] const Physics::Shape* shape)
        {
            if (body->GetEntityId() == blockingBox->GetId() || body->GetEntityId() == blockingSphere->GetId())
            {
                return QueryHitType::Block;
            }
            return QueryHitType::Touch;
        };

        AZStd::vector<RayCastHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::RayCastMultiple, request);

        ASSERT_EQ(hits.size(), 3);

        ASSERT_EQ(1, AZStd::count_if(hits.begin(), hits.end(), GetEntityInRayCastHitCallBack(dynamicSphere->GetId())));
        ASSERT_EQ(1, AZStd::count_if(hits.begin(), hits.end(), GetEntityInRayCastHitCallBack(staticBox->GetId())));
        ASSERT_EQ(1, AZStd::count_if(hits.begin(), hits.end(), GetEntityInRayCastHitCallBack(blockingSphere->GetId())));

        delete dynamicSphere;
        delete staticBox;
        delete blockingSphere;
        delete blockingBox;
        delete farSphere;
    }

    TEST_F(GenericPhysicsInterfaceTest, ShapeCast_CastAgainstNothing_ReturnsNoHits)
    {
        Physics::RayCastHit hit;
        WorldRequestBus::BroadcastResult(hit, &WorldRequests::SphereCast,
            1.0f, 
            AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f)),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            20.0f, Physics::QueryType::StaticAndDynamic,
            Physics::CollisionGroup::All,
            nullptr
        );

        EXPECT_FALSE(hit);
    }
    
    TEST_F(GenericPhysicsInterfaceTest, ShapeCast_CastAgainstSphere_ReturnsHits)
    {
        auto sphereEntity = AddSphereEntity(AZ::Vector3(0.0f), 10.0f);

        Physics::RayCastHit hit;
        WorldRequestBus::BroadcastResult(hit, &WorldRequests::SphereCast,
            1.0f,
            AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f)),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            20.0f, Physics::QueryType::StaticAndDynamic,
            Physics::CollisionGroup::All,
            nullptr
        );

        EXPECT_TRUE(hit);
        EXPECT_EQ(hit.m_body->GetEntityId(), sphereEntity->GetId());

        // clear up scene
        delete sphereEntity;
    }

    TEST_F(GenericPhysicsInterfaceTest, ShapeCast_SphereCastAgainstStaticObject_ReturnsHits)
    {
        auto boxEntity = AZStd::shared_ptr<AZ::Entity>(AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 1.0f, 1.0f)));

        Physics::RayCastHit hit;
        WorldRequestBus::BroadcastResult(hit, &WorldRequests::SphereCast,
            1.5f,
            AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f)),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            20.0f, Physics::QueryType::StaticAndDynamic,
            Physics::CollisionGroup::All,
            nullptr
        );

        EXPECT_TRUE(hit);
        EXPECT_EQ(hit.m_body->GetEntityId(), boxEntity->GetId());
    }

    TEST_F(GenericPhysicsInterfaceTest, ShapeCast_SphereCastAgainstFilteredObjects_ReturnsHits)
    {
        auto entity1 = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f, CollisionLayer(0));
        auto entity2 = AddCapsuleEntity(AZ::Vector3(0.0f, 0.0f, 20.0f), 10.0f, 2.0f, CollisionLayer(1));
        auto entity3 = AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 30.0f), AZ::Vector3(20.0f, 20.0f, 20.0f), CollisionLayer(2));

        CollisionGroup group = CollisionGroup::All;
        group.SetLayer(CollisionLayer(0), true);
        group.SetLayer(CollisionLayer(1), false);
        group.SetLayer(CollisionLayer(2), true);

        AZStd::vector<Physics::RayCastHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::SphereCastMultiple,
            1.5f,
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            AZ::Vector3(0.0f, 0.0f, 1.0f),
            200.0f, Physics::QueryType::StaticAndDynamic,
            group,
            nullptr
        );

        ASSERT_TRUE(hits.size() == 2);
        EXPECT_TRUE(hits[1].m_body->GetEntityId() == entity1->GetId());
        EXPECT_TRUE(hits[0].m_body->GetEntityId() == entity3->GetId());

        delete entity1;
        delete entity2;
        delete entity3;
    }

    TEST_F(GenericPhysicsInterfaceTest, ShapeCast_AgainstMultipleTouchAndBlockHits_ReturnsClosestBlockAndTouches)
    {
        auto dynamicSphere = AddSphereEntity(AZ::Vector3(20.0f, 0.0f, 0.0f), 10.0f);
        auto staticBox = AddStaticBoxEntity(AZ::Vector3(40.0f, 0.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 5.0f));
        auto blockingSphere = AddSphereEntity(AZ::Vector3(60.0f, 0.0f, 0.0f), 5.0f);
        auto blockingBox = AddStaticBoxEntity(AZ::Vector3(80.0f, 0.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 5.0f));
        auto farSphere = AddSphereEntity(AZ::Vector3(120.0f, 0.0f, 0.0f), 10.0f);

        AZStd::vector<Physics::RayCastHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::SphereCastMultiple,
            1.5f,
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            200.0f, Physics::QueryType::StaticAndDynamic,
            Physics::CollisionGroup::All,
            [&](const Physics::WorldBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                if (body->GetEntityId() == blockingBox->GetId() || body->GetEntityId() == blockingSphere->GetId())
                {
                    return QueryHitType::Block;
                }
                return QueryHitType::Touch;
            }
        );

        ASSERT_EQ(hits.size(), 3);

        ASSERT_EQ(1, AZStd::count_if(hits.begin(), hits.end(), GetEntityInRayCastHitCallBack(dynamicSphere->GetId())));
        ASSERT_EQ(1, AZStd::count_if(hits.begin(), hits.end(), GetEntityInRayCastHitCallBack(staticBox->GetId())));
        ASSERT_EQ(1, AZStd::count_if(hits.begin(), hits.end(), GetEntityInRayCastHitCallBack(blockingSphere->GetId())));

        delete dynamicSphere;
        delete staticBox;
        delete blockingSphere;
        delete blockingBox;
        delete farSphere;
    }

    TEST_F(GenericPhysicsInterfaceTest, Overlap_OverlapMultipleObjects_ReturnsHits)
    {
        AZStd::shared_ptr<AZ::Entity> sphereEntity(AddSphereEntity(AZ::Vector3(10.0f, 0.0f, 0.0f), 3.0f));
        AZStd::shared_ptr<AZ::Entity> boxEntity(AddBoxEntity(AZ::Vector3(7.0f, 4.0f, 0.0f), AZ::Vector3(1.0f)));
        AZStd::shared_ptr<AZ::Entity> capsuleEntity(AddCapsuleEntity(AZ::Vector3(15.0f, 0.0f, 0.0f), 3.0f, 1.0f));

        BoxShapeConfiguration overlapShape;
        overlapShape.m_dimensions = AZ::Vector3(3.0f);

        OverlapRequest request;
        request.m_pose = AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f));
        request.m_shapeConfiguration = &overlapShape;

        AZStd::vector<OverlapHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::Overlap, request);

        EXPECT_EQ(hits.size(), 2);

        // boxEntity shouldn't be included in the result
        EXPECT_FALSE(AZStd::any_of(hits.begin(), hits.end(),
            [idToFind = boxEntity->GetId()](const OverlapHit& hit) { return hit.m_body->GetEntityId() == idToFind; }));

    }

    TEST_F(GenericPhysicsInterfaceTest, Overlap_OverlapMultipleObjectsUseFriendlyFunctions_ReturnsHits)
    {
        AZStd::shared_ptr<AZ::Entity> sphereEntity(AddSphereEntity(AZ::Vector3(10.0f, 0.0f, 0.0f), 3.0f));
        AZStd::shared_ptr<AZ::Entity> boxEntity(AddBoxEntity(AZ::Vector3(7.0f, 4.0f, 0.0f), AZ::Vector3(1.0f)));
        AZStd::shared_ptr<AZ::Entity> capsuleEntity(AddCapsuleEntity(AZ::Vector3(15.0f, 0.0f, 0.0f), 3.0f, 1.0f));

        AZStd::shared_ptr<World> defaultWorld;
        DefaultWorldBus::BroadcastResult(defaultWorld, &DefaultWorldRequests::GetDefaultWorld);
        
        {
            AZStd::vector<OverlapHit> hits = defaultWorld->OverlapBox(AZ::Vector3(3.0f), AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f)));

            EXPECT_EQ(hits.size(), 2);

            // boxEntity shouldn't be included in the result
            EXPECT_FALSE(AZStd::any_of(hits.begin(), hits.end(),
                [idToFind = boxEntity->GetId()](const OverlapHit& hit) { return hit.m_body->GetEntityId() == idToFind; }));
        }

        {
            AZStd::vector<OverlapHit> hits = defaultWorld->OverlapSphere(3.0f, AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f)));

            EXPECT_EQ(hits.size(), 2);

            // boxEntity shouldn't be included in the result
            EXPECT_FALSE(AZStd::any_of(hits.begin(), hits.end(),
                [idToFind = boxEntity->GetId()](const OverlapHit& hit) { return hit.m_body->GetEntityId() == idToFind; }));

        }
    }

    TEST_F(GenericPhysicsInterfaceTest, Overlap_OverlapMultipleObjectsUseFriendlyFunctionsCustomFiltering_ReturnsHits)
    {
        AZStd::shared_ptr<AZ::Entity> sphereEntity(AddSphereEntity(AZ::Vector3(10.0f, 0.0f, 0.0f), 3.0f));
        AZStd::shared_ptr<AZ::Entity> boxEntity(AddBoxEntity(AZ::Vector3(7.0f, 4.0f, 0.0f), AZ::Vector3(1.0f)));
        AZStd::shared_ptr<AZ::Entity> capsuleEntity(AddCapsuleEntity(AZ::Vector3(15.0f, 0.0f, 0.0f), 3.0f, 1.0f));

        AZStd::shared_ptr<World> defaultWorld;
        DefaultWorldBus::BroadcastResult(defaultWorld, &DefaultWorldRequests::GetDefaultWorld);

        // Here we do an overlap test that covers all objects in the scene
        // However we provide a custom filtering function that filters out a specific entity
        {
            AZ::EntityId entityIdToFilterOut = capsuleEntity->GetId();

            AZStd::vector<Physics::OverlapHit> hits = defaultWorld->OverlapCapsule(100.0f, 30.0f, AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f)),
                [entityIdToFilterOut](const Physics::WorldBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->GetEntityId() != entityIdToFilterOut;
            });

            EXPECT_EQ(hits.size(), 2);

            EXPECT_FALSE(AZStd::any_of(hits.begin(), hits.end(),
                [entityIdToFilterOut](const OverlapHit& hit) { return hit.m_body->GetEntityId() == entityIdToFilterOut; }));
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, Overlap_OverlapMultipleObjects_ReturnsFilteredHits)
    {
        AZStd::shared_ptr<AZ::Entity> sphereEntity(AddSphereEntity(AZ::Vector3(10.0f, 0.0f, 0.0f), 3.0f, CollisionLayer(0)));
        AZStd::shared_ptr<AZ::Entity> boxEntity(AddStaticBoxEntity(AZ::Vector3(12.0f, 0.0f, 0.0f), AZ::Vector3(1.0f), CollisionLayer(1)));
        AZStd::shared_ptr<AZ::Entity> capsuleEntity(AddCapsuleEntity(AZ::Vector3(14.0f, 0.0f, 0.0f), 3.0f, 1.0f, CollisionLayer(2)));

        BoxShapeConfiguration overlapShape;
        overlapShape.m_dimensions = AZ::Vector3(1.0f);

        OverlapRequest request;
        request.m_pose = AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f));
        request.m_shapeConfiguration = &overlapShape;
        request.m_collisionGroup = CollisionGroup::All;
        request.m_collisionGroup.SetLayer(CollisionLayer(0), false); // Filter out the sphere
        request.m_collisionGroup.SetLayer(CollisionLayer(1), true);
        request.m_collisionGroup.SetLayer(CollisionLayer(2), true);

        AZStd::vector<OverlapHit> hits;
        WorldRequestBus::BroadcastResult(hits, &WorldRequests::Overlap, request);

        EXPECT_EQ(hits.size(), 2);
        EXPECT_FALSE(AZStd::any_of(hits.begin(), hits.end(),
            [sphereEntity](const OverlapHit& hit)
        { 
            // Make sure the sphere was not included
            return hit.m_body->GetEntityId() == sphereEntity->GetId();
        }));

    }

    TEST_F(GenericPhysicsInterfaceTest, Gravity_DynamicBody_BodyFalls)
    {
        auto world = CreateTestWorld();
        auto rigidBody = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 0.0f, 100.0f));
        UpdateWorld(world.get(), 1.0f / 60.f, 60);

        // expect velocity to be -gt and distance fallen to be 1/2gt^2, but allow quite a lot of tolerance
        // due to potential differences in back end integration schemes etc.
        EXPECT_NEAR(rigidBody->GetLinearVelocity().GetZ(), -10.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetTransform().GetTranslation().GetZ(), 95.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetCenterOfMassWorld().GetZ(), 95.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetPosition().GetZ(), 95.0f, 0.5f);
    }

    TEST_F(GenericPhysicsInterfaceTest, World_SplitSimulation_BodyFallsTheSameInBothWorlds)
    {
        auto worldA = CreateTestWorld();
        auto worldB = CreateTestWorld();

        AZ::Vector3 initialPosition(0.0f, 0.0f, 100.0f);

        auto rigidBodyA = AddUnitBoxToWorld(worldA.get(), initialPosition);
        auto rigidBodyB = AddUnitBoxToWorld(worldB.get(), initialPosition);

        Physics::WorldConfiguration worldConfiguration;
        float deltaTime = worldConfiguration.m_fixedTimeStep;

        AZ::u32 numSteps = 60;

        UpdateWorld(worldA.get(), deltaTime, numSteps);
        UpdateWorldSplitSim(worldB.get(), deltaTime, numSteps);

        // expect velocity to be -gt and distance fallen to be 1/2gt^2, but allow quite a lot of tolerance
        // due to potential differences in back end integration schemes etc.
        EXPECT_NEAR(rigidBodyA->GetLinearVelocity().GetZ(), -10.0f, 0.5f);
        EXPECT_NEAR(rigidBodyA->GetTransform().GetTranslation().GetZ(), 95.0f, 0.5f);
        EXPECT_NEAR(rigidBodyA->GetCenterOfMassWorld().GetZ(), 95.0f, 0.5f);
        EXPECT_NEAR(rigidBodyA->GetPosition().GetZ(), 95.0f, 0.5f);

        // Verify simulation results are the same
        EXPECT_TRUE(rigidBodyA->GetLinearVelocity().IsClose(rigidBodyB->GetLinearVelocity()));
        EXPECT_TRUE(rigidBodyA->GetTransform().GetTranslation().IsClose(rigidBodyB->GetTransform().GetTranslation()));
        EXPECT_TRUE(rigidBodyA->GetCenterOfMassWorld().IsClose(rigidBodyB->GetCenterOfMassWorld()));
        EXPECT_TRUE(rigidBodyA->GetPosition().IsClose(rigidBodyB->GetPosition()));
    }

    TEST_F(GenericPhysicsInterfaceTest, IncreaseMass_StaggeredTowerOfBoxes_TowerOverbalances)
    {
        auto world = CreateTestWorld();

        // make a tower of boxes which is staggered but should still balance if all the blocks are the same mass
        auto boxA = AddStaticUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 0.0f, 0.5f));
        auto boxB = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.3f, 0.0f, 1.5f));
        auto boxC = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.6f, 0.0f, 2.5f));

        // check that the tower balances
        UpdateWorld(world.get(), 1.0f / 60.0f, 60);
        EXPECT_NEAR(2.5f, boxC->GetPosition().GetZ(), 0.01f);

        // increasing the mass of the top block in the tower should overbalance it
        boxC->SetMass(5.0f);
        EXPECT_NEAR(1.0f, boxB->GetMass(), 0.01f);
        EXPECT_NEAR(1.0f, boxB->GetInverseMass(), 0.01f);
        EXPECT_NEAR(5.0f, boxC->GetMass(), 0.01f);
        EXPECT_NEAR(0.2f, boxC->GetInverseMass(), 0.01f);
        boxB->ForceAwake();
        boxC->ForceAwake();
        UpdateWorld(world.get(), 1.0f / 60.0f, 300);
        EXPECT_GT(0.0f, static_cast<float>(boxC->GetPosition().GetZ()));
    }

    TEST_F(GenericPhysicsInterfaceTest, GetCenterOfMass_FallingBody_CenterOfMassCorrectDuringFall)
    {
        auto world = CreateTestWorld();
        auto boxStatic = AddStaticUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto boxDynamic = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 0.0f, 2.0f));
        auto tolerance = 1e-3f;

        EXPECT_TRUE(boxDynamic->GetCenterOfMassWorld().IsClose(AZ::Vector3(0.0f, 0.0f, 2.0f), tolerance));
        EXPECT_TRUE(boxDynamic->GetCenterOfMassLocal().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), tolerance));
        UpdateWorld(world.get(), 1.0f / 60.0f, 300);
        EXPECT_NEAR(static_cast<float>(boxDynamic->GetCenterOfMassWorld().GetZ()), 1.0f, 1e-3f);
        EXPECT_TRUE(boxDynamic->GetCenterOfMassLocal().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), tolerance));
    }

    TEST_F(GenericPhysicsInterfaceTest, SetLinearVelocity_DynamicBox_AffectsTrajectory)
    {
        auto world = CreateTestWorld();
        auto boxA = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, -5.0f, 10.0f));
        auto boxB = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 5.0f, 10.0f));

        boxA->SetLinearVelocity(AZ::Vector3(10.0f, 0.0f, 0.0f));
        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = boxA->GetPosition().GetX();
            float xPreviousB = boxB->GetPosition().GetX();
            UpdateWorld(world.get(), 1.0f / 60.0f, 10);
            EXPECT_GT(static_cast<float>(boxA->GetPosition().GetX()), xPreviousA);
            EXPECT_NEAR(boxB->GetPosition().GetX(), xPreviousB, 1e-3f);
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, ApplyLinearImpulse_DynamicBox_AffectsTrajectory)
    {
        auto world = CreateTestWorld();
        auto boxA = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 0.0f, 100.0f));
        auto boxB = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 10.0f, 100.0f));

        boxA->ApplyLinearImpulse(AZ::Vector3(10.0f, 0.0f, 0.0f));
        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = boxA->GetPosition().GetX();
            float xPreviousB = boxB->GetPosition().GetX();
            UpdateWorld(world.get(), 1.0f / 60.0f, 10);
            EXPECT_GT(static_cast<float>(boxA->GetPosition().GetX()), xPreviousA);
            EXPECT_NEAR(boxB->GetPosition().GetX(), xPreviousB, 1e-3f);
        }
    }

    // allow a more generous tolerance on tests involving objects in contact, since the way physics engines normally
    // handle multiple contacts between objects can lead to slight imbalances in contact forces
    static constexpr float ContactTestTolerance = 0.01f;

    TEST_F(GenericPhysicsInterfaceTest, GetAngularVelocity_DynamicCapsuleOnSlope_GainsAngularVelocity)
    {
        auto world = CreateTestWorld();
        AZ::Transform slopeTransform = AZ::Transform::CreateRotationY(0.1f);
        auto slope = AddStaticFloorToWorld(world.get(), slopeTransform);
        auto capsule = AddCapsuleToWorld(world.get(), slopeTransform.TransformPoint(AZ::Vector3::CreateAxisZ()));

        // the capsule should roll down the slope, picking up angular velocity parallel to the Y axis
        float angularVelocityMagnitude = capsule->GetAngularVelocity().GetLength();
        UpdateWorld(world.get(), 1.0f / 60.0f, 60);
        angularVelocityMagnitude = capsule->GetAngularVelocity().GetLength();
        for (int i = 0; i < 60; i++)
        {
            world->Update(1.0f / 60.0f);
            auto angularVelocity = capsule->GetAngularVelocity();
            EXPECT_TRUE(angularVelocity.IsPerpendicular(AZ::Vector3::CreateAxisX(), ContactTestTolerance));
            EXPECT_TRUE(angularVelocity.IsPerpendicular(AZ::Vector3::CreateAxisZ(), ContactTestTolerance));
            EXPECT_TRUE(capsule->GetAngularVelocity().GetLength() > angularVelocityMagnitude);
            angularVelocityMagnitude = angularVelocity.GetLength();
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, SetAngularVelocity_DynamicCapsule_StartsRolling)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world.get());
        auto capsule = AddCapsuleToWorld(world.get(), AZ::Vector3::CreateAxisZ());

        // capsule should remain stationary
        for (int i = 0; i < 60; i++)
        {
            world->Update(1.0f / 60.0f);
            EXPECT_TRUE(capsule->GetPosition().IsClose(AZ::Vector3::CreateAxisZ(), ContactTestTolerance));
            EXPECT_TRUE(capsule->GetLinearVelocity().IsClose(AZ::Vector3::CreateZero(), ContactTestTolerance));
            EXPECT_TRUE(capsule->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero(), ContactTestTolerance));
        }

        // apply an angular velocity and it should start rolling
        auto angularVelocity = AZ::Vector3::CreateAxisY(10.0f);
        capsule->SetAngularVelocity(angularVelocity);
        EXPECT_TRUE(capsule->GetAngularVelocity().IsClose(angularVelocity));

        for (int i = 0; i < 60; i++)
        {
            float xPrevious = capsule->GetPosition().GetX();
            world->Update(1.0f / 60.0f);
            EXPECT_TRUE(capsule->GetPosition().GetX() > xPrevious);
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, GetLinearVelocityAtWorldPoint_FallingRotatingCapsule_EdgeVelocitiesCorrect)
    {
        auto world = CreateTestWorld();

        // create dynamic capsule and start it falling and rotating
        auto capsule = AddCapsuleToWorld(world.get(), AZ::Vector3::CreateAxisZ());
        float angularVelocityMagnitude = 1.0f;
        capsule->SetAngularVelocity(AZ::Vector3::CreateAxisY(angularVelocityMagnitude));
        capsule->SetAngularDamping(0.0f);
        UpdateWorld(world.get(), 1.0f / 60.0f, 60);

        // check the velocities at some points on the rim of the capsule are as expected
        for (int i = 0; i < 60; i++)
        {
            world->Update(1.0f / 60.0f);
            auto position = capsule->GetPosition();
            float fallingSpeed = capsule->GetLinearVelocity().GetZ();
            float radius = 0.5f;
            AZ::Vector3 z = AZ::Vector3::CreateAxisZ(radius);
            AZ::Vector3 x = AZ::Vector3::CreateAxisX(radius);

            auto v1 = capsule->GetLinearVelocityAtWorldPoint(position - z);
            auto v2 = capsule->GetLinearVelocityAtWorldPoint(position - x);
            auto v3 = capsule->GetLinearVelocityAtWorldPoint(position + x);

            EXPECT_TRUE(v1.IsClose(AZ::Vector3(-radius * angularVelocityMagnitude, 0.0f, fallingSpeed)));
            EXPECT_TRUE(v2.IsClose(AZ::Vector3(0.0f, 0.0f, fallingSpeed + radius * angularVelocityMagnitude)));
            EXPECT_TRUE(v3.IsClose(AZ::Vector3(0.0f, 0.0f, fallingSpeed - radius * angularVelocityMagnitude)));
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, GetPosition_RollingCapsule_OrientationCorrect)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world.get());

        // create dynamic capsule and start it rolling
        auto capsule = AddCapsuleToWorld(world.get(), AZ::Vector3::CreateAxisZ());
        capsule->SetLinearVelocity(AZ::Vector3::CreateAxisX(5.0f));
        capsule->SetAngularVelocity(AZ::Vector3::CreateAxisY(10.0f));
        UpdateWorld(world.get(), 1.0f / 60.0f, 60);

        // check the capsule orientation evolves as expected
        for (int i = 0; i < 60; i++)
        {
            auto orientationPrevious = capsule->GetOrientation();
            float xPrevious = capsule->GetPosition().GetX();
            world->Update(1.0f / 60.0f);
            float angle = 2.0f * (capsule->GetPosition().GetX() - xPrevious);
            EXPECT_TRUE(capsule->GetOrientation().IsClose(orientationPrevious * AZ::Quaternion::CreateRotationY(angle)));
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, OffCenterImpulse_DynamicCapsule_StartsRotating)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world.get());
        AZ::Vector3 posA(0.0f, -5.0f, 1.0f);
        AZ::Vector3 posB(0.0f, 0.0f, 1.0f);
        AZ::Vector3 posC(0.0f, 5.0f, 1.0f);
        auto capsuleA = AddCapsuleToWorld(world.get(), posA);
        auto capsuleB = AddCapsuleToWorld(world.get(), posB);
        auto capsuleC = AddCapsuleToWorld(world.get(), posC);

        // all the capsules should be stationary initially
        for (int i = 0; i < 10; i++)
        {
            world->Update(1.0f / 60.0f);
            EXPECT_TRUE(capsuleA->GetPosition().IsClose(posA));
            EXPECT_TRUE(capsuleA->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero(), ContactTestTolerance));
            EXPECT_TRUE(capsuleB->GetPosition().IsClose(posB));
            EXPECT_TRUE(capsuleB->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero(), ContactTestTolerance));
            EXPECT_TRUE(capsuleC->GetPosition().IsClose(posC));
            EXPECT_TRUE(capsuleC->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero(), ContactTestTolerance));
        }

        // apply off-center impulses to capsule A and C, and an impulse through the center of B
        AZ::Vector3 impulse(0.0f, 0.0f, 10.0f);
        capsuleA->ApplyLinearImpulseAtWorldPoint(impulse, posA + AZ::Vector3::CreateAxisX(0.5f));
        capsuleB->ApplyLinearImpulseAtWorldPoint(impulse, posB);
        capsuleC->ApplyLinearImpulseAtWorldPoint(impulse, posC + AZ::Vector3::CreateAxisX(-0.5f));

        // A and C should be rotating in opposite directions, B should still have 0 angular velocity
        for (int i = 0; i < 30; i++)
        {
            world->Update(1.0f / 60.0f);
            EXPECT_TRUE(capsuleA->GetAngularVelocity().GetY() < 0.0f);
            EXPECT_TRUE(capsuleB->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero(), ContactTestTolerance));
            EXPECT_TRUE(capsuleC->GetAngularVelocity().GetY() > 0.0f);
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, ApplyAngularImpulse_DynamicSphere_StartsRotating)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world.get());

        AZStd::shared_ptr<RigidBody> spheres[3];
        for (int i = 0; i < 3; i++)
        {
            spheres[i] = AddSphereToWorld(world.get(), AZ::Vector3(0.0f, -5.0f + 5.0f * i, 1.0f));
        }

        // all the spheres should start stationary
        UpdateWorld(world.get(), 1.0f / 60.0f, 10);
        for (int i = 0; i < 3; i++)
        {
            EXPECT_TRUE(spheres[i]->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero()));
        }

        // apply angular impulses and they should gain angular velocity parallel to the impulse direction
        AZ::Vector3 impulses[3] = { AZ::Vector3(2.0f, 4.0f, 0.0f), AZ::Vector3(-3.0f, 1.0f, 0.0f),
            AZ::Vector3(-2.0f, 3.0f, 0.0f) };
        for (int i = 0; i < 3; i++)
        {
            spheres[i]->ApplyAngularImpulse(impulses[i]);
        }

        UpdateWorld(world.get(), 1.0f / 60.0f, 10);

        for (int i = 0; i < 3; i++)
        {
            auto angVel = spheres[i]->GetAngularVelocity();
            EXPECT_TRUE(angVel.GetProjected(impulses[i]).IsClose(angVel, 0.1f));
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, StartAsleep_FallingBox_DoesNotFall)
    {
        auto world = CreateTestWorld();

        // Box should start asleep
        RigidBodyConfiguration config;
        config.m_startAsleep = true;

        // Create rigid body
        AZStd::shared_ptr<RigidBody> box;
        SystemRequestBus::BroadcastResult(box, &SystemRequests::CreateRigidBody, config);
        world->AddBody(*box);

        UpdateWorld(world.get(), 1.0f / 60.0f, 100);

        // Check the box is still at 0 and hasn't dropped
        EXPECT_NEAR(0.0f, box->GetPosition().GetZ(), 0.01f);
    }

    TEST_F(GenericPhysicsInterfaceTest, ForceAsleep_FallingBox_BecomesStationary)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world.get());
        auto box = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 0.0f, 10.0f));
        UpdateWorld(world.get(), 1.0f / 60.0f, 60);

        EXPECT_TRUE(box->IsAwake());

        auto pos = box->GetPosition();
        box->ForceAsleep();
        EXPECT_FALSE(box->IsAwake());
        UpdateWorld(world.get(), 1.0f / 60.0f, 30);
        EXPECT_FALSE(box->IsAwake());
        // the box should be asleep so it shouldn't have moved
        EXPECT_TRUE(box->GetPosition().IsClose(pos));
    }

    TEST_F(GenericPhysicsInterfaceTest, ForceAwake_SleepingBox_SleepStateCorrect)
    {
        auto world = CreateTestWorld();
        auto floor = AddStaticFloorToWorld(world.get());
        auto box = AddUnitBoxToWorld(world.get(), AZ::Vector3(0.0f, 0.0f, 1.0f));

        UpdateWorld(world.get(), 1.0f / 60.0f, 60);
        EXPECT_FALSE(box->IsAwake());

        box->ForceAwake();
        EXPECT_TRUE(box->IsAwake());

        UpdateWorld(world.get(), 1.0f / 60.0f, 60);
        // the box should have gone back to sleep
        EXPECT_FALSE(box->IsAwake());
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAabb_Box_ValidExtents)
    {
        auto world = CreateTestWorld();
        AZ::Vector3 posBox(0.0f, 0.0f, 0.0f);
        auto box = AddUnitBoxToWorld(world.get(), posBox);

        EXPECT_TRUE(box->GetAabb().GetMin().IsClose(posBox - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(box->GetAabb().GetMax().IsClose(posBox + 0.5f * AZ::Vector3::CreateOne()));

        // rotate the box and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        box->SetTransform(AZ::Transform::CreateFromQuaternionAndTranslation(quat, posBox));

        AZ::Vector3 boxExtent(sqrtf(0.5f), sqrtf(0.5f), 0.5f);
        EXPECT_TRUE(box->GetAabb().GetMin().IsClose(posBox - boxExtent));
        EXPECT_TRUE(box->GetAabb().GetMax().IsClose(posBox + boxExtent));
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAabb_Sphere_ValidExtents)
    {
        auto world = CreateTestWorld();
        AZ::Vector3 posSphere(0.0f, 0.0f, 0.0f);
        auto sphere = AddSphereToWorld(world.get(), posSphere);

        EXPECT_TRUE(sphere->GetAabb().GetMin().IsClose(posSphere - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(sphere->GetAabb().GetMax().IsClose(posSphere + 0.5f * AZ::Vector3::CreateOne()));

        // rotate the sphere and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        sphere->SetTransform(AZ::Transform::CreateFromQuaternionAndTranslation(quat, posSphere));

        EXPECT_TRUE(sphere->GetAabb().GetMin().IsClose(posSphere - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(sphere->GetAabb().GetMax().IsClose(posSphere + 0.5f * AZ::Vector3::CreateOne()));
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAabb_Capsule_ValidExtents)
    {
        auto world = CreateTestWorld();
        AZ::Vector3 posCapsule(0.0f, 0.0f, 0.0f);
        auto capsule = AddCapsuleToWorld(world.get(), posCapsule);

        EXPECT_TRUE(capsule->GetAabb().GetMin().IsClose(posCapsule - AZ::Vector3(0.5f, 1.0f, 0.5f)));
        EXPECT_TRUE(capsule->GetAabb().GetMax().IsClose(posCapsule + AZ::Vector3(0.5f, 1.0f, 0.5f)));

        // rotate the bodies and check the bounding boxes are still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        capsule->SetTransform(AZ::Transform::CreateFromQuaternionAndTranslation(quat, posCapsule));

        AZ::Vector3 capsuleExtent(0.5f + sqrt(0.125f), 0.5f + sqrt(0.125f), 0.5f);
        EXPECT_TRUE(capsule->GetAabb().GetMin().IsClose(posCapsule - capsuleExtent));
        EXPECT_TRUE(capsule->GetAabb().GetMax().IsClose(posCapsule + capsuleExtent));
    }

    TEST_F(GenericPhysicsInterfaceTest, Materials_BoxesSharingDefaultMaterial_JumpingSameHeight)
    {
        auto world = CreateTestWorld();

        auto boxA = AddStaticFloorToWorld(world.get());
        auto boxB = AddUnitBoxToWorld(world.get(), AZ::Vector3(1.0f, 0.0f, 10.0f));
        auto boxC = AddUnitBoxToWorld(world.get(), AZ::Vector3(-1.0f, 0.0f, 10.0f));

        auto material = boxC->GetShape(0)->GetMaterial();
        material->SetRestitution(1.0f);

        UpdateWorld(world.get(), 1.0f / 60.0f, 150);
        
        // boxB and boxC should have the same material (default)
        // so they should both bounce high
        EXPECT_NEAR(boxB->GetPosition().GetZ(), boxC->GetPosition().GetZ(), 0.5f);
    }

    TEST_F(GenericPhysicsInterfaceTest, World_GetNativePtrByWorldName_ReturnsNativePtr)
    {
        void* validNativePtr = nullptr;
        WorldRequestBus::EventResult(validNativePtr, Physics::DefaultPhysicsWorldId, &WorldRequests::GetNativePointer);
        EXPECT_TRUE(validNativePtr != nullptr);

        void* invalidNativePtr = nullptr;
        WorldRequestBus::EventResult(invalidNativePtr, AZ_CRC("Bad World Name"), &WorldRequests::GetNativePointer);
        EXPECT_TRUE(invalidNativePtr == nullptr);
    }

    TEST_F(GenericPhysicsInterfaceTest, Collider_ColliderTag_IsSetFromConfiguration)
    {
        const AZStd::string colliderTagName = "ColliderTestTag";
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_tag = colliderTagName;
        Physics::SphereShapeConfiguration shapeConfig;

        AZStd::shared_ptr<Physics::Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfig);

        EXPECT_EQ(shape->GetTag(), AZ::Crc32(colliderTagName));
    }

} // namespace Physics
