/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Configuration/SystemConfiguration.h>
#include <AZTestShared/Utils/Utils.h>

#include <Tests/PhysXGenericTestFixture.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysX
{
    class PhysicsComponentBusTest
        : public GenericPhysicsInterfaceTest
    {
    };

    TEST_F(PhysicsComponentBusTest, SetLinearDamping_DynamicSphere_MoreDampedBodyFallsSlower)
    {
        EntityPtr sphereA = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        EntityPtr sphereB = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.1f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.2f);

        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 60);

        float dampingA, dampingB;
        Physics::RigidBodyRequestBus::EventResult(dampingA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);
        Physics::RigidBodyRequestBus::EventResult(dampingB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);
        EXPECT_NEAR(dampingA, 0.1f, 1e-3f);
        EXPECT_NEAR(dampingB, 0.2f, 1e-3f);

        float zA = TestUtils::GetPositionElement(sphereA, 2);
        float zB = TestUtils::GetPositionElement(sphereB, 2);
        EXPECT_GT(zB, zA);

        AZ::Vector3 vA = AZ::Vector3::CreateZero();
        AZ::Vector3 vB = AZ::Vector3::CreateZero();
        Physics::RigidBodyRequestBus::EventResult(vA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetLinearVelocity);
        Physics::RigidBodyRequestBus::EventResult(vB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetLinearVelocity);
        EXPECT_GT(static_cast<float>(vA.GetLength()), static_cast<float>(vB.GetLength()));
    }

    TEST_F(PhysicsComponentBusTest, SetLinearDampingNegative_DynamicSphere_NegativeValueRejected)
    {
        UnitTest::ErrorHandler errorHandler("Negative linear damping value");

        EntityPtr sphere = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), 0.5f);

        float damping = 0.0f, initialDamping = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(initialDamping, sphere->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);

        // a negative damping value should be rejected and the damping should remain at its previous value
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, -0.1f);
        Physics::RigidBodyRequestBus::EventResult(damping, sphere->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);

        EXPECT_NEAR(damping, initialDamping, 1e-3f);
        EXPECT_TRUE(errorHandler.GetWarningCount() > 0);
    }

    TEST_F(PhysicsComponentBusTest, SetAngularDamping_DynamicSphere_MoreDampedBodyRotatesSlower)
    {
        EntityPtr sphereA = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);
        EntityPtr sphereB = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 5.0f, 1.0f), 0.5f);
        EntityPtr floor = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, 0.1f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, 0.2f);

        float dampingA, dampingB;
        Physics::RigidBodyRequestBus::EventResult(dampingA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetAngularDamping);
        Physics::RigidBodyRequestBus::EventResult(dampingB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetAngularDamping);
        EXPECT_NEAR(dampingA, 0.1f, 1e-3f);
        EXPECT_NEAR(dampingB, 0.2f, 1e-3f);

        AZ::Vector3 impulse(10.0f, 0.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulse, impulse);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulse, impulse);

        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 10);

        auto angularVelocityA = AZ::Vector3::CreateZero();
        auto angularVelocityB = AZ::Vector3::CreateZero();

        for (int timestep = 0; timestep < 10; timestep++)
        {
            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_GT(static_cast<float>(angularVelocityA.GetLength()), static_cast<float>(angularVelocityB.GetLength()));
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);
        }
    }

    TEST_F(PhysicsComponentBusTest, SetAngularDampingNegative_DynamicSphere_NegativeValueRejected)
    {
        UnitTest::ErrorHandler errorHandler("Negative angular damping value");

        EntityPtr sphere = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), 0.5f);

        float damping = 0.0f, initialDamping = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(initialDamping, sphere->GetId(),
            &Physics::RigidBodyRequests::GetAngularDamping);

        // a negative damping value should be rejected and the damping should remain at its previous value
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, -0.1f);
        Physics::RigidBodyRequestBus::EventResult(damping, sphere->GetId(),
            &Physics::RigidBodyRequests::GetAngularDamping);

        EXPECT_NEAR(damping, initialDamping, 1e-3f);
        EXPECT_TRUE(errorHandler.GetWarningCount() > 0);
    }

    TEST_F(PhysicsComponentBusTest, AddImpulse_DynamicSphere_AffectsTrajectory)
    {
        EntityPtr sphereA = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        EntityPtr sphereB = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 impulse(10.0f, 0.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulse, impulse);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = TestUtils::GetPositionElement(sphereA, 0);
            float xPreviousB = TestUtils::GetPositionElement(sphereB, 0);
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 10);
            EXPECT_GT(TestUtils::GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(TestUtils::GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);
        }
    }

    TEST_F(PhysicsComponentBusTest, SetLinearVelocity_DynamicSphere_AffectsTrajectory)
    {
        EntityPtr sphereA = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        EntityPtr sphereB = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 velocity(10.0f, 0.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetLinearVelocity, velocity);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = TestUtils::GetPositionElement(sphereA, 0);
            float xPreviousB = TestUtils::GetPositionElement(sphereB, 0);
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 10);
            EXPECT_GT(TestUtils::GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(TestUtils::GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);
        }
    }

    TEST_F(PhysicsComponentBusTest, AddImpulseAtWorldPoint_DynamicSphere_AffectsTrajectoryAndRotation)
    {
        EntityPtr sphereA = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        EntityPtr sphereB = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 impulse(10.0f, 0.0f, 0.0f);
        AZ::Vector3 worldPoint(0.0f, -5.0f, 0.25f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulseAtWorldPoint, impulse, worldPoint);

        AZ::Vector3 angularVelocityA = AZ::Vector3::CreateZero();
        AZ::Vector3 angularVelocityB = AZ::Vector3::CreateZero();

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = TestUtils::GetPositionElement(sphereA, 0);
            float xPreviousB = TestUtils::GetPositionElement(sphereB, 0);
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 10);
            EXPECT_GT(TestUtils::GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(TestUtils::GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_FALSE(angularVelocityA.IsClose(AZ::Vector3::CreateZero()));
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetX()), 0.0f, 1e-3f);
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetZ()), 0.0f, 1e-3f);
            EXPECT_TRUE(angularVelocityB.IsClose(AZ::Vector3::CreateZero()));
        }
    }

    TEST_F(PhysicsComponentBusTest, AddAngularImpulse_DynamicSphere_AffectsRotation)
    {
        EntityPtr sphereA = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        EntityPtr sphereB = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 angularImpulse(0.0f, 10.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyAngularImpulse, angularImpulse);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = TestUtils::GetPositionElement(sphereA, 0);
            float xPreviousB = TestUtils::GetPositionElement(sphereB, 0);
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 10);
            EXPECT_NEAR(TestUtils::GetPositionElement(sphereA, 0), xPreviousA, 1e-3f);
            EXPECT_NEAR(TestUtils::GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

            AZ::Vector3 angularVelocityA = AZ::Vector3::CreateZero();
            AZ::Vector3 angularVelocityB = AZ::Vector3::CreateZero();
            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_FALSE(angularVelocityA.IsClose(AZ::Vector3::CreateZero()));
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetX()), 0.0f, 1e-3f);
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetZ()), 0.0f, 1e-3f);
            EXPECT_TRUE(angularVelocityB.IsClose(AZ::Vector3::CreateZero()));
        }
    }

    TEST_F(PhysicsComponentBusTest, SetAngularVelocity_DynamicSphere_AffectsRotation)
    {
        EntityPtr sphereA = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        EntityPtr sphereB = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 angularVelocity(0.0f, 10.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetAngularVelocity, angularVelocity);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = TestUtils::GetPositionElement(sphereA, 0);
            float xPreviousB = TestUtils::GetPositionElement(sphereB, 0);
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 10);
            EXPECT_NEAR(TestUtils::GetPositionElement(sphereA, 0), xPreviousA, 1e-3f);
            EXPECT_NEAR(TestUtils::GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

            AZ::Vector3 angularVelocityA = AZ::Vector3::CreateZero();
            AZ::Vector3 angularVelocityB = AZ::Vector3::CreateZero();
            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_FALSE(angularVelocityA.IsClose(AZ::Vector3::CreateZero()));
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetX()), 0.0f, 1e-3f);
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetZ()), 0.0f, 1e-3f);
            EXPECT_TRUE(angularVelocityB.IsClose(AZ::Vector3::CreateZero()));
        }
    }

    TEST_F(PhysicsComponentBusTest, GetLinearVelocity_FallingSphere_VelocityIncreasesOverTime)
    {
        EntityPtr sphere = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f), 0.5f);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.0f);

        float previousSpeed = 0.0f;

        for (int timestep = 0; timestep < 60; timestep++)
        {
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);
            AZ::Vector3 velocity;
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(),
                &Physics::RigidBodyRequests::GetLinearVelocity);
            float speed = velocity.GetLength();
            EXPECT_GT(speed, previousSpeed);
            previousSpeed = speed;
        }
    }

    TEST_F(PhysicsComponentBusTest, SetSleepThreshold_RollingSpheres_LowerThresholdSphereTravelsFurther)
    {
        EntityPtr sphereA = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);
        EntityPtr sphereB = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 5.0f, 1.0f), 0.5f);
        EntityPtr floor = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, 0.75f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, 0.75f);

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetSleepThreshold, 1.0f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetSleepThreshold, 0.5f);

        float sleepThresholdA, sleepThresholdB;
        Physics::RigidBodyRequestBus::EventResult(sleepThresholdA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);
        Physics::RigidBodyRequestBus::EventResult(sleepThresholdB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);

        EXPECT_NEAR(sleepThresholdA, 1.0f, 1e-3f);
        EXPECT_NEAR(sleepThresholdB, 0.5f, 1e-3f);

        AZ::Vector3 impulse(0.0f, 0.1f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyAngularImpulse, impulse);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::ApplyAngularImpulse, impulse);

        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 300);

        EXPECT_GT(TestUtils::GetPositionElement(sphereB, 0), TestUtils::GetPositionElement(sphereA, 0));
    }

    TEST_F(PhysicsComponentBusTest, SetSleepThresholdNegative_DynamicSphere_NegativeValueRejected)
    {
        UnitTest::ErrorHandler errorHandler("Negative sleep threshold value");

        EntityPtr sphere = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);

        float threshold = 0.0f, initialThreshold = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(initialThreshold, sphere->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetSleepThreshold, -0.5f);
        Physics::RigidBodyRequestBus::EventResult(threshold, sphere->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);

        EXPECT_NEAR(threshold, initialThreshold, 1e-3f);
        EXPECT_TRUE(errorHandler.GetWarningCount() > 0);
    }

    TEST_F(PhysicsComponentBusTest, SetMass_Seesaw_TipsDownAtHeavierEnd)
    {
        EntityPtr floor = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));
        EntityPtr pivot = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.7f), AZ::Vector3(0.4f, 1.0f, 0.4f));
        EntityPtr seesaw = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.95f), AZ::Vector3(20.0f, 1.0f, 0.1f));
        EntityPtr boxA = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(-9.0f, 0.0f, 1.5f), AZ::Vector3::CreateOne());
        EntityPtr boxB = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(9.0f, 0.0f, 1.5f), AZ::Vector3::CreateOne());

        Physics::RigidBodyRequestBus::Event(boxA->GetId(), &Physics::RigidBodyRequests::SetMass, 5.0f);
        float mass = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(mass, boxA->GetId(),
            &Physics::RigidBodyRequests::GetMass);
        EXPECT_NEAR(mass, 5.0f, 1e-3f);

        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 30);
        EXPECT_GT(1.5f, TestUtils::GetPositionElement(boxA, 2));
        EXPECT_LT(1.5f, TestUtils::GetPositionElement(boxB, 2));

        Physics::RigidBodyRequestBus::Event(boxB->GetId(), &Physics::RigidBodyRequests::SetMass, 20.0f);
        Physics::RigidBodyRequestBus::EventResult(mass, boxB->GetId(),
            &Physics::RigidBodyRequests::GetMass);
        EXPECT_NEAR(mass, 20.0f, 1e-3f);

        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 60);
        EXPECT_LT(1.5f, TestUtils::GetPositionElement(boxA, 2));
        EXPECT_GT(1.5f, TestUtils::GetPositionElement(boxB, 2));
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Sphere_ValidExtents)
    {
        AZ::Vector3 spherePosition(2.0f, -3.0f, 1.0f);
        EntityPtr sphere = TestUtils::CreateSphereEntity(m_testSceneHandle, spherePosition, 0.5f);

        AZ::Aabb sphereAabb;
        Physics::RigidBodyRequestBus::EventResult(sphereAabb, sphere->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        EXPECT_TRUE(sphereAabb.GetMin().IsClose(spherePosition - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(sphereAabb.GetMax().IsClose(spherePosition + 0.5f * AZ::Vector3::CreateOne()));

        // rotate the sphere and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        AZ::TransformBus::Event(sphere->GetId(), &AZ::TransformInterface::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(quat, spherePosition));
        sphere->Deactivate();
        sphere->Activate();

        Physics::RigidBodyRequestBus::EventResult(sphereAabb, sphere->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        EXPECT_TRUE(sphereAabb.GetMin().IsClose(spherePosition - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(sphereAabb.GetMax().IsClose(spherePosition + 0.5f * AZ::Vector3::CreateOne()));
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Box_ValidExtents)
    {
        AZ::Vector3 boxPosition(2.0f, -3.0f, 1.0f);
        AZ::Vector3 boxDimensions(3.0f, 4.0f, 5.0f);
        EntityPtr box = TestUtils::CreateBoxEntity(m_testSceneHandle, boxPosition, boxDimensions);

        AZ::Aabb boxAabb;
        Physics::RigidBodyRequestBus::EventResult(boxAabb, box->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        EXPECT_TRUE(boxAabb.GetMin().IsClose(boxPosition - 0.5f * boxDimensions));
        EXPECT_TRUE(boxAabb.GetMax().IsClose(boxPosition + 0.5f * boxDimensions));

        // rotate the box and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        AZ::TransformBus::Event(box->GetId(), &AZ::TransformInterface::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(quat, boxPosition));
        box->Deactivate();
        box->Activate();

        Physics::RigidBodyRequestBus::EventResult(boxAabb, box->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        AZ::Vector3 expectedRotatedDimensions(3.5f * sqrtf(2.0f), 3.5f * sqrtf(2.0f), 5.0f);
        EXPECT_TRUE(boxAabb.GetMin().IsClose(boxPosition - 0.5f * expectedRotatedDimensions));
        EXPECT_TRUE(boxAabb.GetMax().IsClose(boxPosition + 0.5f * expectedRotatedDimensions));
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Capsule_ValidExtents)
    {
        AZ::Vector3 capsulePosition(1.0f, -3.0f, 5.0f);
        float capsuleHeight = 2.0f;
        float capsuleRadius = 0.3f;
        EntityPtr capsule = TestUtils::CreateCapsuleEntity(m_testSceneHandle, capsulePosition, capsuleHeight, capsuleRadius);

        AZ::Aabb capsuleAabb;
        Physics::RigidBodyRequestBus::EventResult(capsuleAabb, capsule->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        AZ::Vector3 expectedCapsuleHalfExtents(capsuleRadius, capsuleRadius, 0.5f * capsuleHeight);

        EXPECT_TRUE(capsuleAabb.GetMin().IsClose(capsulePosition - expectedCapsuleHalfExtents));
        EXPECT_TRUE(capsuleAabb.GetMax().IsClose(capsulePosition + expectedCapsuleHalfExtents));

        // rotate the capsule and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationY(0.25f * AZ::Constants::Pi);
        AZ::TransformBus::Event(capsule->GetId(), &AZ::TransformInterface::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(quat, capsulePosition));
        capsule->Deactivate();
        capsule->Activate();

        Physics::RigidBodyRequestBus::EventResult(capsuleAabb, capsule->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        float rotatedHalfHeight = 0.25f * sqrtf(2.0f) * capsuleHeight + (1.0f - 0.5f * sqrt(2.0f)) * capsuleRadius;
        expectedCapsuleHalfExtents = AZ::Vector3(rotatedHalfHeight, capsuleRadius, rotatedHalfHeight);
        EXPECT_TRUE(capsuleAabb.GetMin().IsClose(capsulePosition - expectedCapsuleHalfExtents));
        EXPECT_TRUE(capsuleAabb.GetMax().IsClose(capsulePosition + expectedCapsuleHalfExtents));
    }

    TEST_F(PhysicsComponentBusTest, ForceAwakeForceAsleep_DynamicSphere_SleepStateCorrect)
    {
        EntityPtr floor = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));
        EntityPtr boxA = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(-5.0f, 0.0f, 1.0f), AZ::Vector3::CreateOne());
        EntityPtr boxB = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(5.0f, 0.0f, 100.0f), AZ::Vector3::CreateOne());

        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 60);
        bool isAwakeA = false;
        bool isAwakeB = false;
        Physics::RigidBodyRequestBus::EventResult(isAwakeA, boxA->GetId(),
            &Physics::RigidBodyRequests::IsAwake);
        Physics::RigidBodyRequestBus::EventResult(isAwakeB, boxB->GetId(),
            &Physics::RigidBodyRequests::IsAwake);

        EXPECT_FALSE(isAwakeA);
        EXPECT_TRUE(isAwakeB);

        Physics::RigidBodyRequestBus::Event(boxA->GetId(), &Physics::RigidBodyRequests::ForceAwake);
        Physics::RigidBodyRequestBus::Event(boxB->GetId(), &Physics::RigidBodyRequests::ForceAsleep);

        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);

        Physics::RigidBodyRequestBus::EventResult(isAwakeA, boxA->GetId(),
            &Physics::RigidBodyRequests::IsAwake);
        Physics::RigidBodyRequestBus::EventResult(isAwakeB, boxB->GetId(),
            &Physics::RigidBodyRequests::IsAwake);

        EXPECT_TRUE(isAwakeA);
        EXPECT_FALSE(isAwakeB);

        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 60);

        Physics::RigidBodyRequestBus::EventResult(isAwakeA, boxA->GetId(),
            &Physics::RigidBodyRequests::IsAwake);
        Physics::RigidBodyRequestBus::EventResult(isAwakeB, boxB->GetId(),
            &Physics::RigidBodyRequests::IsAwake);

        EXPECT_FALSE(isAwakeA);
        EXPECT_FALSE(isAwakeB);
    }

    TEST_F(PhysicsComponentBusTest, DisableEnablePhysics_DynamicSphere)
    {
        using namespace AzFramework;

        EntityPtr sphere = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f), 0.5f);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(), &Physics::RigidBodyRequests::SetLinearDamping, 0.0f);

        AZ::Vector3 velocity;
        float previousSpeed = 0.0f;
        for (int timestep = 0; timestep < 30; timestep++)
        {
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(), &Physics::RigidBodyRequests::GetLinearVelocity);
            previousSpeed = velocity.GetLength();
        }

        // Disable physics
        Physics::RigidBodyRequestBus::Event(sphere->GetId(), &Physics::RigidBodyRequests::DisablePhysics);

        // Check speed is not changing
        for (int timestep = 0; timestep < 60; timestep++)
        {
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(), &Physics::RigidBodyRequests::GetLinearVelocity);
            float speed = velocity.GetLength();
            EXPECT_FLOAT_EQ(speed, previousSpeed);
            previousSpeed = speed;
        }

        // Check physics is disabled
        bool physicsEnabled = true;
        Physics::RigidBodyRequestBus::EventResult(physicsEnabled, sphere->GetId(), &Physics::RigidBodyRequests::IsPhysicsEnabled);
        EXPECT_FALSE(physicsEnabled);

        // Enable physics
        Physics::RigidBodyRequestBus::Event(sphere->GetId(), &Physics::RigidBodyRequests::EnablePhysics);

        // Check speed is increasing
        for (int timestep = 0; timestep < 60; timestep++)
        {
            TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(), &Physics::RigidBodyRequests::GetLinearVelocity);
            float speed = velocity.GetLength();
            EXPECT_GT(speed, previousSpeed);
            previousSpeed = speed;
        }
    }

    TEST_F(PhysicsComponentBusTest, Shape_Box_GetAabbIsCorrect)
    {
        Physics::ColliderConfiguration colliderConfig;
        Physics::BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = AZ::Vector3(20.f, 20.f, 20.f);
        AZStd::shared_ptr<Physics::Shape> shape;
        Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
        const AZ::Aabb localAabb = shape->GetAabbLocal();
        EXPECT_TRUE(localAabb.GetMin().IsClose(-shapeConfiguration.m_dimensions / 2.f)
            && localAabb.GetMax().IsClose(shapeConfiguration.m_dimensions / 2.f));

        AZ::Vector3 worldOffset = AZ::Vector3(0, 0, 40.f);
        AZ::Transform worldTransform = AZ::Transform::Identity();
        worldTransform.SetTranslation(worldOffset);
        const AZ::Aabb worldAabb = shape->GetAabb(worldTransform);
        EXPECT_TRUE(worldAabb.GetMin().IsClose((-shapeConfiguration.m_dimensions / 2.f) + worldOffset)
            && worldAabb.GetMax().IsClose((shapeConfiguration.m_dimensions / 2.f) + worldOffset));
    }

    TEST_F(PhysicsComponentBusTest, Shape_Sphere_GetAabbIsCorrect)
    {
        const float radius = 20.f;
        Physics::ColliderConfiguration colliderConfig;
        Physics::SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = radius;
        AZStd::shared_ptr<Physics::Shape> shape;
        Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
        const AZ::Aabb localAabb = shape->GetAabbLocal();
        EXPECT_TRUE(localAabb.GetMin().IsClose(AZ::Vector3(-radius, -radius, -radius))
                    && localAabb.GetMax().IsClose(AZ::Vector3(radius, radius, radius)));

        AZ::Vector3 worldOffset = AZ::Vector3(0, 0, 40.f);
        AZ::Transform worldTransform = AZ::Transform::Identity();
        worldTransform.SetTranslation(worldOffset);
        const AZ::Aabb worldAabb = shape->GetAabb(worldTransform);
        EXPECT_TRUE(worldAabb.GetMin().IsClose(AZ::Vector3(-radius, -radius, -radius) + worldOffset)
                    && worldAabb.GetMax().IsClose(AZ::Vector3(radius, radius, radius) + worldOffset));
    }

    TEST_F(PhysicsComponentBusTest, Shape_Capsule_GetAabbIsCorrect)
    {
        const float radius = 20.f;
        const float height = 80.f;
        Physics::ColliderConfiguration colliderConfig;
        Physics::CapsuleShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = radius;
        shapeConfiguration.m_height = height;
        AZStd::shared_ptr<Physics::Shape> shape;
        Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
        const AZ::Aabb localAabb = shape->GetAabbLocal();
        EXPECT_TRUE(localAabb.GetMin().IsClose(AZ::Vector3(-radius, -radius, -height / 2.f))
            && localAabb.GetMax().IsClose(AZ::Vector3(radius, radius, height / 2.f)));

        AZ::Vector3 worldOffset = AZ::Vector3(0, 0, 40.f);
        AZ::Transform worldTransform = AZ::Transform::Identity();
        worldTransform.SetTranslation(worldOffset);
        const AZ::Aabb worldAabb = shape->GetAabb(worldTransform);
        EXPECT_TRUE(worldAabb.GetMin().IsClose(AZ::Vector3(-radius, -radius, -height / 2.f) + worldOffset)
            && worldAabb.GetMax().IsClose(AZ::Vector3(radius, radius, height / 2.f) + worldOffset));
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyBus_RigidBodyColliders_AABBAreCorrect)
    {
        // Create 3 colliders, one of each type and check that the AABB of their body is the expected
        EntityPtr box = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(0, 0, 0), AZ::Vector3(32, 32, 32));
        AZ::Aabb boxAABB;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(boxAABB, box->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetAabb);
        EXPECT_TRUE(boxAABB.GetMin().IsClose(AZ::Vector3(-16, -16, -16)) && boxAABB.GetMax().IsClose(AZ::Vector3(16, 16, 16)));

        EntityPtr sphere = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(-100, 0, 0), 16);
        AZ::Aabb sphereAABB;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(sphereAABB, sphere->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetAabb);
        EXPECT_TRUE(sphereAABB.GetMin().IsClose(AZ::Vector3(-16 -100, -16, -16)) && sphereAABB.GetMax().IsClose(AZ::Vector3(16 -100, 16, 16)));

        EntityPtr capsule = TestUtils::CreateCapsuleEntity(m_testSceneHandle, AZ::Vector3(100, 0, 0), 128, 16);
        AZ::Aabb capsuleAABB;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(capsuleAABB, capsule->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetAabb);
        EXPECT_TRUE(capsuleAABB.GetMin().IsClose(AZ::Vector3(-16 +100, -16, -64)) && capsuleAABB.GetMax().IsClose(AZ::Vector3(16 +100, 16, 64)));
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyBus_StaticRigidBodyColliders_AABBAreCorrect)
    {
        // Create 3 colliders, one of each type and check that the AABB of their body is the expected
        EntityPtr box = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(0, 0, 0), AZ::Vector3(32, 32, 32));
        AZ::Aabb boxAABB;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(boxAABB, box->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetAabb);
        EXPECT_TRUE(boxAABB.GetMin().IsClose(AZ::Vector3(-16, -16, -16)) && boxAABB.GetMax().IsClose(AZ::Vector3(16, 16, 16)));

        EntityPtr sphere = TestUtils::CreateStaticSphereEntity(m_testSceneHandle, AZ::Vector3(-100, 0, 0), 16);
        AZ::Aabb sphereAABB;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(sphereAABB, sphere->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetAabb);
        EXPECT_TRUE(sphereAABB.GetMin().IsClose(AZ::Vector3(-16 -100, -16, -16)) && sphereAABB.GetMax().IsClose(AZ::Vector3(16 -100, 16, 16)));

        EntityPtr capsule = TestUtils::CreateStaticCapsuleEntity(m_testSceneHandle, AZ::Vector3(100, 0, 0), 128, 16);
        AZ::Aabb capsuleAABB;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(capsuleAABB, capsule->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetAabb);
        EXPECT_TRUE(capsuleAABB.GetMin().IsClose(AZ::Vector3(-16 +100, -16, -64)) && capsuleAABB.GetMax().IsClose(AZ::Vector3(16 +100, 16, 64)));
    }

    using CreateEntityFunc = AZStd::function<EntityPtr(const AZ::Vector3&)>;

    void CheckDisableEnablePhysics(AZStd::vector<CreateEntityFunc> entityCreations, AzPhysics::SceneHandle sceneHandle)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        int i = 0;
        for (CreateEntityFunc& entityCreation : entityCreations)
        {
            AZ::Vector3 entityPos = AZ::Vector3(128.f * i, 0, 0);
            EntityPtr entity = entityCreation(entityPos);

            AzPhysics::RayCastRequest request;
            request.m_start = entityPos + AZ::Vector3(0, 0, 100);
            request.m_direction = AZ::Vector3(0, 0, -1);
            request.m_distance = 200.f;

            AzPhysics::SimulatedBodyComponentRequestsBus::Event(entity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::DisablePhysics);

            bool enabled = true;
            AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(enabled, entity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::IsPhysicsEnabled);
            EXPECT_FALSE(enabled);

            AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(sceneHandle, &request);
            EXPECT_FALSE(result);

            AzPhysics::SimulatedBodyComponentRequestsBus::Event(entity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::EnablePhysics);

            enabled = false;
            AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(enabled, entity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::IsPhysicsEnabled);
            EXPECT_TRUE(enabled);

            result = sceneInterface->QueryScene(sceneHandle, &request);
            EXPECT_TRUE(result);
            EXPECT_TRUE(result.m_hits.size() == 1);
            EXPECT_TRUE(result.m_hits[0].m_entityId == entity->GetId());

            ++i;
        }
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyBus_EnableDisablePhysics_StaticRigidBody)
    {
        AZStd::vector<CreateEntityFunc> entityCreations =
        {
            [this](const AZ::Vector3& position) { return TestUtils::CreateStaticBoxEntity(m_testSceneHandle, position, AZ::Vector3(32, 32, 32)); },
            [this](const AZ::Vector3& position) { return TestUtils::CreateStaticSphereEntity(m_testSceneHandle, position, 16); },
            [this](const AZ::Vector3& position) { return TestUtils::CreateStaticCapsuleEntity(m_testSceneHandle, position, 16, 16); }
        };
        CheckDisableEnablePhysics(entityCreations, m_testSceneHandle);
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyBus_EnableDisablePhysics_RigidBody)
    {
        AZStd::vector<CreateEntityFunc> entityCreations =
        {
            [this](const AZ::Vector3& position) { return TestUtils::CreateBoxEntity(m_testSceneHandle, position, AZ::Vector3(32, 32, 32)); },
            [this](const AZ::Vector3& position) { return TestUtils::CreateSphereEntity(m_testSceneHandle, position, 16); },
            [this](const AZ::Vector3& position) { return TestUtils::CreateCapsuleEntity(m_testSceneHandle, position, 16, 16); }
        };
        CheckDisableEnablePhysics(entityCreations, m_testSceneHandle);
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyRayCast_CastAgainstStaticBox_ReturnsHit)
    {
        EntityPtr staticBoxEntity = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(0.0f), AZ::Vector3(10.f, 10.f, 10.f));

        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        AzPhysics::SceneQueryHit hit;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(hit, staticBoxEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::RayCast, request);

        EXPECT_TRUE(hit);

        bool hitIncludeSphereEntity = (hit.m_entityId == staticBoxEntity->GetId());
        EXPECT_TRUE(hitIncludeSphereEntity);
    }

    using RayCastFunc = AZStd::function<AzPhysics::SceneQueryHit(AZ::EntityId, const AzPhysics::RayCastRequest&)>;
    class PhysicsRigidBodyRayBusTest
        : public GenericPhysicsInterfaceTest
        , public ::testing::WithParamInterface<RayCastFunc>
    {
    };

    TEST_P(PhysicsRigidBodyRayBusTest, ComponentRayCast_CastAgainstNothing_ReturnsNoHit)
    {
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        auto rayCastFunction = GetParam();
        AzPhysics::SceneQueryHit hit = rayCastFunction(AZ::EntityId(), request);

        EXPECT_FALSE(hit);
    }

    TEST_P(PhysicsRigidBodyRayBusTest, ComponentRayCast_CastAgainstSphere_ReturnsHit)
    {
        EntityPtr sphereEntity = TestUtils::CreateSphereEntity(m_testSceneHandle, AZ::Vector3(0.0f), 10.0f);

        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        auto rayCastFunction = GetParam();
        AzPhysics::SceneQueryHit hit = rayCastFunction(sphereEntity->GetId(), request);

        EXPECT_TRUE(hit);

        bool hitsIncludeSphereEntity = (hit.m_entityId == sphereEntity->GetId());
        EXPECT_TRUE(hitsIncludeSphereEntity);
    }

    TEST_P(PhysicsRigidBodyRayBusTest, ComponentRayCast_CastAgainstBoxEntityWithLocalOffset_ReturnsHit)
    {
        const AZ::Vector3 boxExtent = AZ::Vector3(10.0f, 10.0f, 10.0f);
        const AZ::Vector3 box1Offset(0.0f, 0.0f, 30.0f);
        const AZ::Vector3 box2Offset(0.0f, 0.0f, -30.0f);

        MultiShapeConfig config;
        config.m_position = AZ::Vector3(0.0f, 100.0f, 20.0f);
        config.m_shapes.AddBox(boxExtent, box1Offset);
        config.m_shapes.AddBox(boxExtent, box2Offset);

        auto shapeWithTwoBoxes = AddMultiShapeEntity(config);

        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 100.0f, 50.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        auto rayCastFunction = GetParam();
        AzPhysics::SceneQueryHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

        EXPECT_TRUE(result);

        bool hitIncludeEntity = (result.m_entityId == shapeWithTwoBoxes->GetId());
        EXPECT_TRUE(hitIncludeEntity);
    }

    TEST_P(PhysicsRigidBodyRayBusTest, ComponentRayCast_CastAgainstBoxEntityWithMultipleShapesLocalOffset_ReturnsHits)
    {
        // Entity at (0, 100, 20) with two box childs with offsets +30 and -30 in Z.
        // Child boxes world position centers are at (0, 100, 50) and (0, 100, -10)
        // 4 rays tests that should retrieves the correct boxes

        const AZ::Vector3 boxExtent = AZ::Vector3(10.0f, 10.0f, 10.0f);
        const AZ::Vector3 box1Offset(0.0f, 0.0f, 30.0f);
        const AZ::Vector3 box2Offset(0.0f, 0.0f, -30.0f);

        MultiShapeConfig config;
        config.m_position = AZ::Vector3(0.0f, 100.0f, 20.0f);
        config.m_shapes.AddBox(boxExtent, box1Offset);
        config.m_shapes.AddBox(boxExtent, box2Offset);

        AZStd::unique_ptr<AZ::Entity> shapeWithTwoBoxes(AddMultiShapeEntity(config));
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
        PhysX::ColliderComponentRequestBus::EventResult(shapes, shapeWithTwoBoxes->GetId(), &PhysX::ColliderComponentRequests::GetShapes);

        // Upper box part z=50 (-x to +x)
        {
            AzPhysics::RayCastRequest request;
            request.m_start = AZ::Vector3(-100.0f, 100.0f, 50.0f);
            request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
            request.m_distance = 200.0f;

            auto rayCastFunction = GetParam();
            AzPhysics::SceneQueryHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

            EXPECT_TRUE(result);
            bool hitIncludesEntity = (result.m_entityId == shapeWithTwoBoxes->GetId());
            EXPECT_TRUE(hitIncludesEntity);
            bool hitIncludesShape = (result.m_shape == shapes[0].get());
            EXPECT_TRUE(hitIncludesShape);
        }

        // Lower box part z=-10 (-x to +x)
        {
            AzPhysics::RayCastRequest request;
            request.m_start = AZ::Vector3(-100.0f, 100.0f, -10.0f);
            request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
            request.m_distance = 200.0f;

            auto rayCastFunction = GetParam();
            AzPhysics::SceneQueryHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

            EXPECT_TRUE(result);
            bool hitIncludesEntity = (result.m_entityId == shapeWithTwoBoxes->GetId());
            EXPECT_TRUE(hitIncludesEntity);
            bool hitIncludesShape = (result.m_shape == shapes[1].get());
            EXPECT_TRUE(hitIncludesShape);
        }

        // Trace Vertically from top, it should retrieve the upper box shape
        {
            AzPhysics::RayCastRequest request;
            request.m_start = AZ::Vector3(0.0f, 100.0f, 80.0f);
            request.m_direction = AZ::Vector3(0.0f, 0.0f, -1.0f);
            request.m_distance = 200.0f;

            auto rayCastFunction = GetParam();
            AzPhysics::SceneQueryHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

            EXPECT_TRUE(result);
            bool hitIncludesEntity = (result.m_entityId == shapeWithTwoBoxes->GetId());
            EXPECT_TRUE(hitIncludesEntity);
            bool hitIncludesShape = (result.m_shape == shapes[0].get());
            EXPECT_TRUE(hitIncludesShape);
        }

        // Trace Vertically from bottom, it should retrieve the lower box shape
        {
            AzPhysics::RayCastRequest request;
            request.m_start = AZ::Vector3(0.0f, 100.0f, -80.0f);
            request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
            request.m_distance = 200.0f;

            auto rayCastFunction = GetParam();
            AzPhysics::SceneQueryHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

            EXPECT_TRUE(result);
            bool hitIncludesEntity = (result.m_entityId == shapeWithTwoBoxes->GetId());
            EXPECT_TRUE(hitIncludesEntity);
            bool hitIncludesShape = (result.m_shape == shapes[1].get());
            EXPECT_TRUE(hitIncludesShape);
        }
    }

    TEST_P(PhysicsRigidBodyRayBusTest, ComponentRayCast_CastAgainstBoxEntityLocalOffsetAndRotation_ReturnsHits)
    {
        // Entity at (0,0,0) rotated by 90 degrees and child box offset (0,100,0).
        // World position of the child should be (-100, 0, 0).
        // This tests raycasts from (0, 0, 0) to (-200, 0 ,0) checking that collides with the box

        const AZ::Vector3 boxExtent = AZ::Vector3(10.0f, 10.0f, 10.0f);
        const AZ::Vector3 boxOffset(0.0f, 100.0f, 0.0f);

        MultiShapeConfig config;
        config.m_position = AZ::Vector3(0.0f, 0.0f, 0.0f);
        config.m_rotation = AZ::Vector3(0, 0, AZ::Constants::HalfPi);
        config.m_shapes.AddBox(boxExtent, boxOffset);

        AZStd::unique_ptr<AZ::Entity> shapeWithOneBox(AddMultiShapeEntity(config));
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
        PhysX::ColliderComponentRequestBus::EventResult(shapes, shapeWithOneBox->GetId(), &PhysX::ColliderComponentRequests::GetShapes);

        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(-1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        auto rayCastFunction = GetParam();
        AzPhysics::SceneQueryHit result = rayCastFunction(shapeWithOneBox->GetId(), request);

        EXPECT_TRUE(result);
        bool hitIncludesEntity = (result.m_entityId == shapeWithOneBox->GetId());
        EXPECT_TRUE(hitIncludesEntity);
        bool hitIncludesShape = (result.m_shape == shapes[0].get());
        EXPECT_TRUE(hitIncludesShape);
    }

    static const RayCastFunc RigidBodyRaycastEBusCall = []([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AzPhysics::RayCastRequest& request)
    {
        AzPhysics::SceneQueryHit ret;
        Physics::RigidBodyRequestBus::EventResult(ret, entityId, &Physics::RigidBodyRequests::RayCast, request);
        return ret;
    };

    static const RayCastFunc WorldBodyRaycastEBusCall = []([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AzPhysics::RayCastRequest& request)
    {
        AzPhysics::SceneQueryHit ret;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(ret, entityId, &AzPhysics::SimulatedBodyComponentRequests::RayCast, request);
        return ret;
    };

    INSTANTIATE_TEST_CASE_P(, PhysicsRigidBodyRayBusTest,
        ::testing::Values(RigidBodyRaycastEBusCall, WorldBodyRaycastEBusCall),
        // Provide nice names for the tests runs
        [](const testing::TestParamInfo<PhysicsRigidBodyRayBusTest::ParamType>& info)
        {
            const char* name = "";
            switch (info.index)
            {
            case 0:
                name = "RigidBodyRequestBus";
                break;
            case 1:
                name = "WorldBodyRequestBus";
                break;
            }
            return name;
        });


} // namespace Physics
