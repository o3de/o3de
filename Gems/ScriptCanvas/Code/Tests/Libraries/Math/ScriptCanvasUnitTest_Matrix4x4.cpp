/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/Matrix4x4.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    using ScriptCanvasUnitTestMatrix4x4Functions = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromColumns_Call_GetExpectedResult)
    {
        AZ::Vector4 col1 = AZ::Vector4(1, 0, 0, 0);
        AZ::Vector4 col2 = AZ::Vector4(0, 1, 0, 0);
        AZ::Vector4 col3 = AZ::Vector4(0, 0, 1, 0);
        AZ::Vector4 col4 = AZ::Vector4(0, 0, 0, 1);
        auto actualResult = Matrix4x4Functions::FromColumns(col1, col2, col3, col4);
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromDiagonal_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromDiagonal(AZ::Vector4(1, 1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromMatrix3x3_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromMatrix3x3(AZ::Matrix3x3::CreateIdentity());
        EXPECT_EQ(actualResult.GetColumn(0), AZ::Vector4(1, 0, 0, 0));
        EXPECT_EQ(actualResult.GetColumn(1), AZ::Vector4(0, 1, 0, 0));
        EXPECT_EQ(actualResult.GetColumn(2), AZ::Vector4(0, 0, 1, 0));
        EXPECT_EQ(actualResult.GetColumn(3), AZ::Vector4::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromQuaternion_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromQuaternion(AZ::Quaternion::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromQuaternionAndTranslation_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateZero());
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromRotationXDegrees_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromRotationXDegrees(0);
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromRotationYDegrees_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromRotationYDegrees(0);
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromRotationZDegrees_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromRotationZDegrees(0);
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromRows_Call_GetExpectedResult)
    {
        AZ::Vector4 row1 = AZ::Vector4(1, 0, 0, 0);
        AZ::Vector4 row2 = AZ::Vector4(0, 1, 0, 0);
        AZ::Vector4 row3 = AZ::Vector4(0, 0, 1, 0);
        AZ::Vector4 row4 = AZ::Vector4(0, 0, 0, 1);
        auto actualResult = Matrix4x4Functions::FromRows(row1, row2, row3, row4);
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromScale_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromScale(AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }
   
    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromTranslation_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromTranslation(AZ::Vector3(1, 1, 1));
        EXPECT_EQ(actualResult.GetColumn(0), AZ::Vector4(1, 0, 0, 0));
        EXPECT_EQ(actualResult.GetColumn(1), AZ::Vector4(0, 1, 0, 0));
        EXPECT_EQ(actualResult.GetColumn(2), AZ::Vector4(0, 0, 1, 0));
        EXPECT_EQ(actualResult.GetColumn(3), AZ::Vector4::CreateOne());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, FromTransform_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::FromTransform(AZ::Transform::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, GetColumn_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::GetColumn(AZ::Matrix4x4::CreateIdentity(), 0);
        EXPECT_EQ(actualResult, AZ::Vector4(1, 0, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, GetColumns_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::GetColumns(AZ::Matrix4x4::CreateIdentity());
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector4(1, 0, 0, 0));
        EXPECT_EQ(AZStd::get<1>(actualResult), AZ::Vector4(0, 1, 0, 0));
        EXPECT_EQ(AZStd::get<2>(actualResult), AZ::Vector4(0, 0, 1, 0));
        EXPECT_EQ(AZStd::get<3>(actualResult), AZ::Vector4(0, 0, 0, 1));
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, GetDiagonal_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::GetDiagonal(AZ::Matrix4x4::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Vector4(1, 1, 1, 1));
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, GetElement_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::GetElement(AZ::Matrix4x4::CreateIdentity(), 0, 0);
        EXPECT_EQ(actualResult, 1);
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, GetRow_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::GetRow(AZ::Matrix4x4::CreateIdentity(), 0);
        EXPECT_EQ(actualResult, AZ::Vector4(1, 0, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, GetTranslation_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::GetTranslation(AZ::Matrix4x4::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Vector3::CreateZero());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, GetRows_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::GetRows(AZ::Matrix4x4::CreateIdentity());
        EXPECT_EQ(AZStd::get<0>(actualResult), AZ::Vector4(1, 0, 0, 0));
        EXPECT_EQ(AZStd::get<1>(actualResult), AZ::Vector4(0, 1, 0, 0));
        EXPECT_EQ(AZStd::get<2>(actualResult), AZ::Vector4(0, 0, 1, 0));
        EXPECT_EQ(AZStd::get<3>(actualResult), AZ::Vector4(0, 0, 0, 1));
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, Invert_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::Invert(AZ::Matrix4x4::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, IsClose_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::IsClose(AZ::Matrix4x4::CreateIdentity(), AZ::Matrix4x4::CreateIdentity(), 0.0001);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, IsFinite_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::IsFinite(AZ::Matrix4x4::CreateIdentity());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, MultiplyByVector_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::MultiplyByVector(AZ::Matrix4x4::CreateIdentity(), AZ::Vector4(2, 2, 2, 2));
        EXPECT_EQ(actualResult, AZ::Vector4(2, 2, 2, 2));
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, ToScale_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::ToScale(AZ::Matrix4x4::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Vector3(1, 1, 1));
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, Transpose_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::Transpose(AZ::Matrix4x4::CreateIdentity());
        EXPECT_EQ(actualResult, AZ::Matrix4x4::CreateIdentity());
    }

    TEST_F(ScriptCanvasUnitTestMatrix4x4Functions, Zero_Call_GetExpectedResult)
    {
        auto actualResult = Matrix4x4Functions::Zero();
        EXPECT_EQ(actualResult.RetrieveScale(), AZ::Vector3::CreateZero());
    }
} // namespace ScriptCanvasUnitTest
