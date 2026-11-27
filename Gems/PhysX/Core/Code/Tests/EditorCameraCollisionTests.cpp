/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/Components/EditorCameraCollision.h>
#include <Tests/EditorTestUtilities.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzToolsFramework/API/EditorCameraBus.h>

namespace PhysXEditorTests
{
    class EditorCameraCollisionFixture : public PhysXEditorFixture
    {
    public:
        void AddBox(
            const AZ::Vector3& position, const AZ::Vector3& dimensions, bool dynamic = false, bool trigger = false,
            const AZ::Quaternion& orientation = AZ::Quaternion::CreateIdentity())
        {
            auto collider = AZStd::make_shared<Physics::ColliderConfiguration>();
            collider->m_isTrigger = trigger;
            const AzPhysics::ShapeColliderPair shape(collider, AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions));
            if (dynamic)
            {
                AzPhysics::RigidBodyConfiguration configuration;
                configuration.m_position = position;
                configuration.m_orientation = orientation;
                configuration.m_colliderAndShapeData = shape;
                ASSERT_NE(m_defaultScene->AddSimulatedBody(&configuration), AzPhysics::InvalidSimulatedBodyHandle);
            }
            else
            {
                AzPhysics::StaticRigidBodyConfiguration configuration;
                configuration.m_position = position;
                configuration.m_orientation = orientation;
                configuration.m_colliderAndShapeData = shape;
                ASSERT_NE(m_defaultScene->AddSimulatedBody(&configuration), AzPhysics::InvalidSimulatedBodyHandle);
            }
        }

        void AddHeightfield(float slope = 0.0f, bool centralHole = false)
        {
            auto heightfield = AZStd::make_shared<Physics::HeightfieldShapeConfiguration>();
            heightfield->SetNumColumnVertices(21);
            heightfield->SetNumRowVertices(21);
            heightfield->SetGridResolution(AZ::Vector2(1.0f));
            heightfield->SetMinHeightBounds(-10.0f);
            heightfield->SetMaxHeightBounds(10.0f);
            AZStd::vector<Physics::HeightMaterialPoint> samples;
            for (int row = 0; row < 21; ++row)
            {
                for (int column = 0; column < 21; ++column)
                {
                    const bool hole = centralHole && row >= 8 && row < 12 && column >= 8 && column < 12;
                    samples.emplace_back(
                        slope * (column - 10),
                        hole ? Physics::QuadMeshType::Hole : Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 0);
                }
            }
            heightfield->SetSamples(samples);
            AzPhysics::StaticRigidBodyConfiguration configuration;
            configuration.m_colliderAndShapeData =
                AzPhysics::ShapeColliderPair(AZStd::make_shared<Physics::ColliderConfiguration>(), heightfield);
            ASSERT_NE(m_defaultScene->AddSimulatedBody(&configuration), AzPhysics::InvalidSimulatedBodyHandle);
        }

        AZ::Vector3 Move(const AZ::Vector3& from, const AZ::Vector3& to)
        {
            return PhysX::ResolveCameraCollision(m_defaultSceneHandle, from, to);
        }
    };

    TEST_F(EditorCameraCollisionFixture, NoColliderAllowsFreeMovement)
    {
        const AZ::Vector3 target(10.0f, -20.0f, 5.0f);
        EXPECT_TRUE(Move(AZ::Vector3::CreateZero(), target).IsClose(target));
    }

    TEST_F(EditorCameraCollisionFixture, StaticThinWallStopsHighSpeedMovement)
    {
        AddBox(AZ::Vector3(0.0f, 5.0f, 0.0f), AZ::Vector3(100.0f, 0.02f, 100.0f));
        const auto result = Move(AZ::Vector3::CreateZero(), AZ::Vector3(0.0f, 1000.0f, 0.0f));
        EXPECT_NEAR(result.GetY(), 4.739f, 0.003f);
    }

    TEST_F(EditorCameraCollisionFixture, DynamicBodyAlsoStopsCamera)
    {
        AddBox(AZ::Vector3(0.0f, 5.0f, 0.0f), AZ::Vector3(10.0f, 1.0f, 10.0f), true);
        const auto result = Move(AZ::Vector3::CreateZero(), AZ::Vector3(0.0f, 10.0f, 0.0f));
        EXPECT_NEAR(result.GetY(), 4.249f, 0.003f);
    }

    TEST_F(EditorCameraCollisionFixture, TriggerDoesNotBlockCamera)
    {
        AddBox(AZ::Vector3(0.0f, 5.0f, 0.0f), AZ::Vector3(10.0f, 1.0f, 10.0f), false, true);
        const AZ::Vector3 target(0.0f, 10.0f, 0.0f);
        EXPECT_TRUE(Move(AZ::Vector3::CreateZero(), target).IsClose(target));
    }

    TEST_F(EditorCameraCollisionFixture, CameraSlidesAlongWall)
    {
        AddBox(AZ::Vector3(0.0f, 5.0f, 0.0f), AZ::Vector3(100.0f, 1.0f, 100.0f));
        const auto result = Move(AZ::Vector3::CreateZero(), AZ::Vector3(10.0f, 10.0f, 0.0f));
        EXPECT_NEAR(result.GetX(), 10.0f, 0.003f);
        EXPECT_NEAR(result.GetY(), 4.25f, 0.003f);
    }

    TEST_F(EditorCameraCollisionFixture, CameraStopsInCornerAndCanMoveAway)
    {
        AddBox(AZ::Vector3(0.0f, 5.0f, 0.0f), AZ::Vector3(100.0f, 1.0f, 100.0f));
        AddBox(AZ::Vector3(5.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 100.0f, 100.0f));
        const auto result = Move(AZ::Vector3::CreateZero(), AZ::Vector3(10.0f, 10.0f, 0.0f));
        EXPECT_NEAR(result.GetX(), 4.25f, 0.003f);
        EXPECT_NEAR(result.GetY(), 4.25f, 0.003f);
        EXPECT_TRUE(Move(result, AZ::Vector3::CreateZero()).IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(EditorCameraCollisionFixture, CameraSlidesAlongRotatedCollider)
    {
        const auto rotation = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f));
        const auto normal = rotation.TransformVector(AZ::Vector3::CreateAxisY());
        const auto tangent = rotation.TransformVector(AZ::Vector3::CreateAxisX());
        AddBox(normal * 5.0f, AZ::Vector3(100.0f, 1.0f, 100.0f), false, false, rotation);
        const auto result = Move(AZ::Vector3::CreateZero(), normal * 10.0f + tangent * 10.0f);
        EXPECT_NEAR(result.Dot(normal), 4.25f, 0.004f);
        EXPECT_NEAR(result.Dot(tangent), 10.0f, 0.004f);
    }

    TEST_F(EditorCameraCollisionFixture, RepeatedMovementIntoWallRemainsStable)
    {
        AddBox(AZ::Vector3(0.0f, 5.0f, 0.0f), AZ::Vector3(100.0f, 1.0f, 100.0f));
        auto position = Move(AZ::Vector3::CreateZero(), AZ::Vector3(0.0f, 10.0f, 0.0f));
        const auto stoppedPosition = position;
        for (int frame = 0; frame < 120; ++frame)
        {
            position = Move(position, position + AZ::Vector3(0.0f, 0.1f, 0.0f));
            EXPECT_TRUE(position.IsClose(stoppedPosition, 0.001f));
        }
        EXPECT_TRUE(Move(position, AZ::Vector3::CreateZero()).IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(EditorCameraCollisionFixture, InitialPenetrationSeparatesStationaryCamera)
    {
        AddBox(AZ::Vector3::CreateZero(), AZ::Vector3(10.0f, 1.0f, 10.0f));
        const AZ::Vector3 position(0.0f, 0.6f, 0.0f);
        const auto result = Move(position, position);
        EXPECT_GE(result.GetY(), 0.749f);
        EXPECT_LT(result.GetY(), 0.76f);
    }

    TEST_F(EditorCameraCollisionFixture, CameraCanMoveAwayFromExactContact)
    {
        AddBox(AZ::Vector3(0.0f, 5.0f, 0.0f), AZ::Vector3(100.0f, 1.0f, 100.0f));
        EXPECT_TRUE(Move(AZ::Vector3(0.0f, 4.25f, 0.0f), AZ::Vector3::CreateZero()).IsClose(AZ::Vector3::CreateZero(), 0.003f));
    }

    TEST_F(EditorCameraCollisionFixture, HeightfieldStopsHighSpeedDescent)
    {
        AddHeightfield();
        const auto result = Move(AZ::Vector3(0.0f, 0.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1000.0f));
        EXPECT_NEAR(result.GetZ(), 0.251f, 0.003f);
    }

    TEST_F(EditorCameraCollisionFixture, CameraSlidesAlongHeightfield)
    {
        AddHeightfield();
        const auto result = Move(AZ::Vector3(0.0f, 0.0f, 2.0f), AZ::Vector3(4.0f, 0.0f, -2.0f));
        EXPECT_NEAR(result.GetX(), 4.0f, 0.003f);
        EXPECT_NEAR(result.GetZ(), 0.25f, 0.003f);
        EXPECT_TRUE(Move(result, result + AZ::Vector3::CreateAxisZ()).IsClose(result + AZ::Vector3::CreateAxisZ()));
    }

    TEST_F(EditorCameraCollisionFixture, CameraSlidesAlongSlopedHeightfield)
    {
        AddHeightfield(0.5f);
        const auto result = Move(AZ::Vector3(0.0f, 0.0f, 2.0f), AZ::Vector3(4.0f, 0.0f, -2.0f));
        const auto normal = AZ::Vector3(-0.5f, 0.0f, 1.0f).GetNormalized();
        EXPECT_NEAR(result.Dot(normal), 0.25f, 0.004f);
        EXPECT_GT(result.GetX(), 1.0f);
    }

    TEST_F(EditorCameraCollisionFixture, CameraPassesThroughHeightfieldHole)
    {
        AddHeightfield(0.0f, true);
        const AZ::Vector3 target(0.0f, 0.0f, -2.0f);
        EXPECT_TRUE(Move(AZ::Vector3(0.0f, 0.0f, 2.0f), target).IsClose(target));
        // Solid terrain alongside the hole must still block the camera.
        EXPECT_NEAR(Move(AZ::Vector3(5.0f, 0.0f, 2.0f), AZ::Vector3(5.0f, 0.0f, -2.0f)).GetZ(), 0.251f, 0.003f);
    }

    TEST_F(EditorCameraCollisionFixture, HeightfieldSeparatesInitiallyOverlappingCamera)
    {
        AddHeightfield();
        const AZ::Vector3 position(0.0f, 0.0f, 0.1f);
        EXPECT_NEAR(Move(position, position).GetZ(), 0.251f, 0.003f);
    }

    TEST_F(EditorCameraCollisionFixture, HeightfieldBlocksCameraFromBelow)
    {
        AddHeightfield();
        EXPECT_NEAR(Move(AZ::Vector3(0.0f, 0.0f, -2.0f), AZ::Vector3(0.0f, 0.0f, 2.0f)).GetZ(), -0.251f, 0.003f);
    }

    TEST_F(EditorCameraCollisionFixture, SlopedHeightfieldSeparatesStationaryCamera)
    {
        AddHeightfield(0.5f);
        const auto normal = AZ::Vector3(-0.5f, 0.0f, 1.0f).GetNormalized();
        const AZ::Vector3 position = normal * 0.1f;
        const auto result = Move(position, position);
        EXPECT_NEAR(result.Dot(normal), 0.251f, 0.004f);
        EXPECT_TRUE(Move(result, result).IsClose(result, 0.001f));
    }

    TEST_F(EditorCameraCollisionFixture, HeightfieldRecoversPenetrationDuringTangentialMovement)
    {
        AddHeightfield();
        const auto result = Move(AZ::Vector3(0.0f, 0.0f, 0.1f), AZ::Vector3(1.0f, 0.0f, 0.1f));
        EXPECT_NEAR(result.GetX(), 1.0f, 0.003f);
        EXPECT_NEAR(result.GetZ(), 0.251f, 0.003f);
    }

    TEST_F(EditorCameraCollisionFixture, PenetrationRecoveryIgnoresTriggers)
    {
        AddBox(AZ::Vector3::CreateZero(), AZ::Vector3(10.0f), false, true);
        const auto position = AZ::Vector3::CreateZero();
        EXPECT_TRUE(Move(position, position).IsClose(position));
    }

    TEST_F(EditorCameraCollisionFixture, InvalidSceneAllowsFreeMovement)
    {
        const AZ::Vector3 target(0.0f, 10.0f, 0.0f);
        EXPECT_TRUE(PhysX::ResolveCameraCollision(AzPhysics::InvalidSceneHandle, AZ::Vector3::CreateZero(), target).IsClose(target));
    }

    TEST_F(EditorCameraCollisionFixture, EditorProviderToggleControlsCollision)
    {
        auto* provider = AZ::Interface<Camera::EditorCameraCollisionInterface>::Get();
        ASSERT_NE(provider, nullptr);
        auto wall = CreateBoxPrimitiveColliderEditorEntity(
            AZ::Vector3(100.0f, 1.0f, 100.0f), AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 5.0f, 0.0f)));
        const AZ::Vector3 target(0.0f, 10.0f, 0.0f);

        provider->SetCameraCollisionEnabled(false);
        EXPECT_TRUE(provider->ConstrainCameraMovement(AZ::Vector3::CreateZero(), target).IsClose(target));
        provider->SetCameraCollisionEnabled(true);
        EXPECT_NEAR(provider->ConstrainCameraMovement(AZ::Vector3::CreateZero(), target).GetY(), 4.249f, 0.003f);
        provider->SetCameraCollisionEnabled(false);
        EXPECT_TRUE(provider->ConstrainCameraMovement(AZ::Vector3::CreateZero(), target).IsClose(target));
    }
} // namespace PhysXEditorTests
