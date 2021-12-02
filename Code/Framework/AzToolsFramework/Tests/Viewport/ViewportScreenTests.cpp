/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <Tests/Utils/Printers.h>

namespace UnitTest
{
    // transform a point from normalized device coordinates to world space, and then from world space back to normalized device coordinates
    AZ::Vector2 ScreenNdcToWorldToScreenNdc(const AZ::Vector2& ndcPoint, const AzFramework::CameraState& cameraState)
    {
        const auto worldResult =
            AzFramework::ScreenNdcToWorld(ndcPoint, InverseCameraView(cameraState), InverseCameraProjection(cameraState));
        const auto ndcResult = AzFramework::WorldToScreenNdc(worldResult, CameraView(cameraState), CameraProjection(cameraState));
        return AZ::Vector3ToVector2(ndcResult);
    }

    // transform a point from screen space to world space, and then from world space back to screen space
    AzFramework::ScreenPoint ScreenToWorldToScreen(const AzFramework::ScreenPoint& screenPoint, const AzFramework::CameraState& cameraState)
    {
        const auto worldResult = AzFramework::ScreenToWorld(screenPoint, cameraState);
        return AzFramework::WorldToScreen(worldResult, cameraState);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ScreenPoint tests
    TEST(ViewportScreen, WorldToScreenAndScreenToWorldReturnsTheSameValueIdentityCameraOffsetFromOrigin)
    {
        using AzFramework::ScreenPoint;

        const auto screenDimensions = AZ::Vector2(800.0f, 600.0f);
        const auto cameraPosition = AZ::Vector3::CreateAxisY(-10.0f);

        const auto cameraState = AzFramework::CreateIdentityDefaultCamera(cameraPosition, screenDimensions);
        {
            const auto expectedScreenPoint = ScreenPoint{ 600, 450 };
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            const auto expectedScreenPoint = ScreenPoint{ 400, 300 };
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            const auto expectedScreenPoint = ScreenPoint{ 0, 0 };
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            const auto expectedScreenPoint = ScreenPoint{ 800, 600 };
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

        const auto expectedScreenPoint = ScreenPoint{ 200, 300 };
        const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, cameraState);
        EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
    }

    // note: nearClip is 0.1 - the world space value returned will be aligned to the near clip
    // plane of the camera so use that to confirm the mapping to/from is correct
    TEST(ViewportScreen, ScreenToWorldReturnsPositionOnNearClipPlaneInWorldSpace)
    {
        using AzFramework::ScreenPoint;

        const auto screenDimensions = AZ::Vector2(800.0f, 600.0f);
        const auto cameraTransform =
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 0.0f, 0.0f)) * AZ::Transform::CreateRotationZ(AZ::DegToRad(-90.0f));

        const auto cameraState = AzFramework::CreateDefaultCamera(cameraTransform, screenDimensions);

        const auto worldResult = AzFramework::ScreenToWorld(ScreenPoint{ 400, 300 }, cameraState);
        EXPECT_THAT(worldResult, IsClose(AZ::Vector3(10.1f, 0.0f, 0.0f)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Ndc tests
    TEST(ViewportScreen, WorldToScreenNdcAndScreenNdcToWorldReturnsTheSameValueIdentityCameraOffsetFromOrigin)
    {
        using NdcPoint = AZ::Vector2;

        const auto screenDimensions = AZ::Vector2(800.0f, 600.0f);
        const auto cameraPosition = AZ::Vector3::CreateAxisY(-10.0f);

        const auto cameraState = AzFramework::CreateIdentityDefaultCamera(cameraPosition, screenDimensions);
        {
            const auto expectedNdcPoint = NdcPoint{ 0.75f, 0.75f };
            const auto resultNdcPoint = ScreenNdcToWorldToScreenNdc(expectedNdcPoint, cameraState);
            EXPECT_THAT(resultNdcPoint, IsClose(expectedNdcPoint));
        }

        {
            const auto expectedNdcPoint = NdcPoint{ 0.5f, 0.5f };
            const auto resultNdcPoint = ScreenNdcToWorldToScreenNdc(expectedNdcPoint, cameraState);
            EXPECT_THAT(resultNdcPoint, IsClose(expectedNdcPoint));
        }

        {
            const auto expectedNdcPoint = NdcPoint{ 0.0f, 0.0f };
            const auto resultNdcPoint = ScreenNdcToWorldToScreenNdc(expectedNdcPoint, cameraState);
            EXPECT_THAT(resultNdcPoint, IsClose(expectedNdcPoint));
        }

        {
            const auto expectedNdcPoint = NdcPoint{ 1.0f, 1.0f };
            const auto resultNdcPoint = ScreenNdcToWorldToScreenNdc(expectedNdcPoint, cameraState);
            EXPECT_THAT(resultNdcPoint, IsClose(expectedNdcPoint));
        }
    }

    TEST(ViewportScreen, WorldToScreenNdcAndScreenNdcToWorldReturnsTheSameValueOrientatedCamera)
    {
        using NdcPoint = AZ::Vector2;

        const auto screenDimensions = AZ::Vector2(800.0f, 600.0f);
        const auto cameraTransform =
            AZ::Transform::CreateRotationX(AZ::DegToRad(45.0f)) * AZ::Transform::CreateRotationZ(AZ::DegToRad(90.0f));

        const auto cameraState = AzFramework::CreateDefaultCamera(cameraTransform, screenDimensions);

        const auto expectedNdcPoint = NdcPoint{ 0.25f, 0.5f };
        const auto resultNdcPoint = ScreenNdcToWorldToScreenNdc(expectedNdcPoint, cameraState);
        EXPECT_THAT(resultNdcPoint, IsClose(expectedNdcPoint));
    }

    // note: nearClip is 0.1 - the world space value returned will be aligned to the near clip
    // plane of the camera so use that to confirm the mapping to/from is correct
    TEST(ViewportScreen, ScreenNdcToWorldReturnsPositionOnNearClipPlaneInWorldSpace)
    {
        using NdcPoint = AZ::Vector2;

        const auto screenDimensions = AZ::Vector2(800.0f, 600.0f);
        const auto cameraTransform =
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 0.0f, 0.0f)) * AZ::Transform::CreateRotationZ(AZ::DegToRad(-90.0f));

        const auto cameraState = AzFramework::CreateDefaultCamera(cameraTransform, screenDimensions);

        const auto worldResult =
            AzFramework::ScreenNdcToWorld(NdcPoint{ 0.5f, 0.5f }, InverseCameraView(cameraState), InverseCameraProjection(cameraState));
        EXPECT_THAT(worldResult, IsClose(AZ::Vector3(10.1f, 0.0f, 0.0f)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ScreenVector tests
    TEST(ViewportScreen, SubstractingScreenPointGivesScreenVector)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenVector screenVector = ScreenPoint{ 100, 200 } - ScreenPoint{ 10, 20 };
        EXPECT_EQ(screenVector, ScreenVector(90, 180));
    }

    TEST(ViewportScreen, AddingScreenPointAndScreenVectorGivesScreenPoint)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenPoint screenPoint = ScreenPoint{ 100, 200 } + ScreenVector{ 50, 25 };
        EXPECT_EQ(screenPoint, ScreenPoint(150, 225));
    }

    TEST(ViewportScreen, SubtractingScreenPointAndScreenVectorGivesScreenPoint)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenPoint screenPoint = ScreenPoint{ 120, 200 } - ScreenVector{ 50, 20 };
        EXPECT_EQ(screenPoint, ScreenPoint(70, 180));
    }

    TEST(ViewportScreen, AddingScreenVectorGivesScreenVector)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenVector screenVector = ScreenVector{ 100, 200 } + ScreenVector{ 50, 25 };
        EXPECT_EQ(screenVector, ScreenVector(150, 225));
    }

    TEST(ViewportScreen, SubtractingScreenVectorGivesScreenVector)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenVector screenVector = ScreenVector{ 100, 200 } - ScreenVector{ 50, 25 };
        EXPECT_EQ(screenVector, ScreenVector(50, 175));
    }

    TEST(ViewportScreen, ScreenPointAndScreenVectorConvertToVector2)
    {
        using AzFramework::ScreenPoint;
        using AzFramework::ScreenVector;

        const ScreenPoint screenPoint = ScreenPoint{ 100, 200 };
        const ScreenVector screenVector = ScreenVector{ 50, 25 };

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

    TEST(ViewportScreen, ScreenVectorTransformedByScalarUpwards)
    {
        using AzFramework::ScreenVector;

        auto screenVector = ScreenVector(5, 10);
        auto scaledScreenVector = screenVector * 2.0f;

        EXPECT_EQ(scaledScreenVector, ScreenVector(10, 20));
    }

    TEST(ViewportScreen, ScreenVectorTransformedByScalarWithRounding)
    {
        using AzFramework::ScreenVector;

        auto screenVector = ScreenVector(1, 6);
        auto scaledScreenVector = screenVector * 0.1f;

        // value less than 0.5 rounds down, greater than or equal to 0.5 rounds up
        EXPECT_EQ(scaledScreenVector, ScreenVector(0, 1));
    }

    TEST(ViewportScreen, ScreenVectorTransformedByScalarWithRoundingAtHalfwayBoundary)
    {
        using AzFramework::ScreenVector;

        auto screenVector = ScreenVector(5, 10);
        auto scaledScreenVector = screenVector * 0.1f;

        // value less than 0.5 rounds down, greater than or equal to 0.5 rounds up
        EXPECT_EQ(scaledScreenVector, ScreenVector(1, 1));
    }

    TEST(ViewportScreen, ScreenVectorTransformedByScalarDownwards)
    {
        using AzFramework::ScreenVector;

        auto screenVector = ScreenVector(6, 12);
        auto scaledScreenVector = screenVector * 0.5f;

        EXPECT_EQ(scaledScreenVector, ScreenVector(3, 6));
    }

    TEST(ViewportScreen, ScreenVectorTransformedByScalarInplace)
    {
        using AzFramework::ScreenVector;

        auto screenVector = ScreenVector(13, 37);
        screenVector *= 10.0f;

        EXPECT_EQ(screenVector, ScreenVector(130, 370));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Other tests
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
