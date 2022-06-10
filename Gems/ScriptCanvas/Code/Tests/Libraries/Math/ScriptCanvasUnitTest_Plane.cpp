/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Plane.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestPlaneFunctions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, DistanceToPoint_Call_GetExpectedResult)
    {
        AZ::Plane source = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero());
        auto actualResult = PlaneFunctions::DistanceToPoint(source, AZ::Vector3::CreateZero());
        EXPECT_TRUE(actualResult - 0 < 0.001);
    }

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, FromNormalAndDistance_Call_GetExpectedResult)
    {
        auto actualResult = PlaneFunctions::FromNormalAndDistance(AZ::Vector3(0, 0, 1), 0);
        EXPECT_EQ(actualResult, AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero()));
    }

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, FromNormalAndPoint_Call_GetExpectedResult)
    {
        auto actualResult = PlaneFunctions::FromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero());
        EXPECT_EQ(actualResult, AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero()));
    }

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, GetDistance_Call_GetExpectedResult)
    {
        AZ::Plane source = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero());
        auto actualResult = PlaneFunctions::GetDistance(source);
        EXPECT_TRUE(actualResult - 0 < 0.001);
    }

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, GetNormal_Call_GetExpectedResult)
    {
        AZ::Plane source = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero());
        auto actualResult = PlaneFunctions::GetNormal(source);
        EXPECT_EQ(actualResult, AZ::Vector3(0, 0, 1));
    }

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, GetPlaneEquationCoefficients_Call_GetExpectedResult)
    {
        AZ::Plane source = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero());
        auto actualResult = PlaneFunctions::GetPlaneEquationCoefficients(source);
        EXPECT_EQ(AZStd::get<0>(actualResult), 0);
        EXPECT_EQ(AZStd::get<1>(actualResult), 0);
        EXPECT_EQ(AZStd::get<2>(actualResult), 1);
        EXPECT_EQ(AZStd::get<3>(actualResult), 0);
    }

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, IsFinite_Call_GetExpectedResult)
    {
        AZ::Plane source = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero());
        auto actualResult = PlaneFunctions::IsFinite(source);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, Project_Call_GetExpectedResult)
    {
        AZ::Plane source = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero());
        auto actualResult = PlaneFunctions::Project(source, AZ::Vector3(0, 0, 10));
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestPlaneFunctions, Transform_Call_GetExpectedResult)
    {
        AZ::Plane source = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3::CreateZero());
        auto actualResult = PlaneFunctions::Transform(source, AZ::Transform::CreateTranslation(AZ::Vector3(0, 0, 1)));
        EXPECT_EQ(actualResult, AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0, 0, 1), AZ::Vector3(0, 0, 1)));
    }
} // namespace ScriptCanvasUnitTest
