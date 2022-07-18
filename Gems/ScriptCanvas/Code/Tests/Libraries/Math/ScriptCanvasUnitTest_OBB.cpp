/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/OBB.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestOBBFunctions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestOBBFunctions, FromAabb_Call_GetExpectedResult)
    {
        AZ::Aabb source = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(1, 1, 1));
        auto actualResult = OBBFunctions::FromAabb(source);
        EXPECT_EQ(actualResult.GetPosition(), AZ::Vector3(0.5, 0.5, 0.5));
        EXPECT_EQ(actualResult.GetRotation(), AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult.GetHalfLengths(), AZ::Vector3(0.5, 0.5, 0.5));
    }

    TEST_F(ScriptCanvasUnitTestOBBFunctions, FromPositionRotationAndHalfLengths_Call_GetExpectedResult)
    {
        auto actualResult = OBBFunctions::FromPositionRotationAndHalfLengths(
            AZ::Vector3(0.5, 0.5, 0.5), AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.5, 0.5, 0.5));
        EXPECT_EQ(actualResult.GetPosition(), AZ::Vector3(0.5, 0.5, 0.5));
        EXPECT_EQ(actualResult.GetRotation(), AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult.GetHalfLengths(), AZ::Vector3(0.5, 0.5, 0.5));
    }

    TEST_F(ScriptCanvasUnitTestOBBFunctions, IsFinite_Call_GetExpectedResult)
    {
        auto testobb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
            AZ::Vector3(0.5, 0.5, 0.5), AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.5, 0.5, 0.5));
        auto actualResult = OBBFunctions::IsFinite(testobb);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestOBBFunctions, GetAxisX_Call_GetExpectedResult)
    {
        auto testobb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
            AZ::Vector3(0.5, 0.5, 0.5), AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.5, 0.5, 0.5));
        auto actualResult = OBBFunctions::GetAxisX(testobb);
        EXPECT_EQ(actualResult, AZ::Vector3(1, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestOBBFunctions, GetAxisY_Call_GetExpectedResult)
    {
        auto testobb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
            AZ::Vector3(0.5, 0.5, 0.5), AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.5, 0.5, 0.5));
        auto actualResult = OBBFunctions::GetAxisY(testobb);
        EXPECT_EQ(actualResult, AZ::Vector3(0, 1, 0));
    }

    TEST_F(ScriptCanvasUnitTestOBBFunctions, GetAxisZ_Call_GetExpectedResult)
    {
        auto testobb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
            AZ::Vector3(0.5, 0.5, 0.5), AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.5, 0.5, 0.5));
        auto actualResult = OBBFunctions::GetAxisZ(testobb);
        EXPECT_EQ(actualResult, AZ::Vector3(0, 0, 1));
    }

    TEST_F(ScriptCanvasUnitTestOBBFunctions, GetPosition_Call_GetExpectedResult)
    {
        auto testobb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
            AZ::Vector3(0.5, 0.5, 0.5), AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.5, 0.5, 0.5));
        auto actualResult = OBBFunctions::GetPosition(testobb);
        EXPECT_EQ(actualResult, AZ::Vector3(0.5, 0.5, 0.5));
    }
} // namespace ScriptCanvasUnitTest
