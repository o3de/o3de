/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Matrix3x3.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestMatrix3x3Functions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromColumns_Call_GetExpectedResult)
    {
        AZ::Vector3 col1 = AZ::Vector3(1, 0, 0);
        AZ::Vector3 col2 = AZ::Vector3(0, 1, 0);
        AZ::Vector3 col3 = AZ::Vector3(0, 0, 1);
        auto actualResult = Matrix3x3Functions::FromColumns(col1, col2, col3);
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromCrossProduct_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromCrossProduct(AZ::Vector3(0, 0, 0));
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromDiagonal_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromDiagonal(AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromMatrix4x4_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromMatrix4x4(AZ::Matrix4x4::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromQuaternion_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromQuaternion(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromRotationXDegrees_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromRotationXDegrees(0);
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromRotationYDegrees_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromRotationYDegrees(0);
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromRotationZDegrees_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromRotationZDegrees(0);
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromRows_Call_GetExpectedResult)
    {
        AZ::Vector3 row1 = AZ::Vector3(1, 0, 0);
        AZ::Vector3 row2 = AZ::Vector3(0, 1, 0);
        AZ::Vector3 row3 = AZ::Vector3(0, 0, 1);
        auto actualResult = Matrix3x3Functions::FromRows(row1, row2, row3);
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromScale_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromScale(AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, FromTransform_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::FromTransform(AZ::Transform::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, GetColumn_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::GetColumn(AZ::Matrix3x3::CreateIdentity(), 0);
        EXPECT_EQ(actualResult, AZ::Vector3(1, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, GetColumns_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::GetColumns(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector3(1, 0, 0));
        EXPECT_EQ(AZStd::get<1>(actualResult), AZ::Vector3(0, 1, 0));
        EXPECT_EQ(AZStd::get<2>(actualResult), AZ::Vector3(0, 0, 1));
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, GetDiagonal_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::GetDiagonal(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Vector3(1, 1, 1));
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, GetElement_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::GetElement(AZ::Matrix3x3::CreateIdentity(), 0, 0);
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, GetRow_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::GetRow(AZ::Matrix3x3::CreateIdentity(), 0);
        EXPECT_EQ(actualResult, AZ::Vector3(1, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, GetRows_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::GetRows(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector3(1, 0, 0));
        EXPECT_EQ(AZStd::get<1>(actualResult), AZ::Vector3(0, 1, 0));
        EXPECT_EQ(AZStd::get<2>(actualResult), AZ::Vector3(0, 0, 1));
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, Invert_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::Invert(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, IsClose_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::IsClose(AZ::Matrix3x3::CreateIdentity(), AZ::Matrix3x3::CreateIdentity(), 0.0001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, IsFinite_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::IsFinite(AZ::Matrix3x3::CreateIdentity());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, IsOrthogonal_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::IsOrthogonal(AZ::Matrix3x3::CreateIdentity());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, MultiplyByNumber_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::MultiplyByNumber(AZ::Matrix3x3::CreateIdentity(), 1);
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, MultiplyByVector_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::MultiplyByVector(AZ::Matrix3x3::CreateIdentity(), AZ::Vector3(2, 2, 2));
        EXPECT_EQ(actualResult, AZ::Vector3(2, 2, 2));
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, Orthogonalize_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::Orthogonalize(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, ToAdjugate_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::ToAdjugate(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, ToDeterminant_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::ToDeterminant(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, ToScale_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::ToScale(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Vector3(1, 1, 1));
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, Transpose_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::Transpose(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix3x3::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix3x3Functions, Zero_Call_GetExpectedResult)
    {
        auto actualResult = Matrix3x3Functions::Zero();
        EXPECT_EQ(actualResult.RetrieveScale(), AZ::Vector3::CreateZero());
    }
} // namespace ScriptCanvasUnitTest
