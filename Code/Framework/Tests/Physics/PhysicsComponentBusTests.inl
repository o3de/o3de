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

namespace Physics
{
    TEST_F(PhysicsComponentBusTest, SetLinearDamping_DynamicSphere_MoreDampedBodyFallsSlower)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.1f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.2f);

        UpdateDefaultWorld(60);

        float dampingA, dampingB;
        Physics::RigidBodyRequestBus::EventResult(dampingA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);
        Physics::RigidBodyRequestBus::EventResult(dampingB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);
        EXPECT_NEAR(dampingA, 0.1f, 1e-3f);
        EXPECT_NEAR(dampingB, 0.2f, 1e-3f);

        float zA = GetPositionElement(sphereA, 2);
        float zB = GetPositionElement(sphereB, 2);
        EXPECT_GT(zB, zA);

        AZ::Vector3 vA = AZ::Vector3::CreateZero();
        AZ::Vector3 vB = AZ::Vector3::CreateZero();
        Physics::RigidBodyRequestBus::EventResult(vA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetLinearVelocity);
        Physics::RigidBodyRequestBus::EventResult(vB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetLinearVelocity);
        EXPECT_GT(static_cast<float>(vA.GetLength()), static_cast<float>(vB.GetLength()));

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetLinearDampingNegative_DynamicSphere_NegativeValueRejected)
    {
        ErrorHandler errorHandler("Negative linear damping value");

        auto sphere = AddSphereEntity(AZ::Vector3::CreateZero(), 0.5f);

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

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, SetAngularDamping_DynamicSphere_MoreDampedBodyRotatesSlower)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 1.0f), 0.5f);
        auto floor = AddStaticBoxEntity(AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));

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

        UpdateDefaultWorld(10);
        auto angularVelocityA = AZ::Vector3::CreateZero();
        auto angularVelocityB = AZ::Vector3::CreateZero();

        for (int timestep = 0; timestep < 10; timestep++)
        {
            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_GT(static_cast<float>(angularVelocityA.GetLength()), static_cast<float>(angularVelocityB.GetLength()));
            UpdateDefaultWorld(1);
        }

        delete floor;
        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetAngularDampingNegative_DynamicSphere_NegativeValueRejected)
    {
        ErrorHandler errorHandler("Negative angular damping value");

        auto sphere = AddSphereEntity(AZ::Vector3::CreateZero(), 0.5f);

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

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, AddImpulse_DynamicSphere_AffectsTrajectory)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 impulse(10.0f, 0.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulse, impulse);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(10);
            EXPECT_GT(GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);
        }

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetLinearVelocity_DynamicSphere_AffectsTrajectory)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 velocity(10.0f, 0.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetLinearVelocity, velocity);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(10);
            EXPECT_GT(GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);
        }

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, AddImpulseAtWorldPoint_DynamicSphere_AffectsTrajectoryAndRotation)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 impulse(10.0f, 0.0f, 0.0f);
        AZ::Vector3 worldPoint(0.0f, -5.0f, 0.25f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulseAtWorldPoint, impulse, worldPoint);

        AZ::Vector3 angularVelocityA = AZ::Vector3::CreateZero();
        AZ::Vector3 angularVelocityB = AZ::Vector3::CreateZero();

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(10);
            EXPECT_GT(GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_FALSE(angularVelocityA.IsClose(AZ::Vector3::CreateZero()));
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetX()), 0.0f, 1e-3f);
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetZ()), 0.0f, 1e-3f);
            EXPECT_TRUE(angularVelocityB.IsClose(AZ::Vector3::CreateZero()));
        }

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, AddAngularImpulse_DynamicSphere_AffectsRotation)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 angularImpulse(0.0f, 10.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyAngularImpulse, angularImpulse);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(10);
            EXPECT_NEAR(GetPositionElement(sphereA, 0), xPreviousA, 1e-3f);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

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

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetAngularVelocity_DynamicSphere_AffectsRotation)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 angularVelocity(0.0f, 10.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetAngularVelocity, angularVelocity);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(10);
            EXPECT_NEAR(GetPositionElement(sphereA, 0), xPreviousA, 1e-3f);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

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

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, GetLinearVelocity_FallingSphere_VelocityIncreasesOverTime)
    {
        auto sphere = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 0.0f), 0.5f);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.0f);

        float previousSpeed = 0.0f;

        for (int timestep = 0; timestep < 60; timestep++)
        {
            UpdateDefaultWorld(1);
            AZ::Vector3 velocity;
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(),
                &Physics::RigidBodyRequests::GetLinearVelocity);
            float speed = velocity.GetLength();
            EXPECT_GT(speed, previousSpeed);
            previousSpeed = speed;
        }

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, SetSleepThreshold_RollingSpheres_LowerThresholdSphereTravelsFurther)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 1.0f), 0.5f);
        auto floor = AddStaticBoxEntity(AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));

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

        UpdateDefaultWorld(300);

        EXPECT_GT(GetPositionElement(sphereB, 0), GetPositionElement(sphereA, 0));

        delete floor;
        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetSleepThresholdNegative_DynamicSphere_NegativeValueRejected)
    {
        ErrorHandler errorHandler("Negative sleep threshold value");

        auto sphere = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);

        float threshold = 0.0f, initialThreshold = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(initialThreshold, sphere->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetSleepThreshold, -0.5f);
        Physics::RigidBodyRequestBus::EventResult(threshold, sphere->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);

        EXPECT_NEAR(threshold, initialThreshold, 1e-3f);
        EXPECT_TRUE(errorHandler.GetWarningCount() > 0);

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, SetMass_Seesaw_TipsDownAtHeavierEnd)
    {
        auto floor = AddStaticBoxEntity(AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));
        auto pivot = AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 0.7f), AZ::Vector3(0.4f, 1.0f, 0.4f));
        auto seesaw = AddBoxEntity(AZ::Vector3(0.0f, 0.0f, 0.95f), AZ::Vector3(20.0f, 1.0f, 0.1f));
        auto boxA = AddBoxEntity(AZ::Vector3(-9.0f, 0.0f, 1.5f), AZ::Vector3::CreateOne());
        auto boxB = AddBoxEntity(AZ::Vector3(9.0f, 0.0f, 1.5f), AZ::Vector3::CreateOne());

        Physics::RigidBodyRequestBus::Event(boxA->GetId(), &Physics::RigidBodyRequests::SetMass, 5.0f);
        float mass = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(mass, boxA->GetId(),
            &Physics::RigidBodyRequests::GetMass);
        EXPECT_NEAR(mass, 5.0f, 1e-3f);

        UpdateDefaultWorld(30);
        EXPECT_GT(1.5f, GetPositionElement(boxA, 2));
        EXPECT_LT(1.5f, GetPositionElement(boxB, 2));

        Physics::RigidBodyRequestBus::Event(boxB->GetId(), &Physics::RigidBodyRequests::SetMass, 20.0f);
        Physics::RigidBodyRequestBus::EventResult(mass, boxB->GetId(),
            &Physics::RigidBodyRequests::GetMass);
        EXPECT_NEAR(mass, 20.0f, 1e-3f);

        UpdateDefaultWorld(60);
        EXPECT_LT(1.5f, GetPositionElement(boxA, 2));
        EXPECT_GT(1.5f, GetPositionElement(boxB, 2));

        delete floor;
        delete pivot;
        delete seesaw;
        delete boxA;
        delete boxB;
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Sphere_ValidExtents)
    {
        AZ::Vector3 spherePosition(2.0f, -3.0f, 1.0f);
        auto sphere = AddSphereEntity(spherePosition, 0.5f);

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

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Box_ValidExtents)
    {
        AZ::Vector3 boxPosition(2.0f, -3.0f, 1.0f);
        AZ::Vector3 boxDimensions(3.0f, 4.0f, 5.0f);
        auto box = AddBoxEntity(boxPosition, boxDimensions);

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

        delete box;
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Capsule_ValidExtents)
    {
        AZ::Vector3 capsulePosition(1.0f, -3.0f, 5.0f);
        float capsuleHeight = 2.0f;
        float capsuleRadius = 0.3f;
        auto capsule = AddCapsuleEntity(capsulePosition, capsuleHeight, capsuleRadius);

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

        delete capsule;
    }

    TEST_F(PhysicsComponentBusTest, ForceAwakeForceAsleep_DynamicSphere_SleepStateCorrect)
    {
        auto floor = AddStaticBoxEntity(AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));
        auto boxA = AddBoxEntity(AZ::Vector3(-5.0f, 0.0f, 1.0f), AZ::Vector3::CreateOne());
        auto boxB = AddBoxEntity(AZ::Vector3(5.0f, 0.0f, 100.0f), AZ::Vector3::CreateOne());

        UpdateDefaultWorld(60);
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

        UpdateDefaultWorld(1);

        Physics::RigidBodyRequestBus::EventResult(isAwakeA, boxA->GetId(),
            &Physics::RigidBodyRequests::IsAwake);
        Physics::RigidBodyRequestBus::EventResult(isAwakeB, boxB->GetId(),
            &Physics::RigidBodyRequests::IsAwake);

        EXPECT_TRUE(isAwakeA);
        EXPECT_FALSE(isAwakeB);

        UpdateDefaultWorld(60);

        Physics::RigidBodyRequestBus::EventResult(isAwakeA, boxA->GetId(),
            &Physics::RigidBodyRequests::IsAwake);
        Physics::RigidBodyRequestBus::EventResult(isAwakeB, boxB->GetId(),
            &Physics::RigidBodyRequests::IsAwake);

        EXPECT_FALSE(isAwakeA);
        EXPECT_FALSE(isAwakeB);

        delete boxA;
        delete boxB;
        delete floor;
    }

    TEST_F(PhysicsComponentBusTest, DisableEnablePhysics_DynamicSphere)
    {
        using namespace AzFramework;

        auto sphere = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 0.0f), 0.5f);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(), &Physics::RigidBodyRequests::SetLinearDamping, 0.0f);

        AZ::Vector3 velocity;
        float previousSpeed = 0.0f;
        for (int timestep = 0; timestep < 30; timestep++)
        {
            UpdateDefaultWorld(1);
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(), &Physics::RigidBodyRequests::GetLinearVelocity);
            previousSpeed = velocity.GetLength();
        }

        // Disable physics
        Physics::RigidBodyRequestBus::Event(sphere->GetId(), &Physics::RigidBodyRequests::DisablePhysics);

        // Check speed is not changing
        for (int timestep = 0; timestep < 60; timestep++)
        {
            UpdateDefaultWorld(1);
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
            UpdateDefaultWorld(1);
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(), &Physics::RigidBodyRequests::GetLinearVelocity);
            float speed = velocity.GetLength();
            EXPECT_GT(speed, previousSpeed);
            previousSpeed = speed;
        }

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, Shape_Box_GetAabbIsCorrect)
    {
        Physics::ColliderConfiguration colliderConfig;
        Physics::BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = AZ::Vector3(20.f, 20.f, 20.f);
        AZStd::shared_ptr<Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
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
        AZStd::shared_ptr<Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
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
        AZStd::shared_ptr<Shape> shape;
        SystemRequestBus::BroadcastResult(shape, &SystemRequests::CreateShape, colliderConfig, shapeConfiguration);
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
        AZStd::unique_ptr<AZ::Entity> box = AZStd::unique_ptr<AZ::Entity>(AddBoxEntity(AZ::Vector3(0, 0, 0), AZ::Vector3(32, 32, 32)));
        AZ::Aabb boxAABB;
        Physics::WorldBodyRequestBus::EventResult(boxAABB, box->GetId(), &Physics::WorldBodyRequests::GetAabb);
        EXPECT_TRUE(boxAABB.GetMin().IsClose(AZ::Vector3(-16, -16, -16)) && boxAABB.GetMax().IsClose(AZ::Vector3(16, 16, 16)));

        AZStd::unique_ptr<AZ::Entity> sphere = AZStd::unique_ptr<AZ::Entity>(AddSphereEntity(AZ::Vector3(-100, 0, 0), 16));
        AZ::Aabb sphereAABB;
        Physics::WorldBodyRequestBus::EventResult(sphereAABB, sphere->GetId(), &Physics::WorldBodyRequests::GetAabb);
        EXPECT_TRUE(sphereAABB.GetMin().IsClose(AZ::Vector3(-16 -100, -16, -16)) && sphereAABB.GetMax().IsClose(AZ::Vector3(16 -100, 16, 16)));

        AZStd::unique_ptr<AZ::Entity> capsule = AZStd::unique_ptr<AZ::Entity>(AddCapsuleEntity(AZ::Vector3(100, 0, 0), 128, 16));
        AZ::Aabb capsuleAABB;
        Physics::WorldBodyRequestBus::EventResult(capsuleAABB, capsule->GetId(), &Physics::WorldBodyRequests::GetAabb);
        EXPECT_TRUE(capsuleAABB.GetMin().IsClose(AZ::Vector3(-16 +100, -16, -64)) && capsuleAABB.GetMax().IsClose(AZ::Vector3(16 +100, 16, 64)));
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyBus_StaticRigidBodyColliders_AABBAreCorrect)
    {
        // Create 3 colliders, one of each type and check that the AABB of their body is the expected
        AZStd::unique_ptr<AZ::Entity> box = AZStd::unique_ptr<AZ::Entity>(AddStaticBoxEntity(AZ::Vector3(0, 0, 0), AZ::Vector3(32, 32, 32)));
        AZ::Aabb boxAABB;
        Physics::WorldBodyRequestBus::EventResult(boxAABB, box->GetId(), &Physics::WorldBodyRequests::GetAabb);
        EXPECT_TRUE(boxAABB.GetMin().IsClose(AZ::Vector3(-16, -16, -16)) && boxAABB.GetMax().IsClose(AZ::Vector3(16, 16, 16)));

        AZStd::unique_ptr<AZ::Entity> sphere = AZStd::unique_ptr<AZ::Entity>(AddStaticSphereEntity(AZ::Vector3(-100, 0, 0), 16));
        AZ::Aabb sphereAABB;
        Physics::WorldBodyRequestBus::EventResult(sphereAABB, sphere->GetId(), &Physics::WorldBodyRequests::GetAabb);
        EXPECT_TRUE(sphereAABB.GetMin().IsClose(AZ::Vector3(-16 -100, -16, -16)) && sphereAABB.GetMax().IsClose(AZ::Vector3(16 -100, 16, 16)));

        AZStd::unique_ptr<AZ::Entity> capsule = AZStd::unique_ptr<AZ::Entity>(AddStaticCapsuleEntity(AZ::Vector3(100, 0, 0), 128, 16));
        AZ::Aabb capsuleAABB;
        Physics::WorldBodyRequestBus::EventResult(capsuleAABB, capsule->GetId(), &Physics::WorldBodyRequests::GetAabb);
        EXPECT_TRUE(capsuleAABB.GetMin().IsClose(AZ::Vector3(-16 +100, -16, -64)) && capsuleAABB.GetMax().IsClose(AZ::Vector3(16 +100, 16, 64)));
    }

    using CreateEntityFunc = AZStd::function<AZStd::unique_ptr<AZ::Entity>(const AZ::Vector3&)>;

    void CheckDisableEnablePhysics(AZStd::vector<CreateEntityFunc> entityCreations)
    {
        // Fake Pointer for filling result to make sure that m_body has changed
        Physics::WorldBody* fakeBody = (Physics::WorldBody*)(0x1234);

        int i = 0;
        for (CreateEntityFunc entityCreation : entityCreations)
        {
            AZ::Vector3 entityPos = AZ::Vector3(128.f * i, 0, 0);
            AZStd::unique_ptr<AZ::Entity> entity = entityCreation(entityPos);

            Physics::RayCastHit hit;
            Physics::RayCastRequest request;
            request.m_start = entityPos + AZ::Vector3(0, 0, 100);
            request.m_direction = AZ::Vector3(0, 0, -1);
            request.m_distance = 200.f;

            Physics::WorldBodyRequestBus::Event(entity->GetId(), &Physics::WorldBodyRequests::DisablePhysics);

            bool enabled = true;
            Physics::WorldBodyRequestBus::EventResult(enabled, entity->GetId(), &Physics::WorldBodyRequests::IsPhysicsEnabled);
            EXPECT_FALSE(enabled);

            hit.m_body = fakeBody;
            Physics::WorldRequestBus::BroadcastResult(hit, &Physics::WorldRequests::RayCast, request);
            EXPECT_FALSE(hit);

            Physics::WorldBodyRequestBus::Event(entity->GetId(), &Physics::WorldBodyRequests::EnablePhysics);

            enabled = false;
            Physics::WorldBodyRequestBus::EventResult(enabled, entity->GetId(), &Physics::WorldBodyRequests::IsPhysicsEnabled);
            EXPECT_TRUE(enabled);

            hit.m_body = nullptr;
            Physics::WorldRequestBus::BroadcastResult(hit, &Physics::WorldRequests::RayCast, request);
            EXPECT_TRUE(hit && hit.m_body->GetEntityId() == entity->GetId());

            ++i;
        }
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyBus_EnableDisablePhysics_StaticRigidBody)
    {
        AZStd::vector<CreateEntityFunc> entityCreations =
        {
            [this](const AZ::Vector3& position) { return AZStd::unique_ptr<AZ::Entity>(AddStaticBoxEntity(position, AZ::Vector3(32, 32, 32))); },
            [this](const AZ::Vector3& position) { return AZStd::unique_ptr<AZ::Entity>(AddStaticSphereEntity(position, 16)); },
            [this](const AZ::Vector3& position) { return AZStd::unique_ptr<AZ::Entity>(AddStaticCapsuleEntity(position, 16, 16)); }
        };
        CheckDisableEnablePhysics(entityCreations);
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyBus_EnableDisablePhysics_RigidBody)
    {
        AZStd::vector<CreateEntityFunc> entityCreations =
        {
            [this](const AZ::Vector3& position) { return AZStd::unique_ptr<AZ::Entity>(AddBoxEntity(position, AZ::Vector3(32, 32, 32))); },
            [this](const AZ::Vector3& position) { return AZStd::unique_ptr<AZ::Entity>(AddSphereEntity(position, 16)); },
            [this](const AZ::Vector3& position) { return AZStd::unique_ptr<AZ::Entity>(AddCapsuleEntity(position, 16, 16)); }
        };
        CheckDisableEnablePhysics(entityCreations);
    }

    TEST_F(PhysicsComponentBusTest, WorldBodyRayCast_CastAgainstStaticBox_ReturnsHit)
    {
        AZStd::unique_ptr<AZ::Entity> staticBoxEntity(AddStaticBoxEntity(AZ::Vector3(0.0f), AZ::Vector3(10.f, 10.f, 10.f)));

        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        Physics::RayCastHit hit;
        Physics::WorldBodyRequestBus::EventResult(hit, staticBoxEntity->GetId(), &Physics::WorldBodyRequests::RayCast, request);

        EXPECT_TRUE(hit);

        bool hitIncludeSphereEntity = (hit.m_body->GetEntityId() == staticBoxEntity->GetId());
        EXPECT_TRUE(hitIncludeSphereEntity);
    }

    using RayCastFunc = AZStd::function<Physics::RayCastHit(AZ::EntityId, const Physics::RayCastRequest&)>;
    class PhysicsRigidBodyRayBusTest
        : public Physics::GenericPhysicsInterfaceTest
        , public ::testing::WithParamInterface<RayCastFunc>
    {
    };

    TEST_P(PhysicsRigidBodyRayBusTest, ComponentRayCast_CastAgainstNothing_ReturnsNoHit)
    {
        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        auto rayCastFunction = GetParam();
        RayCastHit hit = rayCastFunction(AZ::EntityId(), request);

        EXPECT_FALSE(hit);
    }

    TEST_P(PhysicsRigidBodyRayBusTest, ComponentRayCast_CastAgainstSphere_ReturnsHit)
    {
        AZStd::unique_ptr<AZ::Entity> sphereEntity(AddSphereEntity(AZ::Vector3(0.0f), 10.0f));

        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        auto rayCastFunction = GetParam();
        RayCastHit hit = rayCastFunction(sphereEntity->GetId(), request);

        EXPECT_TRUE(hit);

        bool hitsIncludeSphereEntity = (hit.m_body->GetEntityId() == sphereEntity->GetId());
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

        RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 100.0f, 50.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        auto rayCastFunction = GetParam();
        RayCastHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

        EXPECT_TRUE(result);

        bool hitIncludeEntity = (result.m_body->GetEntityId() == shapeWithTwoBoxes->GetId());
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
            RayCastRequest request;
            request.m_start = AZ::Vector3(-100.0f, 100.0f, 50.0f);
            request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
            request.m_distance = 200.0f;

            auto rayCastFunction = GetParam();
            RayCastHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

            EXPECT_TRUE(result);
            bool hitIncludesEntity = (result.m_body->GetEntityId() == shapeWithTwoBoxes->GetId());
            EXPECT_TRUE(hitIncludesEntity);
            bool hitIncludesShape = (result.m_shape == shapes[0].get());
            EXPECT_TRUE(hitIncludesShape);
        }

        // Lower box part z=-10 (-x to +x)
        {
            RayCastRequest request;
            request.m_start = AZ::Vector3(-100.0f, 100.0f, -10.0f);
            request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
            request.m_distance = 200.0f;

            auto rayCastFunction = GetParam();
            RayCastHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

            EXPECT_TRUE(result);
            bool hitIncludesEntity = (result.m_body->GetEntityId() == shapeWithTwoBoxes->GetId());
            EXPECT_TRUE(hitIncludesEntity);
            bool hitIncludesShape = (result.m_shape == shapes[1].get());
            EXPECT_TRUE(hitIncludesShape);
        }

        // Trace Vertically from top, it should retrieve the upper box shape
        {
            RayCastRequest request;
            request.m_start = AZ::Vector3(0.0f, 100.0f, 80.0f);
            request.m_direction = AZ::Vector3(0.0f, 0.0f, -1.0f);
            request.m_distance = 200.0f;

            auto rayCastFunction = GetParam();
            RayCastHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

            EXPECT_TRUE(result);
            bool hitIncludesEntity = (result.m_body->GetEntityId() == shapeWithTwoBoxes->GetId());
            EXPECT_TRUE(hitIncludesEntity);
            bool hitIncludesShape = (result.m_shape == shapes[0].get());
            EXPECT_TRUE(hitIncludesShape);
        }

        // Trace Vertically from bottom, it should retrieve the lower box shape
        {
            RayCastRequest request;
            request.m_start = AZ::Vector3(0.0f, 100.0f, -80.0f);
            request.m_direction = AZ::Vector3(0.0f, 0.0f, 1.0f);
            request.m_distance = 200.0f;

            auto rayCastFunction = GetParam();
            RayCastHit result = rayCastFunction(shapeWithTwoBoxes->GetId(), request);

            EXPECT_TRUE(result);
            bool hitIncludesEntity = (result.m_body->GetEntityId() == shapeWithTwoBoxes->GetId());
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

        RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f, 0.0f, 0.0f);
        request.m_direction = AZ::Vector3(-1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        auto rayCastFunction = GetParam();
        RayCastHit result = rayCastFunction(shapeWithOneBox->GetId(), request);

        EXPECT_TRUE(result);
        bool hitIncludesEntity = (result.m_body->GetEntityId() == shapeWithOneBox->GetId());
        EXPECT_TRUE(hitIncludesEntity);
        bool hitIncludesShape = (result.m_shape == shapes[0].get());
        EXPECT_TRUE(hitIncludesShape);
    }

    static const RayCastFunc RigidBodyRaycastEBusCall = [](AZ::EntityId entityId, const Physics::RayCastRequest& request)
    {
        Physics::RayCastHit ret;
        Physics::RigidBodyRequestBus::EventResult(ret, entityId, &Physics::RigidBodyRequests::RayCast, request);
        return ret;
    };

    static const RayCastFunc WorldBodyRaycastEBusCall = [](AZ::EntityId entityId, const Physics::RayCastRequest& request)
    {
        Physics::RayCastHit ret;
        Physics::WorldBodyRequestBus::EventResult(ret, entityId, &Physics::WorldBodyRequests::RayCast, request);
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
