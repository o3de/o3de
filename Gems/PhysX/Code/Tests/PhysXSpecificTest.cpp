/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhysXTestFixtures.h"
#include "PhysXTestUtil.h"

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AZTestShared/Utils/Utils.h>

#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

#include <RigidBodyStatic.h>
#include <SphereColliderComponent.h>
#include <Utils.h>

#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/Material/PhysXMaterialConfiguration.h>
#include <Scene/PhysXScene.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysX
{
    class PhysXSpecificTest
        : public PhysXDefaultWorldTest
        , public UnitTest::TraceBusRedirector
    {
    protected:


        float tolerance = 1e-3f;
    };


    namespace PhysXTests
    {
        typedef EntityPtr(* EntityFactoryFunc)(AzPhysics::SceneHandle, const AZ::Vector3&, const char*);
    }

    class PhysXEntityFactoryParamTest
        : public PhysXSpecificTest
        , public ::testing::WithParamInterface<PhysXTests::EntityFactoryFunc>
    {
    };

    void SetCollisionLayerName(AZ::u8 index, const AZStd::string& name)
    {
        AZ::Interface<Physics::CollisionRequests>::Get()->SetCollisionLayerName(index, name);
    }

    void CreateCollisionGroup(const AzPhysics::CollisionGroup& group, const AZStd::string& name)
    {
        AZ::Interface<Physics::CollisionRequests>::Get()->CreateCollisionGroup(name, group);
    }
    
    void SanityCheckValidFrustumParams(const AZStd::vector<AZ::Vector3>& points, float validHeight, float validBottomRadius, float validTopRadius, AZ::u8 validSubdivisions)
    {
        double rad = 0;
        const double step = AZ::Constants::TwoPi / aznumeric_cast<double>(validSubdivisions);
        const float halfHeight = validHeight * 0.5f;

        for (auto i = 0; i < points.size() / 2; i++)
        {
            // Canonical way to plot points on the circumference a cicle
            // If any attempt to refactor/optimize the implemented algorithm fails, this test will fail
            const float x = aznumeric_cast<float>(std::cos(rad));
            const float y = aznumeric_cast<float>(std::sin(rad));

            // Top face point is offset half the height along the positive z axis
            {
                const AZ::Vector3& p = points[i * 2];

                EXPECT_FLOAT_EQ(p.GetX(), x * validTopRadius);
                EXPECT_FLOAT_EQ(p.GetY(), y * validTopRadius);
                EXPECT_FLOAT_EQ(p.GetZ(), +halfHeight);
            }

            // Bottom face point is offset half the height along the negative z axis
            {
                const AZ::Vector3& p = points[i * 2 + 1];

                EXPECT_FLOAT_EQ(p.GetX(), x * validBottomRadius);
                EXPECT_FLOAT_EQ(p.GetY(), y * validBottomRadius);
                EXPECT_FLOAT_EQ(p.GetZ(), -halfHeight);
            }

            rad += step;
        }
    }

    // Helper functions for calculating the volume
    float GetShapeVolume(const Physics::BoxShapeConfiguration& box)
    {
        return box.m_dimensions.GetX() * box.m_dimensions.GetY() * box.m_dimensions.GetZ() *
            box.m_scale.GetX() * box.m_scale.GetY() * box.m_scale.GetZ();
    }

    float GetShapeVolume(const Physics::SphereShapeConfiguration& sphere)
    {
        return 4.0f * AZ::Constants::Pi * sphere.m_radius * sphere.m_radius * sphere.m_radius / 3.0f;
    }

    TEST_F(PhysXSpecificTest, VectorConversion_ConvertToPxVec3_ConvertedVectorsCorrect)
    {
        AZ::Vector3 lyA(3.0f, -4.0f, 12.0f);
        AZ::Vector3 lyB(-8.0f, 1.0f, -4.0f);

        physx::PxVec3 pxA = PxMathConvert(lyA);
        physx::PxVec3 pxB = PxMathConvert(lyB);

        EXPECT_NEAR(pxA.magnitudeSquared(), 169.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxB.magnitudeSquared(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.dot(pxB), -76.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).x, 4.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).y, -84.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).z, -29.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, VectorConversion_ConvertToLyVec3_ConvertedVectorsCorrect)
    {
        physx::PxVec3 pxA(3.0f, -4.0f, 12.0f);
        physx::PxVec3 pxB(-8.0f, 1.0f, -4.0f);

        AZ::Vector3 lyA = PxMathConvert(pxA);
        AZ::Vector3 lyB = PxMathConvert(pxB);

        EXPECT_NEAR(lyA.GetLengthSq(), 169.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyB.GetLengthSq(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Dot(lyB), -76.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetX(), 4.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetY(), -84.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetZ(), -29.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, ExtendedVectorConversion_ConvertToPxExtendedVec3_ConvertedVectorsCorrect)
    {
        AZ::Vector3 lyA(3.0f, -4.0f, 12.0f);
        AZ::Vector3 lyB(-8.0f, 1.0f, -4.0f);

        physx::PxExtendedVec3 pxA = PxMathConvertExtended(lyA);
        physx::PxExtendedVec3 pxB = PxMathConvertExtended(lyB);

        EXPECT_NEAR(pxA.magnitudeSquared(), 169.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxB.magnitudeSquared(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).x, 4.0, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).y, -84.0, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).z, -29.0, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, ExtendedVectorConversion_ConvertToLyVec3_ConvertedVectorsCorrect)
    {
        physx::PxExtendedVec3 pxA(3.0, -4.0, 12.0);
        physx::PxExtendedVec3 pxB(-8.0, 1.0, -4.0);

        AZ::Vector3 lyA = PxMathConvertExtended(pxA);
        AZ::Vector3 lyB = PxMathConvertExtended(pxB);

        EXPECT_NEAR(lyA.GetLengthSq(), 169.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyB.GetLengthSq(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Dot(lyB), -76.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetX(), 4.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetY(), -84.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetZ(), -29.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, QuaternionConversion_ConvertToPxQuat_ConvertedQuatsCorrect)
    {
        AZ::Quaternion lyQ = AZ::Quaternion(9.0f, -8.0f, -4.0f, 8.0f) / 15.0f;
        physx::PxQuat pxQ = PxMathConvert(lyQ);
        physx::PxVec3 pxV = pxQ.rotate(physx::PxVec3(-8.0f, 1.0f, -4.0f));

        EXPECT_NEAR(pxQ.magnitudeSquared(), 1.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxQ.getImaginaryPart().magnitudeSquared(), 161.0f / 225.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxQ.w, 8.0f / 15.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.magnitudeSquared(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.x, 8.0f / 9.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.y, 403.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.z, 4.0f / 45.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, QuaternionConversion_ConvertToLyQuat_ConvertedQuatsCorrect)
    {
        physx::PxQuat pxQ = physx::PxQuat(9.0f, -8.0f, -4.0f, 8.0f) * (1.0f / 15.0f);
        AZ::Quaternion lyQ = PxMathConvert(pxQ);
        AZ::Vector3 lyV = lyQ.TransformVector(AZ::Vector3(-8.0f, 1.0f, -4.0f));

        EXPECT_NEAR(lyQ.GetLengthSq(), 1.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyQ.GetImaginary().GetLengthSq(), 161.0f / 225.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyQ.GetW(), 8.0f / 15.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetLengthSq(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetX(), 8.0f / 9.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetY(), 403.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetZ(), 4.0f / 45.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, TransformConversion_ConvertToPxTransform_ConvertedTransformsCorrect)
    {
        // create an AZ::Transform and convert it to a pxTransform
        const AZ::Vector3 eulerAngles(40.0f, 25.0f, 37.0f);
        AZ::Transform lyTm;
        lyTm.SetFromEulerDegrees(eulerAngles);
        physx::PxTransform pxTm = PxMathConvert(lyTm);

        // transform a vector with each transform
        const float x = 0.8f;
        const float y = -1.4f;
        const float z = 0.3f;
        AZ::Vector3 lyVec3 = lyTm.TransformPoint(AZ::Vector3(x, y, z));
        physx::PxVec3 pxVec3 = pxTm.transform(physx::PxVec3(x, y, z));

        // check the results are close for both transforms
        EXPECT_NEAR(pxVec3.x, lyVec3.GetX(), PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxVec3.y, lyVec3.GetY(), PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxVec3.z, lyVec3.GetZ(), PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, TransformConversion_ConvertToLyTransform_ConvertedTransformsCorrect)
    {
        physx::PxTransform pxTm(physx::PxVec3(2.0f, 10.0f, 9.0f), physx::PxQuat(6.0f, -8.0f, -5.0f, 10.0f) * (1.0f / 15.0f));
        AZ::Transform lyTm = PxMathConvert(pxTm);
        AZ::Vector3 lyV = lyTm.TransformPoint(AZ::Vector3(4.0f, -12.0f, 3.0f));

        EXPECT_NEAR(lyV.GetX(), -14.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetY(), 22.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetZ(), 4.0f / 9.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, RigidBody_GetNativeShape_ReturnsCorrectShape)
    {
        AZ::Vector3 halfExtents(1.0f, 2.0f, 3.0f);
        Physics::BoxShapeConfiguration shapeConfig(halfExtents * 2.0f);
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi);
        AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);

        AzPhysics::RigidBodyConfiguration rigidBodyConfiguration;
        rigidBodyConfiguration.m_colliderAndShapeData = shape;
        AzPhysics::RigidBody* rigidBody = nullptr;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfiguration);
            rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
        }
        ASSERT_TRUE(rigidBody != nullptr);

        auto nativeShape = rigidBody->GetShape(0);
        ASSERT_TRUE(nativeShape != nullptr);

        {
            auto* actor = static_cast<physx::PxRigidDynamic*>(rigidBody->GetNativePointer());
            PHYSX_SCENE_READ_LOCK(actor->getScene());

            auto pxShape = AZStd::rtti_pointer_cast<PhysX::Shape>(shape);
            ASSERT_TRUE(pxShape->GetPxShape()->getGeometryType() == physx::PxGeometryType::eBOX);

            physx::PxBoxGeometry boxGeometry;
            pxShape->GetPxShape()->getBoxGeometry(boxGeometry);

            EXPECT_NEAR(boxGeometry.halfExtents.x, halfExtents.GetX(), PhysXSpecificTest::tolerance);
            EXPECT_NEAR(boxGeometry.halfExtents.y, halfExtents.GetY(), PhysXSpecificTest::tolerance);
            EXPECT_NEAR(boxGeometry.halfExtents.z, halfExtents.GetZ(), PhysXSpecificTest::tolerance);
        }
    }

    auto entityFactories = { TestUtils::AddUnitTestObject<BoxColliderComponent>, TestUtils::AddUnitTestBoxComponentsMix };
    INSTANTIATE_TEST_CASE_P(DifferentBoxes, PhysXEntityFactoryParamTest, ::testing::ValuesIn(entityFactories));

    TEST_F(PhysXSpecificTest, RigidBody_GetNativeType_ReturnsPhysXRigidBodyType)
    {
        AzPhysics::RigidBodyConfiguration rigidBodyConfiguration;
        AzPhysics::RigidBody* rigidBody = nullptr;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfiguration);
            rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
        }
        EXPECT_EQ(rigidBody->GetNativeType(), AZ::Crc32("PhysXRigidBody"));
    }

    TEST_F(PhysXSpecificTest, RigidBody_GetNativePointer_ReturnsValidPointer)
    {
        AzPhysics::RigidBodyConfiguration rigidBodyConfiguration;
        AzPhysics::RigidBody* rigidBody = nullptr;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfiguration);
            rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
        }
        physx::PxBase* nativePointer = static_cast<physx::PxBase*>(rigidBody->GetNativePointer());
        EXPECT_TRUE(strcmp(nativePointer->getConcreteTypeName(), "PxRigidDynamic") == 0);
    }

    TEST_F(PhysXSpecificTest, TriggerArea_RigidBodyEnteringAndLeavingTrigger_EnterLeaveCallbackCalled)
    {
        // set up a trigger box
        auto triggerBox = TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 12.0f));
        auto* triggerBody = azdynamic_cast<PhysX::StaticRigidBody*>(triggerBox->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        auto triggerShape = triggerBody->GetShape(0);

        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(triggerBox->GetId());

        // Create a test box above the trigger so when it falls down it'd enter and leave the trigger box
        auto testBox = TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 16.0f), "TestBox");
        auto testBoxBody = testBox->FindComponent<RigidBodyComponent>()->GetRigidBody();
        auto testBoxShape = testBoxBody->GetShape(0);

        // run the simulation for a while
        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 500);

        const auto& enteredEvents = testTriggerAreaNotificationListener.GetEnteredEvents();
        const auto& exitedEvents = testTriggerAreaNotificationListener.GetExitedEvents();

        ASSERT_EQ(enteredEvents.size(), 1);
        ASSERT_EQ(exitedEvents.size(), 1);

        EXPECT_EQ(enteredEvents[0].m_triggerBody, triggerBody);
        EXPECT_EQ(enteredEvents[0].m_triggerShape, triggerShape.get());
        EXPECT_EQ(enteredEvents[0].m_otherBody, testBoxBody);
        EXPECT_EQ(enteredEvents[0].m_otherShape, testBoxShape.get());

        EXPECT_EQ(exitedEvents[0].m_triggerBody, triggerBody);
        EXPECT_EQ(exitedEvents[0].m_triggerShape, triggerShape.get());
        EXPECT_EQ(exitedEvents[0].m_otherBody, testBoxBody);
        EXPECT_EQ(exitedEvents[0].m_otherShape, testBoxShape.get());
    }

    TEST_F(PhysXSpecificTest, TriggerArea_RigidBodiesEnteringAndLeavingTriggers_EnterLeaveCallbackCalled)
    {
        // set up triggers
        AZStd::vector<EntityPtr> triggers =
        {
            TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 12.0f)),
            TestUtils::CreateTriggerAtPosition<SphereColliderComponent>(AZ::Vector3(0.0f, 0.0f, 8.0f))
        };

        // set up dynamic objs
        AZStd::vector<EntityPtr> testBoxes =
        {
            TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 16.0f), "TestBox"),
            TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 18.0f), "TestBox2")
        };

        // set up listeners on triggers
        TestTriggerAreaNotificationListener testTriggerBoxNotificationListener(triggers[0]->GetId());
        TestTriggerAreaNotificationListener testTriggerSphereNotificationListener(triggers[1]->GetId());

        // run the simulation for a while
        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 500);

        for (const auto& triggerListener : {&testTriggerBoxNotificationListener, &testTriggerSphereNotificationListener})
        {
            const auto& enteredEvents = triggerListener->GetEnteredEvents();
            ASSERT_EQ(2, enteredEvents.size());
            EXPECT_EQ(enteredEvents[0].m_otherBody, testBoxes[0]->FindComponent<RigidBodyComponent>()->GetRigidBody());
            EXPECT_EQ(enteredEvents[0].m_otherShape, testBoxes[0]->FindComponent<RigidBodyComponent>()->GetRigidBody()->GetShape(0).get());
            EXPECT_EQ(enteredEvents[1].m_otherBody, testBoxes[1]->FindComponent<RigidBodyComponent>()->GetRigidBody());
            EXPECT_EQ(enteredEvents[1].m_otherShape, testBoxes[1]->FindComponent<RigidBodyComponent>()->GetRigidBody()->GetShape(0).get());

            const auto& exitedEvents = triggerListener->GetExitedEvents();
            ASSERT_EQ(2, enteredEvents.size());
            EXPECT_EQ(exitedEvents[0].m_otherBody, testBoxes[0]->FindComponent<RigidBodyComponent>()->GetRigidBody());
            EXPECT_EQ(exitedEvents[0].m_otherShape, testBoxes[0]->FindComponent<RigidBodyComponent>()->GetRigidBody()->GetShape(0).get());
            EXPECT_EQ(exitedEvents[1].m_otherBody, testBoxes[1]->FindComponent<RigidBodyComponent>()->GetRigidBody());
            EXPECT_EQ(exitedEvents[1].m_otherShape, testBoxes[1]->FindComponent<RigidBodyComponent>()->GetRigidBody()->GetShape(0).get());
        }
    }

    TEST_F(PhysXSpecificTest, RigidBody_CollisionCallback_SimpleCallbackOfTwoSpheres)
    {
        auto obj01 = TestUtils::AddUnitTestObject<SphereColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 10.0f), "TestSphere01");
        auto obj02 = TestUtils::AddUnitTestObject<SphereColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f), "TestSphere01");

        auto body01 = obj01->FindComponent<RigidBodyComponent>()->GetRigidBody();
        auto body02 = obj02->FindComponent<RigidBodyComponent>()->GetRigidBody();

        auto shape01 = body01->GetShape(0).get();
        auto shape02 = body02->GetShape(0).get();

        CollisionCallbacksListener listener01(obj01->GetId());
        CollisionCallbacksListener listener02(obj02->GetId());

        Physics::RigidBodyRequestBus::Event(obj02->GetId(), &Physics::RigidBodyRequestBus::Events::ApplyLinearImpulse, AZ::Vector3(0.0f, 0.0f, 50.0f));

        // run the simulation for a while
        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 500);

        // We expect to have two (CollisionBegin and CollisionEnd) events for both objects
        ASSERT_EQ(listener01.m_beginCollisions.size(), 1);
        ASSERT_EQ(listener01.m_endCollisions.size(), 1);
        ASSERT_EQ(listener02.m_beginCollisions.size(), 1);
        ASSERT_EQ(listener02.m_endCollisions.size(), 1);

        // First collision recorded is CollisionBegin event
        auto collisionBegin01 = listener01.m_beginCollisions[0];
        EXPECT_EQ(collisionBegin01.m_body2->GetEntityId(), obj02->GetId());
        EXPECT_EQ(collisionBegin01.m_body2, body02);
        EXPECT_EQ(collisionBegin01.m_shape2, shape02);

        // Checkes one of the collision point details
        ASSERT_EQ(collisionBegin01.m_contacts.size(), 1);
        EXPECT_NEAR(collisionBegin01.m_contacts[0].m_impulse.GetZ(), -37.12f, 0.01f);
        float dotNormal = collisionBegin01.m_contacts[0].m_normal.Dot(AZ::Vector3(0.0f, 0.0f, -1.0f));
        EXPECT_NEAR(dotNormal, 1.0f, 0.01f);
        EXPECT_NEAR(collisionBegin01.m_contacts[0].m_separation, -0.12, 0.01f);

        // Second collision recorded is CollisionExit event
        auto collisionEnd01 = listener01.m_endCollisions[0];
        EXPECT_EQ(collisionEnd01.m_body2->GetEntityId(), obj02->GetId());
        EXPECT_EQ(collisionEnd01.m_body2, body02);
        EXPECT_EQ(collisionEnd01.m_shape2, shape02);

        // Some checks for the second sphere
        auto collisionBegin02 = listener02.m_beginCollisions[0];
        EXPECT_EQ(collisionBegin02.m_body2->GetEntityId(), obj01->GetId());
        EXPECT_EQ(collisionBegin02.m_body2, body01);
        EXPECT_EQ(collisionBegin02.m_shape2, shape01);

        auto collisionEnd02 = listener02.m_endCollisions[0];
        EXPECT_EQ(collisionEnd02.m_body2->GetEntityId(), obj01->GetId());
        EXPECT_EQ(collisionEnd02.m_body2, body01);
        EXPECT_EQ(collisionEnd02.m_shape2, shape01);
    }

    TEST_F(PhysXSpecificTest, RigidBody_CollisionCallback_SimpleCallbackSphereFallingOnStaticBox)
    {
        auto obj01 = TestUtils::AddUnitTestObject<SphereColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 10.0f), "TestSphere01");
        auto obj02 = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f), "TestBox01");

        auto body01 = obj01->FindComponent<RigidBodyComponent>()->GetRigidBody();
        auto* body02 = azdynamic_cast<PhysX::StaticRigidBody*>(obj02->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        auto shape01 = body01->GetShape(0).get();
        auto shape02 = body02->GetShape(0).get();

        CollisionCallbacksListener listener01(obj01->GetId());
        CollisionCallbacksListener listener02(obj02->GetId());

        // run the simulation for a while
        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 500);

        // Ball should bounce at least 2 times, generating CollisionBegin and CollisionEnd events
        ASSERT_GE(listener01.m_beginCollisions.size(), 2);
        ASSERT_GE(listener01.m_endCollisions.size(), 2);
        ASSERT_GE(listener02.m_beginCollisions.size(), 2);
        ASSERT_GE(listener02.m_endCollisions.size(), 2);

        EXPECT_EQ(listener01.m_beginCollisions[0].m_body2->GetEntityId(), obj02->GetId());
        EXPECT_EQ(listener01.m_beginCollisions[0].m_body2, body02);
        EXPECT_EQ(listener01.m_beginCollisions[0].m_shape2, shape02);

        EXPECT_EQ(listener02.m_beginCollisions[0].m_body2->GetEntityId(), obj01->GetId());
        EXPECT_EQ(listener02.m_beginCollisions[0].m_body2, body01);
        EXPECT_EQ(listener02.m_beginCollisions[0].m_shape2, shape01);
    }

     TEST_F(PhysXSpecificTest, CollisionFiltering_CollisionLayers_CombineLayersIntoGroup)
    {
        // Start with empty group
        AzPhysics::CollisionGroup group = AzPhysics::CollisionGroup::None;
        AzPhysics::CollisionLayer layer1(1);
        AzPhysics::CollisionLayer layer2(2);

        // Check nothing is set
        EXPECT_FALSE(group.IsSet(layer1));
        EXPECT_FALSE(group.IsSet(layer2));

        // Combine layers into group
        group = layer1 | layer2;

        // Check they are set
        EXPECT_TRUE(group.IsSet(layer1));
        EXPECT_TRUE(group.IsSet(layer2));
    }

    TEST_F(PhysXSpecificTest, CollisionFiltering_CollisionLayers_ConstructLayerByName)
    {
        // Set layer names
        SetCollisionLayerName(1, "Layer1");
        SetCollisionLayerName(2, "Layer2");
        SetCollisionLayerName(3, "Layer3");

        // Lookup layers by name
        AzPhysics::CollisionLayer layer1("Layer1");
        AzPhysics::CollisionLayer layer2("Layer2");
        AzPhysics::CollisionLayer layer3("Layer3");

        // Check they match what was set before
        EXPECT_EQ(1, layer1.GetIndex());
        EXPECT_EQ(2, layer2.GetIndex());
        EXPECT_EQ(3, layer3.GetIndex());
    }

    TEST_F(PhysXSpecificTest, CollisionFiltering_CollisionGroups_AppendLayerToGroup)
    {
        // Start with empty group
        AzPhysics::CollisionGroup group = AzPhysics::CollisionGroup::None;
        AzPhysics::CollisionLayer layer1(1);

        EXPECT_FALSE(group.IsSet(layer1));

        // Append layer to group
        group = group | layer1;

        // Check its set
        EXPECT_TRUE(group.IsSet(layer1));
    }

    TEST_F(PhysXSpecificTest, CollisionFiltering_CollisionGroups_ConstructGroupByName)
    {
        // Create a collision group preset from layers
        CreateCollisionGroup(AzPhysics::CollisionLayer(5) | AzPhysics::CollisionLayer(13), "TestGroup");

        // Lookup the group by name
        AzPhysics::CollisionGroup group("TestGroup");

        // Check it looks correct
        EXPECT_TRUE(group.IsSet(AzPhysics::CollisionLayer(5)));
        EXPECT_TRUE(group.IsSet(AzPhysics::CollisionLayer(13)));
    }

    TEST_F(PhysXSpecificTest, RigidBody_CenterOfMassOffsetComputed)
    {
        AZ::Vector3 halfExtents(1.0f, 2.0f, 3.0f);
        auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(halfExtents * 2.0f);
        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_rotation = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi);

        AzPhysics::RigidBodyConfiguration rigidBodyConfiguration;
        rigidBodyConfiguration.m_computeCenterOfMass = true;
        rigidBodyConfiguration.m_computeInertiaTensor = true;
        rigidBodyConfiguration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig);
        AzPhysics::RigidBody* rigidBody = nullptr;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfiguration);
            rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
        }
        ASSERT_TRUE(rigidBody != nullptr);

        auto com = rigidBody->GetCenterOfMassLocal();
        EXPECT_TRUE(com.IsClose(AZ::Vector3::CreateZero(), PhysXSpecificTest::tolerance));
    }

    TEST_F(PhysXSpecificTest, RigidBody_CenterOfMassOffsetSpecified)
    {
        AZ::Vector3 halfExtents(1.0f, 2.0f, 3.0f);
        auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(halfExtents * 2.0f);
        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_rotation = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi);

        AzPhysics::RigidBodyConfiguration rigidBodyConfiguration;
        rigidBodyConfiguration.m_computeCenterOfMass = false;
        rigidBodyConfiguration.m_centerOfMassOffset = AZ::Vector3::CreateOne();
        rigidBodyConfiguration.m_computeInertiaTensor = true;
        rigidBodyConfiguration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig);

        AzPhysics::RigidBody* rigidBody = nullptr;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfiguration);
            rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
        }
        ASSERT_TRUE(rigidBody != nullptr);

        auto com = rigidBody->GetCenterOfMassLocal();
        EXPECT_TRUE(com.IsClose(AZ::Vector3::CreateOne(), PhysXSpecificTest::tolerance));
    }

    TEST_F(PhysXSpecificTest, TriggerArea_BodyDestroyedInsideTrigger_OnTriggerExitEventRaised)
    {
        // set up a trigger box
        auto triggerBox = TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto* triggerBody = azdynamic_cast<PhysX::StaticRigidBody*>(triggerBox->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        // Create a test box above the trigger so when it falls down it'd enter and leave the trigger box
        auto testBox = TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.5f), "TestBox");
        auto testBoxBody = testBox->FindComponent<RigidBodyComponent>()->GetRigidBody();

        // Listen for trigger events on the box
        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(triggerBox->GetId());

        // run the simulation for a while
        const auto& enteredEvents = testTriggerAreaNotificationListener.GetEnteredEvents();
        const auto& exitedEvents = testTriggerAreaNotificationListener.GetExitedEvents();

        for (int timeStep = 0; timeStep < 100; timeStep++)
        {
            m_defaultScene->StartSimulation(AzPhysics::SystemConfiguration::DefaultFixedTimestep);
            m_defaultScene->FinishSimulation();

            // Body entered the trigger area, kill it!!!
            if (enteredEvents.size() > 0 && testBox != nullptr)
            {
                testBox.reset();
            }
        }
        static_cast<PhysX::PhysXScene*>(m_defaultScene)->FlushTransformSync();


        ASSERT_EQ(testBox, nullptr);
        ASSERT_EQ(enteredEvents.size(), 1);
        ASSERT_EQ(exitedEvents.size(), 1);

        EXPECT_EQ(enteredEvents[0].m_triggerBody, triggerBody);
        EXPECT_EQ(enteredEvents[0].m_otherBody, testBoxBody);

        EXPECT_EQ(exitedEvents[0].m_triggerBody, triggerBody);
        EXPECT_EQ(exitedEvents[0].m_otherBody, testBoxBody);
    }

    TEST_F(PhysXSpecificTest, TriggerArea_StaticBodyDestroyedInsideDynamicTrigger_OnTriggerExitEventRaised)
    {
        // Set up a static non trigger box
        auto staticBox = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(staticBox->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        // Create a test trigger box above the static box so when it falls down it'd enter and leave the trigger box
        auto dynamicTrigger = TestUtils::CreateDynamicTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 5.0f));
        auto dynamicBody = dynamicTrigger->FindComponent<RigidBodyComponent>()->GetRigidBody();

        // Listen for trigger events on the box
        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(dynamicTrigger->GetId());

        // run the simulation for a while
        const auto& enteredEvents = testTriggerAreaNotificationListener.GetEnteredEvents();
        const auto& exitedEvents = testTriggerAreaNotificationListener.GetExitedEvents();

        for (int timeStep = 0; timeStep < 100; timeStep++)
        {
            m_defaultScene->StartSimulation(AzPhysics::SystemConfiguration::DefaultFixedTimestep);
            m_defaultScene->FinishSimulation();

            // Body entered the trigger area, kill it!!!
            if (enteredEvents.size() > 0 && staticBox != nullptr)
            {
                staticBox.reset();
            }
        }
        static_cast<PhysX::PhysXScene*>(m_defaultScene)->FlushTransformSync();

        ASSERT_EQ(staticBox, nullptr);
        ASSERT_EQ(enteredEvents.size(), 1);
        ASSERT_EQ(exitedEvents.size(), 1);

        EXPECT_EQ(enteredEvents[0].m_triggerBody, dynamicBody);
        EXPECT_EQ(enteredEvents[0].m_otherBody, staticBody);

        EXPECT_EQ(exitedEvents[0].m_triggerBody, dynamicBody);
        EXPECT_EQ(exitedEvents[0].m_otherBody, staticBody);
    }

    TEST_F(PhysXSpecificTest, TriggerArea_BodyDestroyedOnTriggerEnter_DoesNotCrash)
    {
        // Given a rigid body falling into a trigger.
        auto triggerBox = TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto testBox = TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on trigger enter event.
        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(triggerBox->GetId());
        testTriggerAreaNotificationListener.m_onTriggerEnter = [&]([[maybe_unused]] const AzPhysics::TriggerEvent& triggerEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        TestUtils::UpdateScene(m_defaultScene, 1.0f / 30.0f, 30);
        
        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, TriggerArea_BodyDestroyedOnTriggerExit_DoesNotCrash)
    {
        // Given a rigid body falling into a trigger.
        auto triggerBox = TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto testBox = TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on trigger enter event.
        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(triggerBox->GetId());
        testTriggerAreaNotificationListener.m_onTriggerExit = [&]([[maybe_unused]] const AzPhysics::TriggerEvent& triggerEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        TestUtils::UpdateScene(m_defaultScene, 1.0f / 30.0f, 30);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, CollisionEvents_BodyDestroyedOnCollisionBegin_DoesNotCrash)
    {
        // Given a rigid body falling onto a static box.
        auto staticBox = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f), "StaticTestBox");
        auto testBox = TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on collision begin event.
        CollisionCallbacksListener collisionListener(testBox->GetId());
        collisionListener.m_onCollisionBegin = [&]([[maybe_unused]] const AzPhysics::CollisionEvent& collisionEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        TestUtils::UpdateScene(m_defaultScene, 1.0f / 30.0f, 30);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, CollisionEvents_BodyDestroyedOnCollisionPersist_DoesNotCrash)
    {
        // Given a rigid body falling onto a static box.
        auto staticBox = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f), "StaticTestBox");
        auto testBox = TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on collision begin event.
        CollisionCallbacksListener collisionListener(testBox->GetId());
        collisionListener.m_onCollisionPersist = [&]([[maybe_unused]] const AzPhysics::CollisionEvent& collisionEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        TestUtils::UpdateScene(m_defaultScene, 1.0f / 30.0f, 30);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, CollisionEvents_BodyDestroyedOnCollisionEnd_DoesNotCrash)
    {
        // Given a rigid body falling onto a static box.
        auto staticBox = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f), "StaticTestBox");
        auto testBox = TestUtils::AddUnitTestObject(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on collision begin event.
        CollisionCallbacksListener collisionListener(testBox->GetId());
        collisionListener.m_onCollisionEnd = [&]([[maybe_unused]] const AzPhysics::CollisionEvent& collisionEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        TestUtils::UpdateScene(m_defaultScene, 1.0f / 30.0f, 30);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, RigidBody_ConvexRigidBodyCreatedFromCookedMesh_CachedMeshObjectCreated)
    {
        // Create rigid body
        AzPhysics::RigidBodyConfiguration rigidBodyConfiguration;
        AzPhysics::RigidBody* rigidBody = nullptr;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfiguration);
            rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
        }
        ASSERT_TRUE(rigidBody != nullptr);

        // Generate input data
        const PointList testPoints = TestUtils::GeneratePyramidPoints(1.0f);
        AZStd::vector<AZ::u8> cookedData;
        bool cookingResult = false;
        Physics::SystemRequestBus::BroadcastResult(cookingResult, &Physics::SystemRequests::CookConvexMeshToMemory,
            testPoints.data(), static_cast<AZ::u32>(testPoints.size()), cookedData);
        EXPECT_TRUE(cookingResult);

        // Setup shape & collider configurations
        Physics::CookedMeshShapeConfiguration shapeConfig;
        shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(), 
            Physics::CookedMeshShapeConfiguration::MeshType::Convex);

        Physics::ColliderConfiguration colliderConfig;

        // Create the first shape
        AZStd::shared_ptr<Physics::Shape> firstShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        ASSERT_TRUE(firstShape != nullptr);

        rigidBody->AddShape(firstShape);

        // Validate the cached mesh is there
        EXPECT_NE(shapeConfig.GetCachedNativeMesh(), nullptr);

        // Make some changes in the configuration for the second shape
        colliderConfig.m_position.SetX(1.0f);
        shapeConfig.m_scale = AZ::Vector3(2.0f, 2.0f, 2.0f);

        // Create the second shape
        AZStd::shared_ptr<Physics::Shape> secondShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        ASSERT_TRUE(secondShape != nullptr);
        
        rigidBody->AddShape(secondShape);

        AZ::Vector3 initialPosition = rigidBody->GetPosition();

        // Tick the world
        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 20);

        // Verify the actor has moved
        EXPECT_NE(rigidBody->GetPosition(), initialPosition);
    }

    TEST_F(PhysXSpecificTest, RigidBody_TriangleMeshRigidBodyCreatedFromCookedMesh_CachedMeshObjectCreated)
    {
        // Generate input data
        VertexIndexData cubeMeshData = TestUtils::GenerateCubeMeshData(3.0f);
        AZStd::vector<AZ::u8> cookedData;
        bool cookingResult = false;
        Physics::SystemRequestBus::BroadcastResult(cookingResult, &Physics::SystemRequests::CookTriangleMeshToMemory,
            cubeMeshData.first.data(), static_cast<AZ::u32>(cubeMeshData.first.size()),
            cubeMeshData.second.data(), static_cast<AZ::u32>(cubeMeshData.second.size()),
            cookedData);
        EXPECT_TRUE(cookingResult);

        // Setup shape & collider configurations
        Physics::CookedMeshShapeConfiguration shapeConfig;
        shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(),
            Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);

        Physics::ColliderConfiguration colliderConfig;

        // Create the first shape
        AZStd::shared_ptr<Physics::Shape> firstShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        AZ_Assert(firstShape != nullptr, "Failed to create a shape from cooked data");

        // Create static rigid body
        AzPhysics::StaticRigidBodyConfiguration staticBodyConfiguration;
        staticBodyConfiguration.m_colliderAndShapeData = firstShape;

        AzPhysics::StaticRigidBody* rigidBody = nullptr;
        AzPhysics::SimulatedBodyHandle rigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            rigidBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &staticBodyConfiguration);
            rigidBody = azdynamic_cast<AzPhysics::StaticRigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, rigidBodyHandle));
        }

        // Validate the cached mesh is there
        EXPECT_NE(shapeConfig.GetCachedNativeMesh(), nullptr);

        // Make some changes in the configuration for the second shape
        colliderConfig.m_position.SetX(4.0f);
        shapeConfig.m_scale = AZ::Vector3(2.0f, 2.0f, 2.0f);

        // Create the second shape
        AZStd::shared_ptr<Physics::Shape> secondShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        AZ_Assert(secondShape != nullptr, "Failed to create a shape from cooked data");
        
        rigidBody->AddShape(secondShape);

        // Drop a sphere
        auto sphereActor = TestUtils::AddUnitTestObject<SphereColliderComponent>(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 8.0f), "TestSphere01");
        AzPhysics::RigidBody* sphereRigidBody = sphereActor->FindComponent<RigidBodyComponent>()->GetRigidBody();

        // Tick the world
        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 120);

        // Verify the sphere is lying on top of the mesh
        AZ::Vector3 spherePosition = sphereRigidBody->GetPosition();
        EXPECT_NEAR(spherePosition.GetZ(), 6.5f, 0.01f);

        // Clean up
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_testSceneHandle, rigidBodyHandle);
        }
        rigidBody = nullptr;
    }

    TEST_F(PhysXSpecificTest, Shape_ConstructorDestructor_PxShapeReferenceCounterIsCorrect)
    {
        // Create physx::PxShape object
        AzPhysics::CollisionGroup assignedCollisionGroup = AzPhysics::CollisionGroup::None;
        physx::PxShape* shape = Utils::CreatePxShapeFromConfig(
            Physics::ColliderConfiguration(), Physics::BoxShapeConfiguration(), assignedCollisionGroup);

        // physx::PxShape object ref count is expected to be 1 after creation
        EXPECT_EQ(shape->getReferenceCount(), 1);

        // Create PhysX::Shape wrapper object and verify physx::PxShape ref count is increased to 2
        AZStd::unique_ptr<Shape> shapeWrapper = AZStd::make_unique<Shape>(shape);
        EXPECT_EQ(shape->getReferenceCount(), 2);

        // Destroy PhysX::Shape wrapper object and verify physx::PxShape ref count is back to 1
        shapeWrapper = nullptr;
        EXPECT_EQ(shape->getReferenceCount(), 1);

        // Clean up
        shape->release();
    }
    
    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateWithInvalidHeight_ReturnsEmpty)
    {
        // Given a frustum with an invalid height
        float invalidHeight = 0.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions; 

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(invalidHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // The frustum creation will be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateWithInvalidBottomRadius_ReturnsEmpty)
    {
        // Given a frustum with an invalid bottom radius
        float validHeight = 1.0f;
        float invalidBottomRadius = -1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, invalidBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateFromInvalidTopRadius_ReturnsEmpty)
    {
        // Given a frustum with an invalid top radius
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float invalidTopRadius = -1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, invalidTopRadius, validSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }


    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateFromInvalidBottomAndTopRadius_ReturnsEmpty)
    {
        // Given a frustum with an invalid bottom and top radius
        float validHeight = 1.0f;
        float invalidBottomRadius = 0.0f; 
        float invalidTopRadius = 0.0f;    
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, invalidBottomRadius, invalidTopRadius, validSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateFromInvalidMinSubdivisions_ReturnsEmpty)
    {
        // Given a frustum with an invalid minimum subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 invalidMinSubdivisions = Utils::MinFrustumSubdivisions - 1;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, invalidMinSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateFromInvalidMaxSubdivisions_ReturnsEmpty)
    {
        // Given a frustum with an invalid maximum subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 invalidMaxSubdivisions = Utils::MaxFrustumSubdivisions + 1;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, invalidMaxSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create3SidedFrustum_ReturnsPoints)
    {
        // Given a valid unit frustum with MinSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create3SidedBottomCone_ReturnsPoints)
    {
        // Given a valid unit frustum with MinSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 0.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create3SidedTopCone_ReturnsPoints)
    {
        // Given a valid unit frustum with MinSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 0.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create125SidedFrustum_ReturnsPoints)
    {
        // Given a valid unit frustum with MaxSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MaxFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create125SidedBottomCone_ReturnsPoints)
    {
        // Given a valid unit frustum with MaxSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 0.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MaxFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }
    
    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create125SidedTopCone_ReturnsPoints)
    {
        // Given a valid unit frustum with MaxSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 0.0f;
        AZ::u8 validSubdivisions = Utils::MaxFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, RigidBody_RigidBodyWithAxisLockFlagsCreated_InternalPhysXFlagsSetAccordingly)
    {
        // Helper function wrapping creation logic
        auto CreateRigidBody = [this](bool linearX, bool linearY, bool linearZ, bool angularX, bool angularY, bool angularZ) -> AzPhysics::RigidBody*
        {
            AzPhysics::RigidBodyConfiguration rigidBodyConfig;

            rigidBodyConfig.m_lockLinearX = linearX;
            rigidBodyConfig.m_lockLinearY = linearY;
            rigidBodyConfig.m_lockLinearZ = linearZ;

            rigidBodyConfig.m_lockAngularX = angularX;
            rigidBodyConfig.m_lockAngularY = angularY;
            rigidBodyConfig.m_lockAngularZ = angularZ;

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfig);
                return azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
            }

            return nullptr;
        };

        auto RemoveRigidBody = [](AzPhysics::RigidBody*& rigidBody)
        {
            auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
            if (rigidBody && sceneInterface)
            {
                sceneInterface->RemoveSimulatedBody(rigidBody->m_sceneOwner, rigidBody->m_bodyHandle);
            }
            rigidBody = nullptr;
        };

        auto TestLockFlags = [&CreateRigidBody, &RemoveRigidBody](bool linearX, bool linearY, bool linearZ,
                                                                  bool angularX, bool angularY, bool angularZ,
                                                                  physx::PxRigidDynamicLockFlags expectedFlags)
        {
            auto* rigidBody = CreateRigidBody(linearX, linearY, linearZ, angularX, angularY, angularZ);
            ASSERT_TRUE(rigidBody != nullptr);

            physx::PxRigidDynamic* pxRigidBody = static_cast<physx::PxRigidDynamic*>(rigidBody->GetNativePointer());

            // These values need to be cast to integral types to prevent a compilation error on somme platforms.
            EXPECT_EQ(static_cast<AZ::u32>(pxRigidBody->getRigidDynamicLockFlags()), static_cast<AZ::u32>((expectedFlags)));

            RemoveRigidBody(rigidBody);
        };

        TestLockFlags(false, false, false, false, false, false, physx::PxRigidDynamicLockFlags(0));
        TestLockFlags(true, false, false, false, false, false, physx::PxRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X));
        TestLockFlags(false, false, false, false, true, false, physx::PxRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y));
        TestLockFlags(false, true, false, false, false, true,
            physx::PxRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z));
    }

    TEST_F(PhysXSpecificTest, RigidBody_RigidBodyWithSimulatedFlagsHitsPlane_OnlySimulatedShapeCollidesWithPlane)
    {
        // Helper function wrapping creation logic
        auto CreateBoxRigidBody = [this](const AZ::Vector3& position, bool simulatedFlag, bool triggerFlag) -> AzPhysics::RigidBody*
        {

            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_isSimulated = simulatedFlag;
            colliderConfig->m_isTrigger = triggerFlag;

            AzPhysics::RigidBodyConfiguration rigidBodyConfig;
            rigidBodyConfig.m_entityId = AZ::EntityId(0); // Set entity ID to avoid warnings in OnTriggerEnter 
            rigidBodyConfig.m_position = position;
            rigidBodyConfig.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(
                colliderConfig, AZStd::make_shared<Physics::BoxShapeConfiguration>());

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfig);
                return azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
            }
            return nullptr;
        };

        // Create a box with m_isSimulated = false
        AzPhysics::RigidBody* rigidBodyNonSim =
            CreateBoxRigidBody(AZ::Vector3(-5.0f, 0.0f, 5.0f), false, false);

        AzPhysics::RigidBody* rigidBodySolid =
            CreateBoxRigidBody(AZ::Vector3(5.0f, 0.0f, 5.0f), true, false);

        AzPhysics::RigidBody* rigidBodyTrigger =
            CreateBoxRigidBody(AZ::Vector3(0.0f, 0.0f, 5.0f), true, true);

        // Create ground at origin
        auto ground = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(20.0f, 20.0f, 0.5f));

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 60);

        // Solid rigid body is above the ground
        EXPECT_GT(rigidBodySolid->GetPosition().GetZ(), 0.5f);

        // Non sim rigid body fell through the ground
        EXPECT_LT(rigidBodyNonSim->GetPosition().GetZ(), 0.5f);

        // Trigger rigid body fell through the ground
        EXPECT_LT(rigidBodyTrigger->GetPosition().GetZ(), 0.5f);
    }

    // Fixture for testing combinations of densities on multiple shapes
    class MultiShapesDensityTestFixture
        : public ::testing::TestWithParam<AZStd::pair<float, float>>
    {
    public:
        void SetUp() override
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
                sceneConfiguration.m_sceneName = AzPhysics::DefaultPhysicsSceneName;
                m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            }
        }

        void TearDown() override
        {
            if (auto* materialManager = AZ::Interface<Physics::MaterialManager>::Get())
            {
                materialManager->DeleteAllMaterials();
            }

            //Clean up the Test scene
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveScene(m_testSceneHandle);
            }
            m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        }

        AzPhysics::SceneHandle m_testSceneHandle = AzPhysics::InvalidSceneHandle;
    };

    TEST_P(MultiShapesDensityTestFixture, RigidBody_CreateShapesWithDifferentDensity_ResultingMassMatchesExpected)
    {
        Physics::System* physics = AZ::Interface<Physics::System>::Get();
        AzPhysics::RigidBodyConfiguration rigidBodyConfig;

        AzPhysics::RigidBody* rigidBody = nullptr;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfig);
            rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
        }

        const PhysX::MaterialConfiguration defaultMaterialConfiguration;
        AZ::Data::Asset<Physics::MaterialAsset> defaultMaterialAsset = defaultMaterialConfiguration.CreateMaterialAsset();

        // Create materials for each density
        AZStd::shared_ptr<PhysX::Material> boxMaterial = PhysX::Material::CreateMaterialWithRandomId(defaultMaterialAsset);
        boxMaterial->SetDensity(AZStd::get<0>(GetParam()));

        AZStd::shared_ptr<PhysX::Material> sphereMaterial = PhysX::Material::CreateMaterialWithRandomId(defaultMaterialAsset);
        sphereMaterial->SetDensity(AZStd::get<1>(GetParam()));

        // Create the shapes with their corresponding materials
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_position = AZ::Vector3(1.0f, 0.0f, 0.0f);
        Physics::BoxShapeConfiguration boxShapeConfig;
        AZStd::shared_ptr<Physics::Shape> boxShape =
            physics->CreateShape(colliderConfig, boxShapeConfig);
        boxShape->SetMaterial(boxMaterial);
        rigidBody->AddShape(boxShape);

        colliderConfig.m_position = AZ::Vector3(-1.0f, 0.0f, 0.0f);
        Physics::SphereShapeConfiguration sphereShapeConfig;
        AZStd::shared_ptr<Physics::Shape> sphereShape =
            physics->CreateShape(colliderConfig, sphereShapeConfig);
        sphereShape->SetMaterial(sphereMaterial);
        rigidBody->AddShape(sphereShape);

        // Do mass properties calculation
        rigidBody->UpdateMassProperties();

        // Verify the calculated mass matches the expected
        const float mass = rigidBody->GetMass();

        const float expectedMass = boxMaterial->GetDensity() * GetShapeVolume(boxShapeConfig) +
            sphereMaterial->GetDensity() * GetShapeVolume(sphereShapeConfig);

        EXPECT_TRUE(AZ::IsCloseMag(expectedMass, mass, AZ::Constants::Tolerance));
    }

    // Valid material density values: [0.01f, 1e5f]
    INSTANTIATE_TEST_CASE_P(PhysX, MultiShapesDensityTestFixture,
        ::testing::Values(
            AZStd::make_pair(0.01f, 0.01f),
            AZStd::make_pair(1e5f, 1e5f),
            AZStd::make_pair(0.01f, 1e5f), 
            AZStd::make_pair(2364.0f, 10.0f)
        ));

    // Fixture for testing extreme density values
    class DensityBoundariesTestFixture
        : public ::testing::TestWithParam<float>
    {
    public:
        void TearDown() override
        {
            if (auto* materialManager = AZ::Interface<Physics::MaterialManager>::Get())
            {
                materialManager->DeleteAllMaterials();
            }
        }
    };

    TEST_P(DensityBoundariesTestFixture, Material_ExtremeDensityValues_ResultingDensityClampedToValidRange)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_density = GetParam();

        AZ::Data::Asset<Physics::MaterialAsset> materialAsset = materialConfiguration.CreateMaterialAsset();

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::CreateMaterialWithRandomId(materialAsset);

        // Resulting density should be in the valid range
        float resultingDensity = material->GetDensity();
        EXPECT_TRUE(resultingDensity >= PhysX::MaterialConstants::MinDensityLimit
            && resultingDensity <= PhysX::MaterialConstants::MaxDensityLimit);
    }

    TEST_P(DensityBoundariesTestFixture, MaterialInstance_ExtremeDensityValues_ResultingDensityClampedToValidRange)
    {
        const PhysX::MaterialConfiguration defaultMaterialConfiguration;
        AZ::Data::Asset<Physics::MaterialAsset> defaultMaterialAsset = defaultMaterialConfiguration.CreateMaterialAsset();

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::CreateMaterialWithRandomId(defaultMaterialAsset);
        material->SetDensity(GetParam());

        // Resulting density should be in the valid range
        float resultingDensity = material->GetDensity();
        EXPECT_TRUE(resultingDensity >= PhysX::MaterialConstants::MinDensityLimit
            && resultingDensity <= PhysX::MaterialConstants::MaxDensityLimit);
    }

    // Valid material density values: [0.01f, 1e5f]
    INSTANTIATE_TEST_CASE_P(PhysX, DensityBoundariesTestFixture,
        ::testing::Values(
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max(),
            0.0f,
            1.0f,
            1e9f,
            0.01f,
            1e5f
            ));

    enum class SimulatedShapesMode
    {
        NONE,
        MIXED,
        ALL
    };

    class MassComputeFixture
        : public ::testing::TestWithParam<::testing::tuple<Physics::ShapeType, SimulatedShapesMode, AzPhysics::MassComputeFlags, bool, bool>>
    {
    public:
        void SetUp() override final
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
                sceneConfiguration.m_sceneName = AzPhysics::DefaultPhysicsSceneName;
                m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            }

            AzPhysics::MassComputeFlags massComputeFlags = GetMassComputeFlags();
            m_rigidBodyConfig.SetMassComputeFlags(massComputeFlags);

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &m_rigidBodyConfig);
                m_rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle));
            }

            ASSERT_TRUE(m_rigidBody != nullptr);
        }

        void TearDown() override final
        {
            //Clean up the Test scene
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveScene(m_testSceneHandle);
            }
            m_testSceneHandle = AzPhysics::InvalidSceneHandle;
            m_rigidBodyConfig = AzPhysics::RigidBodyConfiguration();
            m_rigidBody = nullptr;
        }

        Physics::ShapeType GetShapeType() const
        {
            return ::testing::get<0>(GetParam());
        }

        SimulatedShapesMode GetShapesMode() const
        {
            return ::testing::get<1>(GetParam());
        }

        AzPhysics::MassComputeFlags GetMassComputeFlags() const
        {
            const AzPhysics::MassComputeFlags massComputeFlags = ::testing::get<2>(GetParam());
            if (IncludeAllShapes())
            {
                return massComputeFlags | AzPhysics::MassComputeFlags::INCLUDE_ALL_SHAPES;
            }
            else
            {
                return massComputeFlags;
            }
        }

        bool IncludeAllShapes() const
        {
            return ::testing::get<3>(GetParam());
        }

        bool IsMultiShapeTest() const
        {
            return ::testing::get<4>(GetParam());
        }

        bool IsMassExpectedToChange() const
        {
            return m_rigidBodyConfig.m_computeMass &&
                (GetShapesMode() != SimulatedShapesMode::NONE || m_rigidBodyConfig.m_includeAllShapesInMassCalculation);
        }

        bool IsComExpectedToChange() const
        {
            return m_rigidBodyConfig.m_computeCenterOfMass &&
                (GetShapesMode() != SimulatedShapesMode::NONE || m_rigidBodyConfig.m_includeAllShapesInMassCalculation);
        }

        bool IsInertiaExpectedToChange() const
        {
            return m_rigidBodyConfig.m_computeInertiaTensor &&
                (GetShapesMode() != SimulatedShapesMode::NONE || m_rigidBodyConfig.m_includeAllShapesInMassCalculation);
        }

        AZStd::shared_ptr<Physics::Shape> CreateShape(const Physics::ColliderConfiguration& colliderConfiguration, Physics::ShapeType shapeType)
        {
            AZStd::shared_ptr<Physics::Shape> shape;
            Physics::System* physics = AZ::Interface<Physics::System>::Get();
            switch (shapeType)
            {
            case Physics::ShapeType::Sphere:
                shape = physics->CreateShape(colliderConfiguration, Physics::SphereShapeConfiguration());
                break;
            case Physics::ShapeType::Box:
                shape = physics->CreateShape(colliderConfiguration, Physics::BoxShapeConfiguration());
                break;
            case Physics::ShapeType::Capsule:
                shape = physics->CreateShape(colliderConfiguration, Physics::CapsuleShapeConfiguration());
                break;
            }
            return shape;
        };

        AzPhysics::RigidBodyConfiguration m_rigidBodyConfig;
        AzPhysics::RigidBody* m_rigidBody = nullptr;
        AzPhysics::SceneHandle m_testSceneHandle = AzPhysics::InvalidSceneHandle;
    };

    TEST_P(MassComputeFixture, RigidBody_ComputeMassFlagsCombinationsTwoShapes_MassPropertiesCalculatedAccordingly)
    {
        const Physics::ShapeType shapeType = GetShapeType();
        const SimulatedShapesMode shapeMode = GetShapesMode();
        const AzPhysics::MassComputeFlags massComputeFlags = GetMassComputeFlags();
        const bool multiShapeTest = IsMultiShapeTest();

        // Save initial values
        const AZ::Vector3 comBefore = m_rigidBody->GetCenterOfMassWorld();
        const AZ::Matrix3x3 inertiaBefore = m_rigidBody->GetInertiaWorld();
        const float massBefore = m_rigidBody->GetMass();

        // Shape will be simulated for ALL and MIXED shape modes
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_isSimulated =
            (shapeMode == SimulatedShapesMode::ALL || shapeMode == SimulatedShapesMode::MIXED);
        colliderConfig.m_position = AZ::Vector3(1.0f, 0.0f, 0.0f);

        AZStd::shared_ptr<Physics::Shape> shape = CreateShape(colliderConfig, shapeType);
        m_rigidBody->AddShape(shape);

        if (multiShapeTest)
        {
            // Sphere shape will be simulated only for the ALL shape mode
            Physics::ColliderConfiguration sphereColliderConfig;
            sphereColliderConfig.m_isSimulated = (shapeMode == SimulatedShapesMode::ALL);
            sphereColliderConfig.m_position = AZ::Vector3(-2.0f, 0.0f, 0.0f);
            AZStd::shared_ptr<Physics::Shape> sphereShape = CreateShape(sphereColliderConfig, Physics::ShapeType::Sphere);
            m_rigidBody->AddShape(sphereShape);
        }

        // Verify swapping materials results in changes in the mass.
        m_rigidBody->UpdateMassProperties(massComputeFlags, m_rigidBodyConfig.m_centerOfMassOffset,
            m_rigidBodyConfig.m_inertiaTensor, m_rigidBodyConfig.m_mass);

        const float massAfter = m_rigidBody->GetMass();
        const AZ::Vector3 comAfter = m_rigidBody->GetCenterOfMassWorld();
        const AZ::Matrix3x3 inertiaAfter = m_rigidBody->GetInertiaWorld();

        using ::testing::Not;
        using ::testing::FloatNear;
        using ::UnitTest::IsClose;
        if (IsMassExpectedToChange())
        {
            EXPECT_THAT(massBefore, Not(FloatNear(massAfter, FLT_EPSILON)));
        }
        else
        {
            EXPECT_THAT(massBefore, FloatNear(massAfter, FLT_EPSILON));
        }

        if (IsComExpectedToChange())
        {
            EXPECT_THAT(comBefore, Not(IsClose(comAfter)));
        }
        else
        {
            EXPECT_THAT(comBefore, IsClose(comAfter));
        }

        if (IsInertiaExpectedToChange())
        {
            EXPECT_THAT(inertiaBefore, Not(IsClose(inertiaAfter)));
        }
        else
        {
            EXPECT_THAT(inertiaBefore, IsClose(inertiaAfter));
        }
    }

    static const AzPhysics::MassComputeFlags PossibleMassComputeFlags[] =
    {
        // No compute
        AzPhysics::MassComputeFlags::NONE,

        // Compute Mass only
        AzPhysics::MassComputeFlags::COMPUTE_MASS,

        // Compute Inertia only
        AzPhysics::MassComputeFlags::COMPUTE_INERTIA,

        // Compute COM only
        AzPhysics::MassComputeFlags::COMPUTE_COM,

        // Compute combinations of 2
        AzPhysics::MassComputeFlags::COMPUTE_MASS | AzPhysics::MassComputeFlags::COMPUTE_COM,
        AzPhysics::MassComputeFlags::COMPUTE_MASS | AzPhysics::MassComputeFlags::COMPUTE_INERTIA,
        AzPhysics::MassComputeFlags::COMPUTE_COM | AzPhysics::MassComputeFlags::COMPUTE_INERTIA,

        // Compute all
        AzPhysics::MassComputeFlags::DEFAULT, // COMPUTE_COM | COMPUTE_INERTIA | COMPUTE_MASS
    };

    INSTANTIATE_TEST_CASE_P(PhysX, MassComputeFixture, ::testing::Combine(
        ::testing::ValuesIn({ Physics::ShapeType::Sphere, Physics::ShapeType::Box, Physics::ShapeType::Capsule }), // Values for GetShapeType() const
        ::testing::ValuesIn({ SimulatedShapesMode::NONE, SimulatedShapesMode::MIXED, SimulatedShapesMode::ALL }), // Values for GetShapesMode()
        ::testing::ValuesIn(PossibleMassComputeFlags), // Values for GetMassComputeFlags()
        ::testing::Bool(), // Values for IncludeAllShapes()
        ::testing::Bool())); // Values for IsMultiShapeTest()

    class MassPropertiesWithTriangleMesh
        : public ::testing::TestWithParam<AzPhysics::MassComputeFlags>
    {
    public:
        void SetUp() override
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
                sceneConfiguration.m_sceneName = AzPhysics::DefaultPhysicsSceneName;
                m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            }
        }

        void TearDown() override
        {
            // Clean up the Test scene
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveScene(m_testSceneHandle);
            }
            m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        }

        AzPhysics::MassComputeFlags GetMassComputeFlags() const
        {
            return GetParam();
        }

        AzPhysics::SceneHandle m_testSceneHandle = AzPhysics::InvalidSceneHandle;
    };

    TEST_P(MassPropertiesWithTriangleMesh, KinematicRigidBody_ComputeMassProperties_TriggersWarnings)
    {
        const AzPhysics::MassComputeFlags flags = GetMassComputeFlags();

        const bool doesComputeCenterOfMass = AzPhysics::MassComputeFlags::COMPUTE_COM == (flags & AzPhysics::MassComputeFlags::COMPUTE_COM);
        const bool doesComputeMass = AzPhysics::MassComputeFlags::COMPUTE_MASS == (flags & AzPhysics::MassComputeFlags::COMPUTE_MASS);
        const bool doesComputeInertia = AzPhysics::MassComputeFlags::COMPUTE_INERTIA == (flags & AzPhysics::MassComputeFlags::COMPUTE_INERTIA);

        UnitTest::ErrorHandler computeCenterOfMassWarningHandler(
            "cannot compute COM");
        UnitTest::ErrorHandler computeMassWarningHandler(
            "cannot compute Mass");
        UnitTest::ErrorHandler computeIneriaWarningHandler(
            "cannot compute Inertia");

        AzPhysics::SimulatedBodyHandle rigidBodyhandle = TestUtils::AddKinematicTriangleMeshCubeToScene(m_testSceneHandle, 3.0f, flags);

        EXPECT_TRUE(rigidBodyhandle != AzPhysics::InvalidSimulatedBodyHandle);
        EXPECT_EQ(computeCenterOfMassWarningHandler.GetExpectedWarningCount(), doesComputeCenterOfMass ? 1 : 0);
        EXPECT_EQ(computeMassWarningHandler.GetExpectedWarningCount(), doesComputeMass ? 1 : 0);
        EXPECT_EQ(computeIneriaWarningHandler.GetExpectedWarningCount(), doesComputeInertia ? 1 : 0);

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_testSceneHandle, rigidBodyhandle);
        }
    }

    INSTANTIATE_TEST_CASE_P(PhysX, MassPropertiesWithTriangleMesh,
        ::testing::ValuesIn(PossibleMassComputeFlags)); // Values for GetMassComputeFlags()

    TEST_F(PhysXSpecificTest, RigidBodyWithBoxGeometryCanSwitchFromKinematicToDynamic)
    {
        const AZ::Vector3 position = AZ::Vector3::CreateZero();
        const AZ::Vector3 dimensions = AZ::Vector3::CreateOne();
        EntityPtr entity = TestUtils::CreateBoxEntity(m_testSceneHandle, position, dimensions);
        Physics::RigidBodyRequestBus::Event(entity->GetId(), &Physics::RigidBodyRequestBus::Events::SetKinematic, true);
        bool isKinematic = false;
        Physics::RigidBodyRequestBus::EventResult(isKinematic, entity->GetId(), &Physics::RigidBodyRequestBus::Events::IsKinematic);
        EXPECT_TRUE(isKinematic);
        Physics::RigidBodyRequestBus::Event(entity->GetId(), &Physics::RigidBodyRequestBus::Events::SetKinematic, false);
        UnitTest::ErrorHandler setKinematicFalseWarningHandler("Cannot set kinematic to false");
        Physics::RigidBodyRequestBus::EventResult(isKinematic, entity->GetId(), &Physics::RigidBodyRequestBus::Events::IsKinematic);
        EXPECT_EQ(setKinematicFalseWarningHandler.GetWarningCount(), 0);
        EXPECT_FALSE(isKinematic);
    }

    TEST_F(PhysXSpecificTest, RigidBodyWithSphereGeometryCanSwitchFromKinematicToDynamic)
    {
        const AZ::Vector3 position = AZ::Vector3::CreateZero();
        const float radius = 1.0f;
        EntityPtr entity = TestUtils::CreateSphereEntity(m_testSceneHandle, position, radius);
        Physics::RigidBodyRequestBus::Event(entity->GetId(), &Physics::RigidBodyRequestBus::Events::SetKinematic, true);
        bool isKinematic = false;
        Physics::RigidBodyRequestBus::EventResult(isKinematic, entity->GetId(), &Physics::RigidBodyRequestBus::Events::IsKinematic);
        EXPECT_TRUE(isKinematic);
        Physics::RigidBodyRequestBus::Event(entity->GetId(), &Physics::RigidBodyRequestBus::Events::SetKinematic, false);
        UnitTest::ErrorHandler setKinematicFalseWarningHandler("Cannot set kinematic to false");
        Physics::RigidBodyRequestBus::EventResult(isKinematic, entity->GetId(), &Physics::RigidBodyRequestBus::Events::IsKinematic);
        EXPECT_EQ(setKinematicFalseWarningHandler.GetWarningCount(), 0);
        EXPECT_FALSE(isKinematic);
    }

    TEST_F(PhysXSpecificTest, RigidBodyWithCapsuleGeometryCanSwitchFromKinematicToDynamic)
    {
        const AZ::Vector3 position = AZ::Vector3::CreateZero();
        const float radius = 0.5f;
        const float height = 2.0f;
        EntityPtr entity = TestUtils::CreateCapsuleEntity(m_testSceneHandle, position, height, radius);
        Physics::RigidBodyRequestBus::Event(entity->GetId(), &Physics::RigidBodyRequestBus::Events::SetKinematic, true);
        bool isKinematic = false;
        Physics::RigidBodyRequestBus::EventResult(isKinematic, entity->GetId(), &Physics::RigidBodyRequestBus::Events::IsKinematic);
        EXPECT_TRUE(isKinematic);
        Physics::RigidBodyRequestBus::Event(entity->GetId(), &Physics::RigidBodyRequestBus::Events::SetKinematic, false);
        UnitTest::ErrorHandler setKinematicFalseWarningHandler("Cannot set kinematic to false");
        Physics::RigidBodyRequestBus::EventResult(isKinematic, entity->GetId(), &Physics::RigidBodyRequestBus::Events::IsKinematic);
        EXPECT_EQ(setKinematicFalseWarningHandler.GetWarningCount(), 0);
        EXPECT_FALSE(isKinematic);
    }

    TEST_F(PhysXSpecificTest, RigidBodyWithConvexMeshGeometryCanSwitchFromKinematicToDynamic)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::RigidBodyConfiguration rigidBodyConfiguration;
        rigidBodyConfiguration.m_kinematic = true;

        AzPhysics::SimulatedBodyHandle rigidBodyhandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidBodyConfiguration);
        auto convexShape = PhysX::TestUtils::CreatePyramidShape(1.0f);
        auto* rigidBody =
            azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, rigidBodyhandle));
        rigidBody->AddShape(convexShape);
        EXPECT_TRUE(rigidBody->IsKinematic());
        UnitTest::ErrorHandler setKinematicFalseWarningHandler("Cannot set kinematic to false");
        rigidBody->SetKinematic(false);
        EXPECT_EQ(setKinematicFalseWarningHandler.GetWarningCount(), 0);
        EXPECT_FALSE(rigidBody->IsKinematic());
    }

    TEST_F(PhysXSpecificTest, RigidBodyWithTriangleMeshGeometryCannotSwitchFromKinematicToDynamic)
    {
        AzPhysics::SimulatedBodyHandle rigidBodyhandle =
            TestUtils::AddKinematicTriangleMeshCubeToScene(m_testSceneHandle, 3.0f, AzPhysics::MassComputeFlags::NONE);
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        auto* rigidBody =
            azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, rigidBodyhandle));
        EXPECT_TRUE(rigidBody->IsKinematic());
        UnitTest::ErrorHandler setKinematicFalseWarningHandler("Cannot set kinematic to false");
        rigidBody->SetKinematic(false);
        EXPECT_EQ(setKinematicFalseWarningHandler.GetWarningCount(), 1);
        EXPECT_TRUE(rigidBody->IsKinematic());
    }
} // namespace PhysX

