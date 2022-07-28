/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>

#include <AzTest/AzTest.h>
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

#include <RigidBodyComponent.h>
#include <SphereColliderComponent.h>

namespace PhysX
{
    class PhysXSceneQueryBase
    {
    public:
        void SetUpFixture()
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                AzPhysics::SceneConfiguration newSceneConfig;
                newSceneConfig.m_sceneName = "TestScene";
                m_testSceneHandle = physicsSystem->AddScene(newSceneConfig);
            }
        }
        void TearDownFixture()
        {
            //Cleanup any created scenes
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveScene(m_testSceneHandle);
            }
            m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        }

        AzPhysics::SceneHandle m_testSceneHandle;
    };
    //setup a test fixture with a scene named 'TestScene'
    class PhysXSceneQueryFixture
        : public testing::Test
        , public PhysXSceneQueryBase
    {
    public:
        void SetUp() override
        {
            PhysXSceneQueryBase::SetUpFixture();
        }
        void TearDown() override
        {
            PhysXSceneQueryBase::TearDownFixture();
        }
    };

    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstNothing_ReturnsNoHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        EXPECT_FALSE(result);
    }

    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstRigidBody_ReturnsHit)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //create the Simulated body
        AzPhysics::SimulatedBodyHandle sphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle, AZ::Vector3(0.0f), 10.0f);
        EXPECT_FALSE(sphereHandle == AzPhysics::InvalidSimulatedBodyHandle); //ensure the sphere was created

        //create the raycast request
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        //query the scene
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //result should have some hits
        EXPECT_TRUE(result);
        //should only have 1 hit
        ASSERT_EQ(result.m_hits.size(), 1);
        //hit should be valid
        const AzPhysics::SceneQueryHit& hit = result.m_hits[0];
        EXPECT_TRUE(hit);

        //Distance result flag should be set and have a non zero value
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::Distance);
        EXPECT_FALSE(AZ::IsClose(hit.m_distance, 0.0f));

        //Position result flag should be set and have a non zero value
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::Position);
        EXPECT_FALSE(hit.m_position.IsZero());

        //Normal result flag should be set and have a non zero value
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::Normal);
        EXPECT_FALSE(hit.m_normal.IsZero());

        //BodyHandle result flag should be set and should match the sphereHandle
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::BodyHandle);
        EXPECT_TRUE(hit.m_bodyHandle == sphereHandle);

        //Shape result flag should be set and should not be null
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::Shape);
        EXPECT_FALSE(hit.m_shape == nullptr);

        //EntityId result flag should NOT be set
        EXPECT_FALSE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::EntityId);

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, sphereHandle);
    }

    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstSphereEntity_ReturnsCorrectShapeAndMaterial)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //create entity
        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = AzPhysics::CollisionLayer::Default;
        EntityPtr sphereEntity = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f), 10.0f, colliderConfig);

        //create raycast request
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        //query the scene
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //result should have some hits
        EXPECT_TRUE(result);
        //should only have 1 hit
        ASSERT_EQ(result.m_hits.size(), 1);
        //hit should be valid
        const AzPhysics::SceneQueryHit& hit = result.m_hits[0];
        EXPECT_TRUE(hit);

        //get the Rigid body from the entity
        AzPhysics::RigidBody* rigidBody;
        Physics::RigidBodyRequestBus::EventResult(rigidBody, sphereEntity->GetId(), &Physics::RigidBodyRequestBus::Events::GetRigidBody);

        //Verify shape and material
        ASSERT_NE(rigidBody->GetShape(0), nullptr);
        EXPECT_TRUE(hit.m_physicsMaterialId.IsValid());
        ASSERT_EQ(hit.m_shape, rigidBody->GetShape(0).get());
        ASSERT_EQ(hit.m_physicsMaterialId, rigidBody->GetShape(0).get()->GetMaterial()->GetId());
    }

    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstStaticObject_ReturnsHit)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //Create the static body
        AzPhysics::SimulatedBodyHandle boxHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(10.0f));

        //create the raycast request
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        //query the scene
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //result should have some hits
        EXPECT_TRUE(result);
        //should only have 1 hit
        ASSERT_EQ(result.m_hits.size(), 1);
        //hit should be valid
        const AzPhysics::SceneQueryHit& hit = result.m_hits[0];
        EXPECT_TRUE(hit);

        //Distance result flag should be set and have a non zero value
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::Distance);
        EXPECT_FALSE(AZ::IsClose(hit.m_distance, 0.0f));

        //Position result flag should be set and have a non zero value
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::Position);
        EXPECT_FALSE(hit.m_position.IsZero());

        //Normal result flag should be set and have a non zero value
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::Normal);
        EXPECT_FALSE(hit.m_normal.IsZero());

        //BodyHandle result flag should be set and should match the sphereHandle
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::BodyHandle);
        EXPECT_TRUE(hit.m_bodyHandle == boxHandle);

        //Shape result flag should be set and should not be null
        EXPECT_TRUE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::Shape);
        EXPECT_FALSE(hit.m_shape == nullptr);

        //EntityId result flag should NOT be set
        EXPECT_FALSE(hit.m_resultFlags & AzPhysics::SceneQuery::ResultFlags::EntityId);

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, boxHandle);
    }

    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstFilteredSpheres_ReturnsHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //Create the simulated bodies
        AzPhysics::SimulatedBodyHandle sphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 10.0f),
            10.0f,
            AzPhysics::CollisionLayer(0));
        AzPhysics::SimulatedBodyHandle capsuleHandle = TestUtils::AddCapsuleToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 20.0f),
            10.0f,
            2.0f,
            AzPhysics::CollisionLayer(1));
        AzPhysics::SimulatedBodyHandle staticCubeHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 30.0f),
            AZ::Vector3(20.0f, 20.0f, 20.0f),
            AzPhysics::CollisionLayer(2));

        //Setup request
        AzPhysics::CollisionGroup group = AzPhysics::CollisionGroup::All;
        group.SetLayer(AzPhysics::CollisionLayer(0), true);
        group.SetLayer(AzPhysics::CollisionLayer(1), false);
        group.SetLayer(AzPhysics::CollisionLayer(2), true);

        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
        request.m_distance = 200.0f;
        request.m_collisionGroup = group;
        request.m_reportMultipleHits = true;

        //query the scene
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //result should have some hits
        EXPECT_TRUE(result);
        //should only have 2 hits
        ASSERT_EQ(result.m_hits.size(), 2);
        //hit 0 should be the staticCubeHandle
        const AzPhysics::SceneQueryHit& hit0 = result.m_hits[0];
        EXPECT_TRUE(hit0.m_bodyHandle == staticCubeHandle);
        //hit 0 should be the sphereHandle
        const AzPhysics::SceneQueryHit& hit1 = result.m_hits[1];
        EXPECT_TRUE(hit1.m_bodyHandle == sphereHandle);

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, sphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, capsuleHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, staticCubeHandle);
    }

    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstStaticOnly_ReturnsStaticBody)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //Create the simulated bodies
        AzPhysics::SimulatedBodyHandle sphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f);
        AzPhysics::SimulatedBodyHandle staticCubeHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 30.0f),
            AZ::Vector3(20.0f, 20.0f, 20.0f));

        //Setup request
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Static;
        request.m_reportMultipleHits = true;

        //query the scene
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //result should have some hits
        EXPECT_TRUE(result);
        //should only have 1 hit
        ASSERT_EQ(result.m_hits.size(), 1);
        //hit 0 should be the staticCubeHandle
        const AzPhysics::SceneQueryHit& hit0 = result.m_hits[0];
        EXPECT_TRUE(hit0.m_bodyHandle == staticCubeHandle);

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, sphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, staticCubeHandle);
    }

    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstDynamicOnly_ReturnsDynamicSphere)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //Create the simulated bodies
        AzPhysics::SimulatedBodyHandle sphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f);
        AzPhysics::SimulatedBodyHandle staticCubeHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 30.0f),
            AZ::Vector3(20.0f, 20.0f, 20.0f));

        //Setup request
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Dynamic;
        request.m_reportMultipleHits = true;

        //query the scene
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //result should have some hits
        EXPECT_TRUE(result);
        //should only have 1 hit
        ASSERT_EQ(result.m_hits.size(), 1);
        //hit 0 should be the sphereHandle
        const AzPhysics::SceneQueryHit& hit0 = result.m_hits[0];
        EXPECT_TRUE(hit0.m_bodyHandle == sphereHandle);

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, sphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, staticCubeHandle);
    }


    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstStaticAndDynamic_ReturnsBothObjects)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //Create the simulated bodies
        AzPhysics::SimulatedBodyHandle sphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f);
        AzPhysics::SimulatedBodyHandle staticCubeHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 30.0f),
            AZ::Vector3(20.0f, 20.0f, 20.0f));

        //Setup request
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::StaticAndDynamic;
        request.m_reportMultipleHits = true;

        //query the scene
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //result should have some hits
        EXPECT_TRUE(result);
        //should only have 2 hits
        ASSERT_EQ(result.m_hits.size(), 2);
        //hit 0 should be the staticCubeHandle
        const AzPhysics::SceneQueryHit& hit0 = result.m_hits[0];
        EXPECT_TRUE(hit0.m_bodyHandle == staticCubeHandle);
        //hit 1 should be the sphereHandle
        const AzPhysics::SceneQueryHit& hit1 = result.m_hits[1];
        EXPECT_TRUE(hit1.m_bodyHandle == sphereHandle);

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, sphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, staticCubeHandle);
    }

    TEST_F(PhysXSceneQueryFixture, RayCast_AgainstMultipleWithCustomFilter_ReturnsHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //Create the simulated bodies
        AzPhysics::SimulatedBodyHandle dynamicSphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(20.0f, 0.0f, 0.0f), 10.0f);
        AzPhysics::SimulatedBodyHandle staticBoxHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(40.0f, 0.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 5.0f));
        AzPhysics::SimulatedBodyHandle blockingSphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(60.0f, 0.0f, 0.0f), 5.0f);
        AzPhysics::SimulatedBodyHandle blockingStaticBoxHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(80.0f, 0.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 5.0f));
        AzPhysics::SimulatedBodyHandle farSphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(120.0f, 0.0f, 0.0f), 10.0f);

        //Setup the request
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::StaticAndDynamic;
        request.m_reportMultipleHits = true;
        request.m_filterCallback = [&](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
        {
            if (body->m_bodyHandle == blockingStaticBoxHandle || body->m_bodyHandle == blockingSphereHandle)
            {
                return AzPhysics::SceneQuery::QueryHitType::Block;
            }
            return AzPhysics::SceneQuery::QueryHitType::Touch;
        };

        //query the scene
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //result should have some hits
        EXPECT_TRUE(result);
        //should only have 3 hits
        ASSERT_EQ(result.m_hits.size(), 3);
        //verify each entry is expected
        auto GetSimulatedBodyInRayCastHitCallBack = [](AzPhysics::SimulatedBodyHandle bodyHandle)
        {
            return [bodyHandle](const AzPhysics::SceneQueryHit& hit)
            {
                return hit.m_bodyHandle == bodyHandle;
            };
        };
        ASSERT_EQ(1, AZStd::count_if(result.m_hits.begin(), result.m_hits.end(),
            GetSimulatedBodyInRayCastHitCallBack(dynamicSphereHandle)));
        ASSERT_EQ(1, AZStd::count_if(result.m_hits.begin(), result.m_hits.end(),
            GetSimulatedBodyInRayCastHitCallBack(staticBoxHandle)));
        ASSERT_EQ(1, AZStd::count_if(result.m_hits.begin(), result.m_hits.end(),
            GetSimulatedBodyInRayCastHitCallBack(blockingSphereHandle)));

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, dynamicSphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, staticBoxHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, blockingSphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, blockingStaticBoxHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, farSphereHandle);
    }

    TEST_F(PhysXSceneQueryFixture, RayCast_FromInsideTriangleMesh_ReturnsHitsBasedOnHitFlags)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        // Add a cube to the scene
        AzPhysics::SimulatedBodyHandle rigidBodyhandle = TestUtils::AddStaticTriangleMeshCubeToScene(m_testSceneHandle, 3.0f);

        // Do a simple raycast from the inside of the cube
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 20.0f;
        request.m_hitFlags = AzPhysics::SceneQuery::HitFlags::Position;

        // Verify no hit is detected
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);
        EXPECT_FALSE(result);

        // Set the flags to count hits for both sides of the mesh
        request.m_hitFlags = AzPhysics::SceneQuery::HitFlags::Position | AzPhysics::SceneQuery::HitFlags::MeshBothSides;

        // Verify now the hit is detected and it's indeed the cube
        request.m_reportMultipleHits = true;
        result = sceneInterface->QueryScene(m_testSceneHandle, &request);
        EXPECT_TRUE(result);
        EXPECT_EQ(result.m_hits.size(), 1);
        EXPECT_TRUE(result.m_hits[0].m_bodyHandle == rigidBodyhandle);

        // Shift the ray start position outside of the mesh
        request.m_start.SetX(-4.0f);

        // Set the flags to include multiple hits per object
        request.m_hitFlags = AzPhysics::SceneQuery::HitFlags::Position | AzPhysics::SceneQuery::HitFlags::MeshBothSides | AzPhysics::SceneQuery::HitFlags::MeshMultiple;

        // Verify now we have 4 hits: outside + inside of 1st side and inside + outside of the 2nd side
        result = sceneInterface->QueryScene(m_testSceneHandle, &request);
        EXPECT_EQ(result.m_hits.size(), 4);

        // Verify all hits are for the test cube
        bool testBodyInAllHits = AZStd::all_of(result.m_hits.begin(), result.m_hits.end(), [&rigidBodyhandle](const AzPhysics::SceneQueryHit& hit)
            {
                return hit.m_bodyHandle == rigidBodyhandle;
            });
        EXPECT_TRUE(testBodyInAllHits);
    }

    // Fixture for testing combinations of shape flags for scene query
    class SceneQueryFlagsTestFixture
        : public ::testing::TestWithParam<::testing::tuple<bool, bool>>
        , public PhysXSceneQueryBase
    {
    public:
        void SetUp() override
        {
            PhysXSceneQueryBase::SetUpFixture();
        }
        void TearDown() override
        {
            PhysXSceneQueryBase::TearDownFixture();
        }

        bool IsTrigger() const
        {
            return AZStd::get<0>(GetParam());
        }

        bool IsSimulated() const
        {
            return AZStd::get<1>(GetParam());
        }
    };

    TEST_P(SceneQueryFlagsTestFixture, RayCast_ShapesWithMixedFlags_ReturnsHitsForShapes)
    {
        auto* physics = AZ::Interface<Physics::System>::Get();
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        AzPhysics::RigidBodyConfiguration rigidBodyConfig;
        AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfig);
        AzPhysics::RigidBody* rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));

        // Create a box shape with InSceneQueries = false
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_isInSceneQueries = false;
        colliderConfig.m_isTrigger = IsTrigger();
        colliderConfig.m_isSimulated = IsSimulated();
        colliderConfig.m_position = AZ::Vector3(1.0f, 0.0f, 0.0f);
        AZStd::shared_ptr<Physics::Shape> boxShape =
            physics->CreateShape(colliderConfig, Physics::BoxShapeConfiguration());
        rigidBody->AddShape(boxShape);

        // Create a sphere with InSceneQuesries = true
        colliderConfig.m_isInSceneQueries = true;
        colliderConfig.m_isTrigger = IsTrigger();
        colliderConfig.m_isSimulated = IsSimulated();
        colliderConfig.m_position = AZ::Vector3(-1.0f, 0.0f, 0.0f);
        AZStd::shared_ptr<Physics::Shape> sphereShape =
            physics->CreateShape(colliderConfig, Physics::SphereShapeConfiguration());
        rigidBody->AddShape(sphereShape);

        // Do a simple raycast from the box side
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(3.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(-1.0f, 0.0f, 0.0f);
        request.m_distance = 20.0f;
        request.m_hitFlags = AzPhysics::SceneQuery::HitFlags::Position;

        // Verify the sphere was hit
        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);
        EXPECT_TRUE(result);
        EXPECT_EQ(result.m_hits[0].m_shape, sphereShape.get());
    }

    INSTANTIATE_TEST_CASE_P(PhysX, SceneQueryFlagsTestFixture, ::testing::Combine(
        ::testing::Bool(),
        ::testing::Bool()));

    TEST_F(PhysXSceneQueryFixture, ShapeCast_AgainstNothing_ReturnsNoHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //Create request
        AzPhysics::ShapeCastRequest request = AzPhysics::ShapeCastRequestHelpers::CreateSphereCastRequest(1.0f,
            AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f)),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            20.0f,
            AzPhysics::SceneQuery::QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup::All,
            nullptr);

        //Run query
        AzPhysics::SceneQueryHits hit = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //verify results - there should be no hits returned.
        EXPECT_FALSE(hit);
    }

    TEST_F(PhysXSceneQueryFixture, ShapeCast_AgainstSphere_ReturnsHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup body
        AzPhysics::SimulatedBodyHandle sphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle, AZ::Vector3(0.0f), 10.0f);

        //Create request
        AzPhysics::ShapeCastRequest request = AzPhysics::ShapeCastRequestHelpers::CreateSphereCastRequest(1.0f,
            AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f)),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            20.0f,
            AzPhysics::SceneQuery::QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup::All,
            nullptr);

        //Run query
        AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //verify results
        EXPECT_TRUE(results); // there is a hit
        EXPECT_TRUE(results.m_hits.size() == 1); // there is exactly 1 hit
        EXPECT_EQ(results.m_hits[0].m_bodyHandle, sphereHandle); // the hit matches the created sphere which is inside the shape cast request

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, sphereHandle);
    }

    TEST_F(PhysXSceneQueryFixture, ShapeCast_AgainstStaticObject_ReturnsHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup body
        AzPhysics::SimulatedBodyHandle boxHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 1.0f, 1.0f));

        //Create request
        AzPhysics::ShapeCastRequest request = AzPhysics::ShapeCastRequestHelpers::CreateSphereCastRequest(1.5f,
            AZ::Transform::CreateTranslation(AZ::Vector3(-20.0f, 0.0f, 0.0f)),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            20.0f,
            AzPhysics::SceneQuery::QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup::All,
            nullptr);

        //Run query
        AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //verify results
        EXPECT_TRUE(results); // there is a hit
        EXPECT_TRUE(results.m_hits.size() == 1); // there is exactly 1 hit
        EXPECT_EQ(results.m_hits[0].m_bodyHandle, boxHandle); // the hit matches the created box which is inside the shape cast request

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, boxHandle);
    }

    TEST_F(PhysXSceneQueryFixture, ShapeCast_AgainstFilteredObjects_ReturnsHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup bodies
        AzPhysics::SimulatedBodyHandle sphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 10.0f), 10.0f, AzPhysics::CollisionLayer(0));
        AzPhysics::SimulatedBodyHandle capsuleHandle = TestUtils::AddCapsuleToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 20.0f), 10.0f, 2.0f, AzPhysics::CollisionLayer(1));
        AzPhysics::SimulatedBodyHandle boxHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(0.0f, 0.0f, 30.0f), AZ::Vector3(20.0f, 20.0f, 20.0f), AzPhysics::CollisionLayer(2));

        //Create request
        AzPhysics::CollisionGroup group = AzPhysics::CollisionGroup::All;
        group.SetLayer(AzPhysics::CollisionLayer(0), true);
        group.SetLayer(AzPhysics::CollisionLayer(1), false);
        group.SetLayer(AzPhysics::CollisionLayer(2), true);

        AzPhysics::ShapeCastRequest request = AzPhysics::ShapeCastRequestHelpers::CreateSphereCastRequest(1.5f,
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            AZ::Vector3(0.0f, 0.0f, 1.0f),
            200.0f, AzPhysics::SceneQuery::QueryType::StaticAndDynamic,
            group,
            nullptr);
        request.m_reportMultipleHits = true;

        //Run query
        AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //verify results
        EXPECT_TRUE(results); // there is a hit
        //while all objects created are within the shape cast the capsule should not be included as it is filtered out by the CollisionGroup.
        ASSERT_TRUE(results.m_hits.size() == 2);
        EXPECT_TRUE(results.m_hits[1].m_bodyHandle == sphereHandle);
        EXPECT_TRUE(results.m_hits[0].m_bodyHandle == boxHandle);

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, sphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, capsuleHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, boxHandle);
    }

    TEST_F(PhysXSceneQueryFixture, ShapeCast_AgainstMultipleTouchAndBlockHits_ReturnsClosestBlockAndTouches)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup bodies
        AzPhysics::SimulatedBodyHandle dynamicSphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(20.0f, 0.0f, 0.0f), 10.0f);
        AzPhysics::SimulatedBodyHandle staticBoxHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(40.0f, 0.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 5.0f));
        AzPhysics::SimulatedBodyHandle blockingSphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(60.0f, 0.0f, 0.0f), 5.0f);
        AzPhysics::SimulatedBodyHandle blockingBoxHandle = TestUtils::AddStaticBoxToScene(m_testSceneHandle,
            AZ::Vector3(80.0f, 0.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 5.0f));
        AzPhysics::SimulatedBodyHandle farSphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(120.0f, 0.0f, 0.0f), 10.0f);

        //Create Request
        AzPhysics::ShapeCastRequest request = AzPhysics::ShapeCastRequestHelpers::CreateSphereCastRequest(1.5f,
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            AZ::Vector3(1.0f, 0.0f, 0.0f),
            200.0f, AzPhysics::SceneQuery::QueryType::StaticAndDynamic,
            AzPhysics::CollisionGroup::All,
            [&](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                if (body->m_bodyHandle == blockingBoxHandle || body->m_bodyHandle == blockingSphereHandle)
                {
                    return AzPhysics::SceneQuery::QueryHitType::Block;
                }
                return AzPhysics::SceneQuery::QueryHitType::Touch;
            });
        request.m_reportMultipleHits = true;

        //Run query
        AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //verify results
        EXPECT_TRUE(results); // there is a hit

        // expect 3 hits, the dynamic sphere, static box and blocking sphere. other objects should not be in the list as the block sphere stops the cast.
        ASSERT_EQ(results.m_hits.size(), 3);

        auto GetSimulatedBodyInRayCastHitCallBack = [](AzPhysics::SimulatedBodyHandle bodyHandle)
        {
            return [bodyHandle](const AzPhysics::SceneQueryHit& hit)
            {
                return hit.m_bodyHandle == bodyHandle;
            };
        };
        ASSERT_EQ(1, AZStd::count_if(results.m_hits.begin(), results.m_hits.end(), GetSimulatedBodyInRayCastHitCallBack(dynamicSphereHandle)));
        ASSERT_EQ(1, AZStd::count_if(results.m_hits.begin(), results.m_hits.end(), GetSimulatedBodyInRayCastHitCallBack(staticBoxHandle)));
        ASSERT_EQ(1, AZStd::count_if(results.m_hits.begin(), results.m_hits.end(), GetSimulatedBodyInRayCastHitCallBack(blockingSphereHandle)));

        //clean up
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, dynamicSphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, staticBoxHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, blockingSphereHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, blockingBoxHandle);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, farSphereHandle);
    }

    TEST_F(PhysXSceneQueryFixture, Overlap_MultipleObjects_ReturnsHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup bodies
        TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(10.0f, 0.0f, 0.0f), 3.0f);
        AzPhysics::SimulatedBodyHandle boxHandle = TestUtils::AddBoxToScene(m_testSceneHandle,
            AZ::Vector3(7.0f, 4.0f, 0.0f), AZ::Vector3(1.0f));
        TestUtils::AddCapsuleToScene(m_testSceneHandle,
            AZ::Vector3(15.0f, 0.0f, 0.0f), 3.0f, 1.0f);

        //Create request
        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(AZ::Vector3(3.0f),
            AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f)));

        //Run query
        AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //verify results
        EXPECT_TRUE(results);
        EXPECT_EQ(results.m_hits.size(), 2);

        // boxEntity shouldn't be included in the result
        EXPECT_FALSE(AZStd::any_of(results.m_hits.begin(), results.m_hits.end(),
            [boxHandle](const AzPhysics::SceneQueryHit& hit)
            {
                return hit.m_bodyHandle == boxHandle;
            }));

    }

    TEST_F(PhysXSceneQueryFixture, Overlap_MultipleObjectsUseFriendlyFunctions_ReturnsHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup bodies
        TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(10.0f, 0.0f, 0.0f), 3.0f);
        AzPhysics::SimulatedBodyHandle boxHandle = TestUtils::AddBoxToScene(m_testSceneHandle,
            AZ::Vector3(7.0f, 4.0f, 0.0f), AZ::Vector3(1.0f));
        TestUtils::AddCapsuleToScene(m_testSceneHandle,
            AZ::Vector3(15.0f, 0.0f, 0.0f), 3.0f, 1.0f);

        //Box Overlap Request
        {
            //Create request
            AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(AZ::Vector3(3.0f),
                AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f)));

            //Run query
            AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

            //verify results
            EXPECT_TRUE(results);
            EXPECT_EQ(results.m_hits.size(), 2);

            // boxEntity shouldn't be included in the result
            EXPECT_FALSE(AZStd::any_of(results.m_hits.begin(), results.m_hits.end(),
                [boxHandle](const AzPhysics::SceneQueryHit& hit)
                {
                    return hit.m_bodyHandle == boxHandle;
                }));
        }

        //Sphere Overlap Request
        {
            //Create request
            AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateSphereOverlapRequest(3.0f,
                AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f)));

            //Run query
            AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

            //verify results
            EXPECT_TRUE(results);
            EXPECT_EQ(results.m_hits.size(), 2);

            // boxEntity shouldn't be included in the result
            EXPECT_FALSE(AZStd::any_of(results.m_hits.begin(), results.m_hits.end(),
                [boxHandle](const AzPhysics::SceneQueryHit& hit)
                {
                    return hit.m_bodyHandle == boxHandle;
                }));
        }
    }

    TEST_F(PhysXSceneQueryFixture, Overlap_MultipleObjectsUseFriendlyFunctionsCustomFiltering_ReturnsHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup bodies
        TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(10.0f, 0.0f, 0.0f), 3.0f);
        TestUtils::AddBoxToScene(m_testSceneHandle,
            AZ::Vector3(7.0f, 4.0f, 0.0f), AZ::Vector3(1.0f));
        AzPhysics::SimulatedBodyHandle capsuleHandle = TestUtils::AddCapsuleToScene(m_testSceneHandle,
            AZ::Vector3(15.0f, 0.0f, 0.0f), 3.0f, 1.0f);

        // Here we do an overlap test that covers all objects in the scene
        // However we provide a custom filtering function that filters out a specific entity
        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateCapsuleOverlapRequest(100.0f,
            30.0f,
            AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f)),
            [capsuleHandle](const AzPhysics::SimulatedBody* body, [[maybe_unused]] const Physics::Shape* shape)
            {
                return body->m_bodyHandle != capsuleHandle;
            });

        //Run query
        AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //verify results
        EXPECT_TRUE(results);
        EXPECT_EQ(results.m_hits.size(), 2);

        // boxEntity shouldn't be included in the result
        EXPECT_FALSE(AZStd::any_of(results.m_hits.begin(), results.m_hits.end(),
            [capsuleHandle](const AzPhysics::SceneQueryHit& hit)
            {
                return hit.m_bodyHandle == capsuleHandle;
            }));
    }

    TEST_F(PhysXSceneQueryFixture, Overlap_MultipleObjects_ReturnsFilteredHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup bodies
        AzPhysics::SimulatedBodyHandle sphereHandle = TestUtils::AddSphereToScene(m_testSceneHandle,
            AZ::Vector3(10.0f, 0.0f, 0.0f), 3.0f, AzPhysics::CollisionLayer(0));
        TestUtils::AddBoxToScene(m_testSceneHandle,
            AZ::Vector3(12.0f, 0.0f, 0.0f), AZ::Vector3(1.0f), AzPhysics::CollisionLayer(1));
        TestUtils::AddCapsuleToScene(m_testSceneHandle,
            AZ::Vector3(14.0f, 0.0f, 0.0f), 3.0f, 1.0f, AzPhysics::CollisionLayer(2));

        //Create Request
        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(AZ::Vector3(1.0f),
            AZ::Transform::CreateTranslation(AZ::Vector3(13.0f, 0.0f, 0.0f)));

        request.m_collisionGroup = AzPhysics::CollisionGroup::All;
        request.m_collisionGroup.SetLayer(AzPhysics::CollisionLayer(0), false); // Filter out the sphere
        request.m_collisionGroup.SetLayer(AzPhysics::CollisionLayer(1), true);
        request.m_collisionGroup.SetLayer(AzPhysics::CollisionLayer(2), true);

        //Run query
        AzPhysics::SceneQueryHits results = sceneInterface->QueryScene(m_testSceneHandle, &request);

        //verify results
        EXPECT_TRUE(results);
        EXPECT_EQ(results.m_hits.size(), 2);

        // sphereHandle shouldn't be included in the result
        EXPECT_FALSE(AZStd::any_of(results.m_hits.begin(), results.m_hits.end(),
            [sphereHandle](const AzPhysics::SceneQueryHit& hit)
            {
                return hit.m_bodyHandle == sphereHandle;
            }));
    }

    TEST_F(PhysXSceneQueryFixture, QuerySceneBatch_ReturnsExpectedHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup bodies
        const AZStd::vector<AZ::Vector3> positions = {
            AZ::Vector3(10.0f, 0.0f, 0.0f),
            AZ::Vector3(-10.0f, 0.0f, 0.0f),
            AZ::Vector3(0.0f, 10.0f, 0.0f),
            AZ::Vector3(0.0f, -10.0f, 0.0f),
            AZ::Vector3(0.0f, 0.0f, 10.0f),
            AZ::Vector3(0.0f, 0.0f, -10.0f)
        };

        AZStd::vector<AzPhysics::SimulatedBodyHandle> simBodies;
        for (const AZ::Vector3& pos : positions)
        {
            simBodies.emplace_back(TestUtils::AddSphereToScene(m_testSceneHandle, pos, 1.0f));
        }

        //create the raycast requests
        AzPhysics::SceneQueryRequests requests;
        for (const AZ::Vector3& targetPos : positions)
        {
            AZStd::shared_ptr<AzPhysics::RayCastRequest> request = AZStd::make_shared<AzPhysics::RayCastRequest>();
            request->m_start = AZ::Vector3::CreateZero();
            request->m_direction = targetPos.GetNormalized();
            request->m_distance = 200.0f;

            requests.emplace_back(AZStd::move(request));
        }

        //run query
        AzPhysics::SceneQueryHitsList results = sceneInterface->QuerySceneBatch(m_testSceneHandle, requests);

        //verify each result from each request has the expected targeted simulated body
        EXPECT_EQ(results.size(), requests.size());//results returned should match the requests
        for (size_t i = 0; i< results.size(); i++)
        {
            const AzPhysics::SceneQueryHits& requestResult = results[i];
            const AzPhysics::SimulatedBodyHandle targetHandle = simBodies[i];

            EXPECT_TRUE(requestResult); // should have a result
            EXPECT_EQ(requestResult.m_hits.size(), 1); // each request should only have 1 hit
            EXPECT_TRUE(requestResult.m_hits[0].m_bodyHandle == targetHandle); //returned hit should match expected
        }
    }

    TEST_F(PhysXSceneQueryFixture, QuerySceneBatch_MultipleHits_ReturnsExpectedHits)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup bodies
        const AZStd::vector<AZ::Vector3> positions = {
            //X-axis
            AZ::Vector3(10.0f, 0.0f, 0.0f),
            AZ::Vector3(15.0f, 0.0f, 0.0f),
            //Y-axis
            AZ::Vector3(0.0f, 10.0f, 0.0f),
            AZ::Vector3(0.0f, 15.0f, 0.0f),
            //Z-axis
            AZ::Vector3(0.0f, 0.0f, 10.0f),
            AZ::Vector3(0.0f, 0.0f, 15.0f)
        };

        AZStd::vector<AzPhysics::SimulatedBodyHandle> simBodies;
        simBodies.reserve(positions.size());
        for (const AZ::Vector3& pos : positions)
        {
            simBodies.emplace_back(TestUtils::AddSphereToScene(m_testSceneHandle, pos, 1.0f));
        }

        //create the raycast requests
        AzPhysics::SceneQueryRequests requests;

        //request along the x Axis
        AZStd::shared_ptr<AzPhysics::RayCastRequest> request = AZStd::make_shared<AzPhysics::RayCastRequest>();
        request->m_start = AZ::Vector3::CreateZero();
        request->m_direction = AZ::Vector3::CreateAxisX(1.0f);
        request->m_distance = 200.0f;
        request->m_reportMultipleHits = true;
        requests.emplace_back(AZStd::move(request));

        //request along the y Axis
        request = AZStd::make_shared<AzPhysics::RayCastRequest>();
        request->m_start = AZ::Vector3::CreateZero();
        request->m_direction = AZ::Vector3::CreateAxisY(1.0f);
        request->m_distance = 200.0f;
        request->m_reportMultipleHits = true;
        requests.emplace_back(AZStd::move(request));

        //request along the z Axis
        request = AZStd::make_shared<AzPhysics::RayCastRequest>();
        request->m_start = AZ::Vector3::CreateZero();
        request->m_direction = AZ::Vector3::CreateAxisZ(1.0f);
        request->m_distance = 200.0f;
        request->m_reportMultipleHits = true;
        requests.emplace_back(AZStd::move(request));
        
        //run query
        AzPhysics::SceneQueryHitsList results = sceneInterface->QuerySceneBatch(m_testSceneHandle, requests);

        //verify each result from each request has the expected targeted simulated body
        EXPECT_EQ(results.size(), requests.size());//results returned should match the requests
        for (size_t i = 0; i < results.size(); i++)
        {
            const AzPhysics::SceneQueryHits& requestResult = results[i];

            EXPECT_TRUE(requestResult); // should have a result
            EXPECT_EQ(requestResult.m_hits.size(), 2); // each request should only have 2 hits
            for (size_t j = 0; j < requestResult.m_hits.size(); j++)
            {
                const AzPhysics::SimulatedBodyHandle targetHandle = simBodies[(i*2)+j];//index into the simBodies, ordered as [request1,request1, request2,request2, ...]
                EXPECT_TRUE(requestResult.m_hits[j].m_bodyHandle == targetHandle); //returned hit should match expected
            }
        }
    }
}
