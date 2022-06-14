/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/AABB.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestAABBFunctions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestAABBFunctions, AddAABB_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        AZ::Aabb b = AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(3, 3, 3));
        auto actualResult = AABBFunctions::AddAABB(a, b);
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(3, 3, 3)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, AddPoint_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::AddPoint(a, AZ::Vector3(3, 3, 3));
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(3, 3, 3)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, ApplyTransform_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::ApplyTransform(a, AZ::Transform::CreateTranslation(AZ::Vector3(1, 1, 1)));
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(3, 3, 3)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, Center_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::Center(a);
        EXPECT_EQ(actualResult, AZ::Vector3(1, 1, 1));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, Clamp_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        AZ::Aabb b = AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(3, 3, 3));
        auto actualResult = AABBFunctions::Clamp(a, b);
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(2, 2, 2)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, ContainsAABB_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        AZ::Aabb b = AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(1.5, 1.5, 1.5));
        auto actualResult = AABBFunctions::ContainsAABB(a, b);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, ContainsVector3_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::ContainsVector3(a, AZ::Vector3(1, 1, 1));
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, Distance_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::Distance(a, AZ::Vector3(3, 0, 0));
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, Expand_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::Expand(a, AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1, -1, -1), AZ::Vector3(3, 3, 3)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, Extents_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::Extents(a);
        EXPECT_EQ(actualResult, AZ::Vector3(2, 2, 2));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, FromCenterHalfExtents_Call_GetExpectedResult)
    {
        auto actualResult = AABBFunctions::FromCenterHalfExtents(AZ::Vector3(1, 1, 1), AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, FromCenterRadius_Call_GetExpectedResult)
    {
        auto actualResult = AABBFunctions::FromCenterRadius(AZ::Vector3(1, 1, 1), 1);
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, FromMinMax_Call_GetExpectedResult)
    {
        auto actualResult = AABBFunctions::FromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, FromOBB_Call_GetExpectedResult)
    {
        AZ::Obb source =
            AZ::Obb::CreateFromPositionRotationAndHalfLengths(AZ::Vector3(1, 1, 1), AZ::Quaternion::CreateIdentity(), AZ::Vector3(1, 1, 1));
        auto actualResult = AABBFunctions::FromOBB(source);
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, FromPoint_Call_GetExpectedResult)
    {
        auto actualResult = AABBFunctions::FromPoint(AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(1, 1, 1)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, GetMax_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::GetMax(a);
        EXPECT_EQ(actualResult, AZ::Vector3(2, 2, 2));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, GetMin_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::GetMin(a);
        EXPECT_EQ(actualResult, AZ::Vector3(0, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, IsFinite_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::IsFinite(a);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, IsValid_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::IsValid(a);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, Null_Call_GetExpectedResult)
    {
        auto actualResult = AABBFunctions::Null();
        EXPECT_FALSE(actualResult.IsValid());
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, Overlaps_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        AZ::Aabb b = AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(3, 3, 3));
        auto actualResult = AABBFunctions::Overlaps(a, b);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, SurfaceArea_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::SurfaceArea(a);
        EXPECT_EQ(actualResult, 2 * 2 * 6);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, ToSphere_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::ToSphere(a);
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector3(1, 1, 1));
        EXPECT_TRUE(AZStd::get<1>(actualResult) - AZStd::sqrt(3) < 0.001);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, Translate_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::Translate(a, AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(3, 3, 3)));
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, XExtent_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::XExtent(a);
        EXPECT_EQ(actualResult, 2);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, YExtent_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::YExtent(a);
        EXPECT_EQ(actualResult, 2);
    }

    TEST_F(ScriptCanvasUnitTestAABBFunctions, ZExtent_Call_GetExpectedResult)
    {
        AZ::Aabb a = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0, 0, 0), AZ::Vector3(2, 2, 2));
        auto actualResult = AABBFunctions::ZExtent(a);
        EXPECT_EQ(actualResult, 2);
    }
} // namespace ScriptCanvasUnitTest
