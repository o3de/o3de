/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <PhysX/ColliderComponentBus.h>
#include <PhysX/Material/PhysXMaterial.h>
#include <PhysX/Material/PhysXMaterialConfiguration.h>

#include <Tests/PhysXGenericTestFixture.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysX
{
    TEST_F(GenericPhysicsInterfaceTest, Gravity_DynamicBody_BodyFalls)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();
        auto* rigidBody = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 100.0f));
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.f, 60);

        // expect velocity to be -gt and distance fallen to be 1/2gt^2, but allow quite a lot of tolerance
        // due to potential differences in back end integration schemes etc.
        EXPECT_NEAR(rigidBody->GetLinearVelocity().GetZ(), -10.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetTransform().GetTranslation().GetZ(), 95.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetCenterOfMassWorld().GetZ(), 95.0f, 0.5f);
        EXPECT_NEAR(rigidBody->GetPosition().GetZ(), 95.0f, 0.5f);
    }

    TEST_F(GenericPhysicsInterfaceTest, IncreaseMass_StaggeredTowerOfBoxes_TowerOverbalances)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        // make a tower of boxes which is staggered but should still balance if all the blocks are the same mass
        TestUtils::AddStaticUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 0.5f));
        AzPhysics::RigidBody* boxB = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.3f, 0.0f, 1.5f));
        AzPhysics::RigidBody* boxC = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.6f, 0.0f, 2.5f));

        // check that the tower balances
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 60);
        EXPECT_NEAR(2.5f, boxC->GetPosition().GetZ(), 0.01f);

        // increasing the mass of the top block in the tower should overbalance it
        boxC->SetMass(5.0f);
        EXPECT_NEAR(1.0f, boxB->GetMass(), 0.01f);
        EXPECT_NEAR(1.0f, boxB->GetInverseMass(), 0.01f);
        EXPECT_NEAR(5.0f, boxC->GetMass(), 0.01f);
        EXPECT_NEAR(0.2f, boxC->GetInverseMass(), 0.01f);
        boxB->ForceAwake();
        boxC->ForceAwake();
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 300);
        EXPECT_GT(0.0f, static_cast<float>(boxC->GetPosition().GetZ()));
    }

    TEST_F(GenericPhysicsInterfaceTest, GetCenterOfMass_FallingBody_CenterOfMassCorrectDuringFall)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 0.0f));
        AzPhysics::RigidBody* boxDynamic = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 2.0f));
        auto tolerance = 1e-3f;

        EXPECT_TRUE(boxDynamic->GetCenterOfMassWorld().IsClose(AZ::Vector3(0.0f, 0.0f, 2.0f), tolerance));
        EXPECT_TRUE(boxDynamic->GetCenterOfMassLocal().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), tolerance));
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 300);
        EXPECT_NEAR(static_cast<float>(boxDynamic->GetCenterOfMassWorld().GetZ()), 1.0f, 1e-3f);
        EXPECT_TRUE(boxDynamic->GetCenterOfMassLocal().IsClose(AZ::Vector3(0.0f, 0.0f, 0.0f), tolerance));
    }

    TEST_F(GenericPhysicsInterfaceTest, SetLinearVelocity_DynamicBox_AffectsTrajectory)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();
        AzPhysics::RigidBody* boxA = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, -5.0f, 10.0f));
        AzPhysics::RigidBody* boxB = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 5.0f, 10.0f));

        boxA->SetLinearVelocity(AZ::Vector3(10.0f, 0.0f, 0.0f));
        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = boxA->GetPosition().GetX();
            float xPreviousB = boxB->GetPosition().GetX();
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 10);
            EXPECT_GT(static_cast<float>(boxA->GetPosition().GetX()), xPreviousA);
            EXPECT_NEAR(boxB->GetPosition().GetX(), xPreviousB, 1e-3f);
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, ApplyLinearImpulse_DynamicBox_AffectsTrajectory)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();
        AzPhysics::RigidBody* boxA = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 100.0f));
        AzPhysics::RigidBody* boxB = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 10.0f, 100.0f));

        boxA->ApplyLinearImpulse(AZ::Vector3(10.0f, 0.0f, 0.0f));
        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = boxA->GetPosition().GetX();
            float xPreviousB = boxB->GetPosition().GetX();
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 10);
            EXPECT_GT(static_cast<float>(boxA->GetPosition().GetX()), xPreviousA);
            EXPECT_NEAR(boxB->GetPosition().GetX(), xPreviousB, 1e-3f);
        }
    }

    // allow a more generous tolerance on tests involving objects in contact, since the way physics engines normally
    // handle multiple contacts between objects can lead to slight imbalances in contact forces
    static constexpr float ContactTestTolerance = 0.01f;

    TEST_F(GenericPhysicsInterfaceTest, GetAngularVelocity_DynamicCapsuleOnSlope_GainsAngularVelocity)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        AZ::Transform slopeTransform = AZ::Transform::CreateRotationY(0.1f);
        TestUtils::AddStaticFloorToScene(sceneHandle, slopeTransform);
        AzPhysics::SimulatedBodyHandle capsuleHandle = TestUtils::AddCapsuleToScene(sceneHandle,
            slopeTransform.TransformPoint(AZ::Vector3::CreateAxisZ()));
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        auto* capsule = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, capsuleHandle));

        // the capsule should roll down the slope, picking up angular velocity parallel to the Y axis
        float angularVelocityMagnitude = capsule->GetAngularVelocity().GetLength();
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 60);
        angularVelocityMagnitude = capsule->GetAngularVelocity().GetLength();
        for (int i = 0; i < 60; i++)
        {
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);
            auto angularVelocity = capsule->GetAngularVelocity();
            EXPECT_TRUE(angularVelocity.IsPerpendicular(AZ::Vector3::CreateAxisX(), ContactTestTolerance));
            EXPECT_TRUE(angularVelocity.IsPerpendicular(AZ::Vector3::CreateAxisZ(), ContactTestTolerance));
            EXPECT_TRUE(capsule->GetAngularVelocity().GetLength() > angularVelocityMagnitude);
            angularVelocityMagnitude = angularVelocity.GetLength();
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, SetAngularVelocity_DynamicCapsule_StartsRolling)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticFloorToScene(sceneHandle);
        AzPhysics::SimulatedBodyHandle capsuleHandle = TestUtils::AddCapsuleToScene(sceneHandle, AZ::Vector3::CreateAxisZ());
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        auto* capsule = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, capsuleHandle));

        // capsule should remain stationary
        for (int i = 0; i < 60; i++)
        {
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);
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
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);
            EXPECT_TRUE(capsule->GetPosition().GetX() > xPrevious);
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, GetLinearVelocityAtWorldPoint_FallingRotatingCapsule_EdgeVelocitiesCorrect)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        // create dynamic capsule and start it falling and rotating
        AzPhysics::SimulatedBodyHandle capsuleHandle = TestUtils::AddCapsuleToScene(sceneHandle, AZ::Vector3::CreateAxisZ());
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        auto* capsule = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, capsuleHandle));

        float angularVelocityMagnitude = 1.0f;
        capsule->SetAngularVelocity(AZ::Vector3::CreateAxisY(angularVelocityMagnitude));
        capsule->SetAngularDamping(0.0f);
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 60);

        // check the velocities at some points on the rim of the capsule are as expected
        for (int i = 0; i < 60; i++)
        {
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);
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
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticFloorToScene(sceneHandle);

        // create dynamic capsule and start it rolling
        AzPhysics::SimulatedBodyHandle capsuleHandle = TestUtils::AddCapsuleToScene(sceneHandle, AZ::Vector3::CreateAxisZ());
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        auto* capsule = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, capsuleHandle));

        capsule->SetLinearVelocity(AZ::Vector3::CreateAxisX(5.0f));
        capsule->SetAngularVelocity(AZ::Vector3::CreateAxisY(10.0f));
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 60);

        // check the capsule orientation evolves as expected
        for (int i = 0; i < 60; i++)
        {
            auto orientationPrevious = capsule->GetOrientation();
            float xPrevious = capsule->GetPosition().GetX();
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);
            float angle = 2.0f * (capsule->GetPosition().GetX() - xPrevious);
            EXPECT_TRUE(capsule->GetOrientation().IsClose(orientationPrevious * AZ::Quaternion::CreateRotationY(angle)));
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, OffCenterImpulse_DynamicCapsule_StartsRotating)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticFloorToScene(sceneHandle);
        AZ::Vector3 posA(0.0f, -5.0f, 1.0f);
        AZ::Vector3 posB(0.0f, 0.0f, 1.0f);
        AZ::Vector3 posC(0.0f, 5.0f, 1.0f);
        AzPhysics::SimulatedBodyHandle capsuleAHandle = TestUtils::AddCapsuleToScene(sceneHandle, posA);
        auto* capsuleA = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, capsuleAHandle));
        AzPhysics::SimulatedBodyHandle capsuleBHandle = TestUtils::AddCapsuleToScene(sceneHandle, posB);
        auto* capsuleB = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, capsuleBHandle));
        AzPhysics::SimulatedBodyHandle capsuleCHandle = TestUtils::AddCapsuleToScene(sceneHandle, posC);
        auto* capsuleC = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, capsuleCHandle));

        // all the capsules should be stationary initially
        for (int i = 0; i < 10; i++)
        {
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);
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
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);
            EXPECT_TRUE(capsuleA->GetAngularVelocity().GetY() < 0.0f);
            EXPECT_TRUE(capsuleB->GetAngularVelocity().IsClose(AZ::Vector3::CreateZero(), ContactTestTolerance));
            EXPECT_TRUE(capsuleC->GetAngularVelocity().GetY() > 0.0f);
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, ApplyAngularImpulse_DynamicSphere_StartsRotating)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->GetScene(sceneHandle);
        }
        TestUtils::AddStaticFloorToScene(sceneHandle);

        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::RigidBody* spheres[3];
        for (int i = 0; i < 3; i++)
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = TestUtils::AddSphereToScene(sceneHandle, AZ::Vector3(0.0f, -5.0f + 5.0f * i, 1.0f));
            spheres[i] = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, simBodyHandle));
        }

        // all the spheres should start stationary
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 10);
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

        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 10);

        for (int i = 0; i < 3; i++)
        {
            auto angVel = spheres[i]->GetAngularVelocity();
            EXPECT_TRUE(angVel.GetProjected(impulses[i]).IsClose(angVel, 0.1f));
        }
    }

    TEST_F(GenericPhysicsInterfaceTest, StartAsleep_FallingBox_DoesNotFall)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        // Box should start asleep
        AzPhysics::RigidBodyConfiguration config;
        config.m_startAsleep = true;
        config.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(
            AZStd::make_shared<Physics::ColliderConfiguration>(),
            AZStd::make_shared<Physics::SphereShapeConfiguration>()
        );

        AzPhysics::SimulatedBodyHandle rigidBodyHandle = sceneInterface->AddSimulatedBody(sceneHandle, &config);
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 100);

        // Check the box is still at 0 and hasn't dropped
        AzPhysics::SimulatedBody* box = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, rigidBodyHandle);
        EXPECT_NEAR(0.0f, box->GetPosition().GetZ(), 0.01f);
    }

    TEST_F(GenericPhysicsInterfaceTest, ForceAsleep_FallingBox_BecomesStationary)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticFloorToScene(sceneHandle);
        AzPhysics::RigidBody* box = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 10.0f));
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 60);

        EXPECT_TRUE(box->IsAwake());

        auto pos = box->GetPosition();
        box->ForceAsleep();
        EXPECT_FALSE(box->IsAwake());
        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 30);
        EXPECT_FALSE(box->IsAwake());
        // the box should be asleep so it shouldn't have moved
        EXPECT_TRUE(box->GetPosition().IsClose(pos));
    }

    TEST_F(GenericPhysicsInterfaceTest, ForceAwake_SleepingBox_SleepStateCorrect)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticFloorToScene(sceneHandle);
        AzPhysics::RigidBody* box = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 1.0f));

        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 60);
        EXPECT_FALSE(box->IsAwake());

        box->ForceAwake();
        EXPECT_TRUE(box->IsAwake());

        //bring these tests into a cpp and into PhysXTests.

        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 60);
        // the box should have gone back to sleep
        EXPECT_FALSE(box->IsAwake());
    }

    TEST_F(GenericPhysicsInterfaceTest, GetAabb_Box_ValidExtents)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        AZ::Vector3 posBox(0.0f, 0.0f, 0.0f);
        AzPhysics::RigidBody* box = TestUtils::AddUnitBoxToScene(sceneHandle, posBox);

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
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        AZ::Vector3 posSphere(0.0f, 0.0f, 0.0f);
        AzPhysics::SimulatedBodyHandle simBodyHandle = TestUtils::AddSphereToScene(sceneHandle, posSphere);

        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        auto* sphere = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, simBodyHandle));

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
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        AZ::Vector3 posCapsule(0.0f, 0.0f, 0.0f);
        AzPhysics::SimulatedBodyHandle capsuleHandle = TestUtils::AddCapsuleToScene(sceneHandle, posCapsule);
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        auto* capsule = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, capsuleHandle));

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
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticFloorToScene(sceneHandle);
        AzPhysics::RigidBody* boxB = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(1.0f, 0.0f, 10.0f));
        AzPhysics::RigidBody* boxC = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(-1.0f, 0.0f, 10.0f));

        auto material = AZStd::rtti_pointer_cast<PhysX::Material>(boxC->GetShape(0)->GetMaterial());
        const float prevRestitution = material->GetRestitution();
        material->SetRestitution(1.0f);

        TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 150);

        // boxB and boxC should have the same material (default)
        // so they should both bounce high
        EXPECT_NEAR(boxB->GetPosition().GetZ(), boxC->GetPosition().GetZ(), 0.5f);

        // Restore restitution value
        material->SetRestitution(prevRestitution);
    }

#if (PX_PHYSICS_VERSION_MAJOR >= 5)
    TEST_F(GenericPhysicsInterfaceTest, Materials_BoxWithCompliantContactModeDisabled_DoesNotPenetrateFloor)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticFloorToScene(sceneHandle, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, -0.5f)));
        AzPhysics::RigidBody* box = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 2.0f));

        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_compliantContactMode.m_enabled = false;
        box->GetShape(0)->SetMaterial(PhysX::Material::CreateMaterialWithRandomId(materialConfiguration.CreateMaterialAsset()));

        for (AZ::u32 step = 0; step < 300; ++step)
        {
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);

            // At every moment the box should not penetrate the floor.
            EXPECT_GE(box->GetPosition().GetZ(), 0.5f);
        }

        // The box should settle on the floor
        EXPECT_THAT(box->GetPosition().GetZ(), ::testing::FloatNear(0.5f, 0.0001f));
    }

    TEST_F(GenericPhysicsInterfaceTest, Materials_BoxWithCompliantContactModeEnabled_PenetratesFloor)
    {
        AzPhysics::SceneHandle sceneHandle = CreateTestScene();

        TestUtils::AddStaticFloorToScene(sceneHandle, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, -0.5f)));
        AzPhysics::RigidBody* box = TestUtils::AddUnitBoxToScene(sceneHandle, AZ::Vector3(0.0f, 0.0f, 2.0f));

        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_compliantContactMode.m_enabled = true;
        box->GetShape(0)->SetMaterial(PhysX::Material::CreateMaterialWithRandomId(materialConfiguration.CreateMaterialAsset()));

        bool penetratedFloor = false;
        for (AZ::u32 step = 0; step < 300; ++step)
        {
            TestUtils::UpdateScene(sceneHandle, 1.0f / 60.0f, 1);
            if (box->GetPosition().GetZ() < 0.5f)
            {
                penetratedFloor = true;
            }
        }

        // With compliant contacts enabled the box should have penetrated the floor and bounce
        // back up like a spring.
        EXPECT_TRUE(penetratedFloor);

        // The box should settle near to the floor
        EXPECT_THAT(box->GetPosition().GetZ(), ::testing::FloatNear(0.5f, 0.0001f));
    }
#endif // (PX_PHYSICS_VERSION_MAJOR >= 5)

    TEST_F(GenericPhysicsInterfaceTest, Collider_ColliderTag_IsSetFromConfiguration)
    {
        const AZStd::string colliderTagName = "ColliderTestTag";
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_tag = colliderTagName;
        Physics::SphereShapeConfiguration shapeConfig;

        AZStd::shared_ptr<Physics::Shape> shape;
        Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape, colliderConfig, shapeConfig);

        EXPECT_EQ(shape->GetTag(), AZ::Crc32(colliderTagName));
    }

} // namespace Physics
