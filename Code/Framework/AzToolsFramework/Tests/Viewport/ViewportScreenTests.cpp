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

#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace UnitTest
{
    // transform a point from screen space to world space, and then from world space back to screen space
    AzFramework::ScreenPoint ScreenToWorldToScreen(
        const AzFramework::ScreenPoint& screenPoint, const AzFramework::CameraState& cameraState)
    {
        const auto worldResult = AzFramework::ScreenToWorld(screenPoint, cameraState);
        return AzFramework::WorldToScreen(worldResult, cameraState);
    }

    TEST(ViewportScreen, WorldToScreenAndScreenToWorldReturnsTheSameValueIdentityCameraOffsetFromOrigin)
    {
        using AzFramework::ScreenPoint;

        const auto screenDimensions = AZ::Vector2(800.0f, 600.0f);
        const auto cameraPosition = AZ::Vector3::CreateAxisY(-10.0f);

        // note: nearClip is 0.1 - the world space value returned will be aligned to the near clip
        // plane of the camera so use that to confirm the mapping to/from is correct
        const auto cameraState = AzFramework::CreateIdentityDefaultCamera(cameraPosition, screenDimensions);
        {
            const auto expectedScreenPoint = ScreenPoint{600, 450};
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            const auto expectedScreenPoint = ScreenPoint{400, 300};
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            const auto expectedScreenPoint = ScreenPoint{0, 0};
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            const auto expectedScreenPoint = ScreenPoint{800, 600};
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }
    }

    TEST(ViewportScreen, WorldToScreenAndScreenToWorldReturnsTheSameValueOrientatedCamera)
    {
        using AzFramework::ScreenPoint;

        const auto screenDimensions = AZ::Vector2(1024.0f, 768.0f);
        const auto cameraTransform =
            AZ::Transform::CreateRotationX(AZ::DegToRad(45.0f)) * AZ::Transform::CreateRotationZ(AZ::DegToRad(90.0f));

        const auto cameraState = AzFramework::CreateDefaultCamera(cameraTransform, screenDimensions);

        const auto expectedScreenPoint = ScreenPoint{200, 300};
        const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
        EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
    }

    TEST(ViewportScreen, ScreenToWorldReturnsPositionOnNearClipPlaneInWorldSpace)
    {
        using AzFramework::ScreenPoint;

        const auto screenDimensions = AZ::Vector2(800.0f, 600.0f);
        const auto cameraTransform = AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 0.0f, 0.0f)) *
            AZ::Transform::CreateRotationZ(AZ::DegToRad(-90.0f));

        const auto cameraState = AzFramework::CreateDefaultCamera(cameraTransform, screenDimensions);

        const auto worldResult = AzFramework::ScreenToWorld(ScreenPoint{400, 300}, cameraState);
        EXPECT_THAT(worldResult, IsClose(AZ::Vector3(10.1f, 0.0f, 0.0f)));
    }

    TEST(ViewportScreen, SubstractingScreenPointGivesScreenVector)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenVector screenVector = ScreenPoint{100, 200} - ScreenPoint{10, 20};
        EXPECT_EQ(screenVector, ScreenVector(90, 180));
    }

    TEST(ViewportScreen, AddingScreenPointAndScreenVectorGivesScreenPoint)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenPoint screenPoint = ScreenPoint{100, 200} + ScreenVector{50, 25};
        EXPECT_EQ(screenPoint, ScreenPoint(150, 225));
    }

    TEST(ViewportScreen, SubtractingScreenPointAndScreenVectorGivesScreenPoint)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenPoint screenPoint = ScreenPoint{120, 200} - ScreenVector{50, 20};
        EXPECT_EQ(screenPoint, ScreenPoint(70, 180));
    }

    TEST(ViewportScreen, AddingScreenVectorGivesScreenVector)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenVector screenVector = ScreenVector{100, 200} + ScreenVector{50, 25};
        EXPECT_EQ(screenVector, ScreenVector(150, 225));
    }

    TEST(ViewportScreen, SubtractingScreenVectorGivesScreenVector)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenVector screenVector = ScreenVector{100, 200} - ScreenVector{50, 25};
        EXPECT_EQ(screenVector, ScreenVector(50, 175));
    }

    TEST(ViewportScreen, ScreenPointAndScreenVectorConvertToVector2)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenPoint screenPoint = ScreenPoint{100, 200};
        const ScreenVector screenVector = ScreenVector{50, 25};

        const AZ::Vector2 fromScreenPoint = AzFramework::Vector2FromScreenPoint(screenPoint);
        const AZ::Vector2 fromScreenVector = AzFramework::Vector2FromScreenVector(screenVector);

        EXPECT_THAT(fromScreenPoint, IsClose(AZ::Vector2(100.0f, 200.0f)));
        EXPECT_THAT(fromScreenVector, IsClose(AZ::Vector2(50.0f, 25.0f)));
    }

    TEST(ViewportScreen, ScreenVectorPlusEqualsCanBeCombined)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        ScreenVector screenVector1 = ScreenVector(50, 175);
        ScreenVector screenVector2 = ScreenVector(2, 4);
        ScreenVector screenVector3 = ScreenVector(3, 1);

        ((screenVector1 += screenVector2) += screenVector3);

        EXPECT_EQ(screenVector1, ScreenVector(55, 180));
    }

    TEST(ViewportScreen, ScreenVectorMinusEqualsCanBeCombined)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        ScreenVector screenVector1 = ScreenVector(50, 175);
        ScreenVector screenVector2 = ScreenVector(2, 4);
        ScreenVector screenVector3 = ScreenVector(3, 1);

        ((screenVector1 -= screenVector2) -= screenVector3);

        EXPECT_EQ(screenVector1, ScreenVector(45, 170));
    }

    TEST(ViewportScreen, ScreenPointPlusEqualsScreenVectorCanBeCombined)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        ScreenPoint screenPoint = ScreenPoint(50, 175);
        ScreenVector screenVector2 = ScreenVector(2, 4);
        ScreenVector screenVector3 = ScreenVector(3, 1);

        ((screenPoint += screenVector2) += screenVector3);

        EXPECT_EQ(screenPoint, ScreenPoint(55, 180));
    }

    TEST(ViewportScreen, ScreenPointMinusEqualsScreenVectorCanBeCombined)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        ScreenPoint screenPoint = ScreenPoint(50, 175);
        ScreenVector screenVector2 = ScreenVector(2, 4);
        ScreenVector screenVector3 = ScreenVector(3, 1);

        ((screenPoint -= screenVector2) -= screenVector3);

        EXPECT_EQ(screenPoint, ScreenPoint(45, 170));
    }

    TEST(ViewportScreen, ScreenVectorLengthReturned)
    {
        using AzFramework::ScreenVector;

        EXPECT_NEAR(AzFramework::ScreenVectorLength(ScreenVector(1, 1)), 1.41421f, 0.001f);
        EXPECT_NEAR(AzFramework::ScreenVectorLength(ScreenVector(3, 4)), 5.0f, 0.001f);
        EXPECT_NEAR(AzFramework::ScreenVectorLength(ScreenVector(12, 15)), 19.20937f, 0.001f);
    }

    TEST(ViewportScreen, CanGetCameraTransformFromCameraViewAndBack)
    {
        const auto screenDimensions = AZ::Vector2(1024.0f, 768.0f);
        const auto transform = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisZ(5.0f)) *
            AZ::Transform::CreateRotationX(AZ::DegToRad(45.0f)) * AZ::Transform::CreateRotationZ(AZ::DegToRad(90.0f));

        const auto cameraState = AzFramework::CreateDefaultCamera(transform, screenDimensions);

        const auto cameraTransform = AzFramework::CameraTransform(cameraState);
        const auto cameraView = AzFramework::CameraView(cameraState);

        const auto cameraTransformFromView = AzFramework::CameraTransformFromCameraView(cameraView);
        const auto cameraViewFromTransform = AzFramework::CameraViewFromCameraTransform(cameraTransform);

        EXPECT_THAT(cameraTransform, IsClose(cameraTransformFromView));
        EXPECT_THAT(cameraView, IsClose(cameraViewFromTransform));
    }

    TEST(ViewportScreen, FovCanBeRetrievedFromProjectionMatrix)
    {
        using ::testing::FloatNear;

        auto cameraState = AzFramework::CreateIdentityDefaultCamera(AZ::Vector3::CreateZero(), AZ::Vector2(800.0f, 600.0f));

        {
            const float fovRadians = AZ::DegToRad(45.0f);
            AzFramework::SetCameraClippingVolume(cameraState, 0.1f, 100.0f, fovRadians);
            EXPECT_THAT(AzFramework::RetrieveFov(AzFramework::CameraProjection(cameraState)), FloatNear(fovRadians, 0.001f));
        }

        {
            const float fovRadians = AZ::DegToRad(90.0f);
            AzFramework::SetCameraClippingVolume(cameraState, 0.1f, 100.0f, fovRadians);
            EXPECT_THAT(AzFramework::RetrieveFov(AzFramework::CameraProjection(cameraState)), FloatNear(fovRadians, 0.001f));
        }
    }
} // namespace UnitTest
