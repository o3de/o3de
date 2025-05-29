/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

using namespace AZ;

namespace UnitTest
{
    static constexpr int testIndices4x4[] = { 0, 1, 2, 3 };

    TEST(MATH_Matrix4x4, TestCreate)
    {
        Matrix4x4 m1 = Matrix4x4::CreateIdentity();
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector4(1.0f, 0.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector4(0.0f, 1.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector4(0.0f, 0.0f, 1.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(3), IsCloseTolerance(Vector4(0.0f, 0.0f, 0.0f, 1.0f), 1e-6f));
        m1 = Matrix4x4::CreateZero();
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector4(0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector4(0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector4(0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(3), IsCloseTolerance(Vector4(0.0f), 1e-6f));
        m1 = Matrix4x4::CreateFromValue(2.0f);
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector4(2.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector4(2.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector4(2.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(3), IsCloseTolerance(Vector4(2.0f), 1e-6f));
        m1 = Matrix4x4::CreateScale(Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector4(1.0f, 0.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector4(0.0f, 2.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector4(0.0f, 0.0f, 3.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(3), IsCloseTolerance(Vector4(0.0f, 0.0f, 0.0f, 1.0f), 1e-6f));
        m1 = Matrix4x4::CreateDiagonal(Vector4(2.0f, 3.0f, 4.0f, 5.0f));
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector4(2.0f, 0.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector4(0.0f, 3.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector4(0.0f, 0.0f, 4.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(3), IsCloseTolerance(Vector4(0.0f, 0.0f, 0.0f, 5.0f), 1e-6f));
        m1 = Matrix4x4::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector4(0.0f, 1.0f, 0.0f, 2.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector4(0.0f, 0.0f, 1.0f, 3.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(3), IsCloseTolerance(Vector4(0.0f, 0.0f, 0.0f, 1.0f), 1e-6f));
    }

    TEST(MATH_Matrix4x4, TestCreateFrom)
    {
        float thisTestFloats[] =
        {
             1.0f,  2.0f,  3.0f,  4.0f,
             5.0f,  6.0f,  7.0f,  8.0f,
             9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f
        };
        float testFloatMtx[16];
        Matrix4x4 m1 = Matrix4x4::CreateFromRowMajorFloat16(thisTestFloats);
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector4(1.0f, 2.0f, 3.0f, 4.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector4(5.0f, 6.0f, 7.0f, 8.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector4(9.0f, 10.0f, 11.0f, 12.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(3), IsCloseTolerance(Vector4(13.0f, 14.0f, 15.0f, 16.0f), 1e-6f));
        m1.StoreToRowMajorFloat16(testFloatMtx);
        EXPECT_EQ(memcmp(testFloatMtx, thisTestFloats, sizeof(testFloatMtx)), 0);
        m1 = Matrix4x4::CreateFromColumnMajorFloat16(thisTestFloats);
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector4(1.0f, 5.0f, 9.0f, 13.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector4(2.0f, 6.0f, 10.0f, 14.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector4(3.0f, 7.0f, 11.0f, 15.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(3), IsCloseTolerance(Vector4(4.0f, 8.0f, 12.0f, 16.0f), 1e-6f));
        m1.StoreToColumnMajorFloat16(testFloatMtx);
        EXPECT_EQ(memcmp(testFloatMtx, thisTestFloats, sizeof(testFloatMtx)), 0);
    }

    TEST(MATH_Matrix4x4, TestCreateFromMatrix3x4)
    {
        const Vector3 translation(2.0f, 3.0f, 4.0f);
        const Quaternion rotation(0.62f, 0.62f, 0.14f, 0.46f);
        const Matrix3x4 matrix3x4 = Matrix3x4::CreateFromQuaternionAndTranslation(rotation, translation);
        const Matrix4x4 matrix4x4 = Matrix4x4::CreateFromMatrix3x4(matrix3x4);
        EXPECT_THAT(matrix4x4.GetTranslation(), IsClose(translation));
        EXPECT_THAT(matrix4x4.GetRow(0), IsClose(AZ::Vector4(0.192f, 0.64f, 0.744f, 2.0f)));
        EXPECT_THAT(matrix4x4.GetRow(1), IsClose(AZ::Vector4(0.8976f, 0.192f, -0.3968f, 3.0f)));
        EXPECT_THAT(matrix4x4.GetRow(2), IsClose(AZ::Vector4(-0.3968f, 0.744f, -0.5376f, 4.0f)));
        EXPECT_THAT(matrix4x4.GetRow(3), IsClose(AZ::Vector4::CreateAxisW()));
    }

    TEST(MATH_Matrix4x4, TestCreateFromTransform)
    {
        const Vector3 translation(2.0f, 3.0f, 4.0f);
        const Quaternion rotation(0.62f, 0.62f, 0.14f, 0.46f);
        const Transform transform = Transform::CreateFromQuaternionAndTranslation(rotation, translation);
        const Matrix4x4 matrix4x4 = Matrix4x4::CreateFromTransform(transform);
        EXPECT_THAT(matrix4x4.GetTranslation(), IsClose(translation));
        EXPECT_THAT(matrix4x4.GetRow(0), IsClose(AZ::Vector4(0.192f, 0.64f, 0.744f, 2.0f)));
        EXPECT_THAT(matrix4x4.GetRow(1), IsClose(AZ::Vector4(0.8976f, 0.192f, -0.3968f, 3.0f)));
        EXPECT_THAT(matrix4x4.GetRow(2), IsClose(AZ::Vector4(-0.3968f, 0.744f, -0.5376f, 4.0f)));
        EXPECT_THAT(matrix4x4.GetRow(3), IsClose(AZ::Vector4::CreateAxisW()));
    }

    TEST(MATH_Matrix4x4, TestCreateRotation)
    {
        Matrix4x4 m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 0.866f, -0.5f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateRotationY(DegToRad(30.0f));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(0.866f, 0.0f, 0.5f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(-0.5f, 0.0f, 0.866f, 0.0f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateRotationZ(DegToRad(30.0f));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(0.866f, -0.5f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.5f, 0.866f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateFromQuaternion(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 0.866f, -0.5f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)), Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Matrix4x4, TestCreateProjection)
    {
        Matrix4x4 m1 = Matrix4x4::CreateProjection(DegToRad(30.0f), 16.0f / 9.0f, 1.0f, 1000.0f);
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(-2.099279f, 0.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 3.732f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.0f, 1.002f, -2.002f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        m1 = Matrix4x4::CreateProjectionFov(DegToRad(30.0f), DegToRad(60.0f), 1.0f, 1000.0f);
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(-3.732f, 0.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 1.732f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.0f, 1.002f, -2.002f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        m1 = Matrix4x4::CreateProjectionOffset(0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 1000.0f);
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(-4.0f, 0.0f, -3.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 4.0f, -1.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.0f, 1.002f, -2.002f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
    }

    TEST(MATH_Matrix4x4, TestElementAccess)
    {
        Matrix4x4 m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        EXPECT_NEAR(m1.GetElement(1, 2), -0.5f, 2e-3f);
        EXPECT_NEAR(m1.GetElement(2, 2), 0.866f, 2e-3f);
        m1.SetElement(2, 1, 5.0f);
        EXPECT_NEAR(m1.GetElement(2, 1), 5.0f, 2e-3f);
    }

    TEST(MATH_Matrix4x4, TestIndexAccessors)
    {
        Matrix4x4 m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        EXPECT_NEAR(m1(1, 2), -0.5f, 2e-3f);
        EXPECT_NEAR(m1(2, 2), 0.866f, 2e-3f);
        m1.SetElement(2, 1, 15.0f);
        EXPECT_NEAR(m1(2, 1), 15.0f, 1e-6f);
    }

    TEST(MATH_Matrix4x4, TestRowAccess)
    {
        Matrix4x4 m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        EXPECT_THAT(m1.GetRowAsVector3(2), IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        m1.SetRow(1, Vector3(5.0f, 6.0f, 7.0f), 8.0f);
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));
        m1.SetRow(2, Vector4(3.0f, 4.0f, 5.0f, 6.0));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(3.0f, 4.0f, 5.0f, 6.0f)));
        m1.SetRow(3, Vector4(7.0f, 8.0f, 9.0f, 10.0));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(7.0f, 8.0f, 9.0f, 10.0f)));
        //test GetRow with non-constant, we have different implementations for constants and variables
        EXPECT_THAT(m1.GetRow(testIndices4x4[0]), IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(m1.GetRow(testIndices4x4[1]), IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));
        EXPECT_THAT(m1.GetRow(testIndices4x4[2]), IsClose(Vector4(3.0f, 4.0f, 5.0f, 6.0f)));
        EXPECT_THAT(m1.GetRow(testIndices4x4[3]), IsClose(Vector4(7.0f, 8.0f, 9.0f, 10.0f)));
    }

    TEST(MATH_Matrix4x4, TestColumnAccess)
    {
        Matrix4x4 m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        EXPECT_THAT(m1.GetColumn(1), IsClose(Vector4(0.0f, 0.866f, 0.5f, 0.0f)));
        m1.SetColumn(3, 1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_THAT(m1.GetColumn(3), IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(m1.GetColumnAsVector3(3), IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f))); //checking all components in case others get messed up with the shuffling
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 0.0f, 4.0f)));
        m1.SetColumn(0, Vector4(2.0f, 3.0f, 4.0f, 5.0f));
        EXPECT_THAT(m1.GetColumn(0), IsClose(Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(2.0f, 0.0f, 0.0f, 1.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(3.0f, 0.866f, -0.5f, 2.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(4.0f, 0.5f, 0.866f, 3.0f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(5.0f, 0.0f, 0.0f, 4.0f)));
        //test GetColumn with non-constant, we have different implementations for constants and variables
        EXPECT_THAT(m1.GetColumn(testIndices4x4[0]), IsClose(Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        EXPECT_THAT(m1.GetColumn(testIndices4x4[1]), IsClose(Vector4(0.0f, 0.866f, 0.5f, 0.0f)));
        EXPECT_THAT(m1.GetColumn(testIndices4x4[2]), IsClose(Vector4(0.0f, -0.5f, 0.866f, 0.0f)));
        EXPECT_THAT(m1.GetColumn(testIndices4x4[3]), IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
    }

    TEST(MATH_Matrix4x4, TestTranslationAccess)
    {
        Matrix4x4 m1 = Matrix4x4::CreateTranslation(Vector3(5.0f, 6.0f, 7.0f));
        EXPECT_THAT(m1.GetTranslation(), IsClose(Vector3(5.0f, 6.0f, 7.0f)));
        m1.SetTranslation(1.0f, 2.0f, 3.0f);
        EXPECT_THAT(m1.GetTranslation(), IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        m1.SetTranslation(Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(m1.GetTranslation(), IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        m1.SetTranslation(4.0f, 5.0f, 6.0f);
        EXPECT_THAT(m1.GetTranslation(), IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        m1.SetTranslation(Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(m1.GetTranslation(), IsClose(Vector3(2.0f, 3.0f, 4.0f)));
    }

    TEST(MATH_Matrix4x4, TestMatrixMultiplication)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        Matrix4x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);
        m2.SetRow(3, 19.0f, 20.0f, 21.0f, 22.0f);
        Matrix4x4 m3 = m1 * m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(150.0f, 160.0f, 170.0f, 180.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(358.0f, 384.0f, 410.0f, 436.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(566.0f, 608.0f, 650.0f, 692.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(774.0f, 832.0f, 890.0f, 948.0f)));
        Matrix4x4 m4 = m1;
        m4 *= m2;
        EXPECT_THAT(m4.GetRow(0), IsClose(Vector4(150.0f, 160.0f, 170.0f, 180.0f)));
        EXPECT_THAT(m4.GetRow(1), IsClose(Vector4(358.0f, 384.0f, 410.0f, 436.0f)));
        EXPECT_THAT(m4.GetRow(2), IsClose(Vector4(566.0f, 608.0f, 650.0f, 692.0f)));
        EXPECT_THAT(m4.GetRow(3), IsClose(Vector4(774.0f, 832.0f, 890.0f, 948.0f)));
    }

    TEST(MATH_Matrix4x4, TestVectorMultiplication)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        EXPECT_THAT((m1 * Vector3(1.0f, 2.0f, 3.0f)), IsClose(Vector3(18.0f, 46.0f, 74.0f)));
        EXPECT_THAT((m1 * Vector4(1.0f, 2.0f, 3.0f, 4.0f)), IsClose(Vector4(30.0f, 70.0f, 110.0f, 150.0f)));
        EXPECT_THAT(m1.TransposedMultiply3x3(Vector3(1.0f, 2.0f, 3.0f)), IsClose(Vector3(38.0f, 44.0f, 50.0f)));
        EXPECT_THAT(m1.Multiply3x3(Vector3(1.0f, 2.0f, 3.0f)), IsClose(Vector3(14.0f, 38.0f, 62.0f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        EXPECT_THAT((v1 * m1), IsClose(Vector3(51.0f, 58.0f, 65.0f)));
        v1 *= m1;
        EXPECT_THAT(v1, IsClose(Vector3(51.0f, 58.0f, 65.0f)));
        Vector4 v2(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_THAT((v2 * m1), IsClose(Vector4(90.0f, 100.0f, 110.0f, 120.0f)));
        v2 *= m1;
        EXPECT_THAT(v2, IsClose(Vector4(90.0f, 100.0f, 110.0f, 120.0f)));
    }

    TEST(MATH_Matrix4x4, TestSum)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        Matrix4x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);
        m2.SetRow(3, 19.0f, 20.0f, 21.0f, 22.0f);

        Matrix4x4 m3 = m1 + m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(8.0f, 10.0f, 12.0f, 14.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(16.0f, 18.0f, 20.0f, 22.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(24.0f, 26.0f, 28.0f, 30.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(32.0f, 34.0f, 36.0f, 38.0f)));

        m3 = m1;
        m3 += m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(8.0f, 10.0f, 12.0f, 14.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(16.0f, 18.0f, 20.0f, 22.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(24.0f, 26.0f, 28.0f, 30.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(32.0f, 34.0f, 36.0f, 38.0f)));
    }

    TEST(MATH_Matrix4x4, TestDifference)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        Matrix4x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);
        m2.SetRow(3, 19.0f, 20.0f, 21.0f, 22.0f);

        Matrix4x4 m3 = m1 - m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        m3 = m1;
        m3 -= m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
    }

    TEST(MATH_Matrix4x4, TestScalarMultiplication)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        Matrix4x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);
        m2.SetRow(3, 19.0f, 20.0f, 21.0f, 22.0f);

        Matrix4x4 m3 = m1 * 2.0f;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(26.0f, 28.0f, 30.0f, 32.0f)));
        m3 = m1;
        m3 *= 2.0f;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(26.0f, 28.0f, 30.0f, 32.0f)));
        m3 = 2.0f * m1;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(26.0f, 28.0f, 30.0f, 32.0f)));
    }

    TEST(MATH_Matrix4x4, TestScalarDivision)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        Matrix4x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);
        m2.SetRow(3, 19.0f, 20.0f, 21.0f, 22.0f);

        Matrix4x4 m3 = m1 / 0.5f;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(26.0f, 28.0f, 30.0f, 32.0f)));
        m3 = m1;
        m3 /= 0.5f;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(26.0f, 28.0f, 30.0f, 32.0f)));
    }

    TEST(MATH_Matrix4x4, TestNegation)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        EXPECT_THAT(-(-m1), IsClose(m1));
        EXPECT_THAT(-Matrix4x4::CreateZero(), IsClose(Matrix4x4::CreateZero()));

        Matrix4x4 m2 = -m1;
        EXPECT_THAT(m2.GetRow(0), IsClose(Vector4(-1.0f, -2.0f, -3.0f, -4.0f)));
        EXPECT_THAT(m2.GetRow(1), IsClose(Vector4(-5.0f, -6.0f, -7.0f, -8.0f)));
        EXPECT_THAT(m2.GetRow(2), IsClose(Vector4(-9.0f, -10.0f, -11.0f, -12.0f)));
        EXPECT_THAT(m2.GetRow(3), IsClose(Vector4(-13.0f, -14.0f, -15.0f, -16.0f)));

        Matrix4x4 m3 = m1 + (-m1);
        EXPECT_THAT(m3, IsClose(Matrix4x4::CreateZero()));
    }

    TEST(MATH_Matrix4x4, TestTranspose)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        Matrix4x4 m2 = m1.GetTranspose();
        EXPECT_THAT(m2.GetRow(0), IsClose(Vector4(1.0f, 5.0f, 9.0f, 13.0f)));
        EXPECT_THAT(m2.GetRow(1), IsClose(Vector4(2.0f, 6.0f, 10.0f, 14.0f)));
        EXPECT_THAT(m2.GetRow(2), IsClose(Vector4(3.0f, 7.0f, 11.0f, 15.0f)));
        EXPECT_THAT(m2.GetRow(3), IsClose(Vector4(4.0f, 8.0f, 12.0f, 16.0f)));
        m2 = m1;
        m2.Transpose();
        EXPECT_THAT(m2.GetRow(0), IsClose(Vector4(1.0f, 5.0f, 9.0f, 13.0f)));
        EXPECT_THAT(m2.GetRow(1), IsClose(Vector4(2.0f, 6.0f, 10.0f, 14.0f)));
        EXPECT_THAT(m2.GetRow(2), IsClose(Vector4(3.0f, 7.0f, 11.0f, 15.0f)));
        EXPECT_THAT(m2.GetRow(3), IsClose(Vector4(4.0f, 8.0f, 12.0f, 16.0f)));
    }

    TEST(MATH_Matrix4x4, TestFastInverse)
    {
        Matrix4x4 m1;
        m1 = Matrix4x4::CreateRotationX(1.0f);
        m1.SetTranslation(Vector3(10.0f, -3.0f, 5.0f));
        EXPECT_THAT((m1 * m1.GetInverseFast()), IsCloseTolerance(Matrix4x4::CreateIdentity(), 0.02f));
        Matrix4x4 m2 = Matrix4x4::CreateRotationZ(2.0f) * Matrix4x4::CreateRotationX(1.0f);
        m2.SetTranslation(Vector3(-5.0f, 4.2f, -32.0f));
        Matrix4x4 m3 = m2.GetInverseFast();
        // allow a little bigger threshold, because of the 2 rot matrices (sin,cos differences)
        EXPECT_THAT((m2 * m3), IsCloseTolerance(Matrix4x4::CreateIdentity(), 0.1f));
        EXPECT_THAT(m3.GetRow(0), IsCloseTolerance(Vector4(-0.420f, 0.909f, 0.0f, -5.920f), 0.06f));
        EXPECT_THAT(m3.GetRow(1), IsCloseTolerance(Vector4(-0.493f, -0.228f, 0.841f, 25.418f), 0.06f));
        EXPECT_THAT(m3.GetRow(2), IsCloseTolerance(Vector4(0.765f, 0.353f, 0.542f, 19.703f), 0.06f));
        EXPECT_THAT(m3.GetRow(3), IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Matrix4x4, TestTransformInverse)
    {
        // should handle non-orthogonal matrices, last row must still be (0,0,0,1)
        Matrix4x4 m1;
        m1 = Matrix4x4::CreateRotationX(1.0f);
        m1.SetTranslation(Vector3(10.0f, -3.0f, 5.0f));
        m1.SetElement(0, 1, 23.1234f);
        m1.MultiplyByScale(Vector3(0.1f, 0.07f, 0.09f));
        EXPECT_THAT((m1 * m1.GetInverseTransform()), IsCloseTolerance(Matrix4x4::CreateIdentity(), 0.01f));
    }

    TEST(MATH_Matrix4x4, TestFullInverse)
    {
        Matrix4x4 m1;
        m1.SetRow(0, -1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 3.0f, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, -16.0f);
        EXPECT_THAT((m1 * m1.GetInverseFull()), IsClose(Matrix4x4::CreateIdentity()));
    }

    TEST(MATH_Matrix4x4, TestIsClose)
    {
        Matrix4x4 m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        Matrix4x4 m2 = m1;
        EXPECT_TRUE(m1.IsClose(m2));
        m2.SetElement(0, 0, 2.0f);
        EXPECT_FALSE(m1.IsClose(m2));
        m2 = m1;
        m2.SetElement(0, 3, 2.0f);
        EXPECT_FALSE(m1.IsClose(m2));
    }

    TEST(MATH_Matrix4x4, TestSetRotationPart)
    {
        Matrix4x4 m1 = Matrix4x4::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
        m1.SetRow(3, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));
        EXPECT_THAT(m1.GetRow(3), IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));
    }

    TEST(MATH_Matrix4x4, TestGetDiagonal)
    {
        Matrix4x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        EXPECT_THAT(m1.GetDiagonal(), IsCloseTolerance(Vector4(1.0f, 6.0f, 11.0f, 16.0f), 1e-6f));
    }

    TEST(MATH_Matrix4x4, TestScaleAccess)
    {
        Matrix4x4 m1 = Matrix4x4::CreateRotationX(DegToRad(40.0f)) * Matrix4x4::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(m1.RetrieveScale(), IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(m1.ExtractScale(), IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(m1.RetrieveScale(), IsClose(Vector3::CreateOne()));
        m1.MultiplyByScale(Vector3(3.0f, 4.0f, 5.0f));
        EXPECT_THAT(m1.RetrieveScale(), IsClose(Vector3(3.0f, 4.0f, 5.0f)));
    }

    TEST(MATH_Matrix4x4, TestScaleSqAccess)
    {
        Matrix4x4 m1 = Matrix4x4::CreateRotationX(DegToRad(40.0f)) * Matrix4x4::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(m1.RetrieveScaleSq(), IsClose(Vector3(4.0f, 9.0f, 16.0f)));
        m1.ExtractScale();
        EXPECT_THAT(m1.RetrieveScaleSq(), IsClose(Vector3::CreateOne()));
        m1.MultiplyByScale(Vector3(3.0f, 4.0f, 5.0f));
        EXPECT_THAT(m1.RetrieveScaleSq(), IsClose(Vector3(9.0f, 16.0f, 25.0f)));
    }

    TEST(MATH_Matrix4x4, TestReciprocalScaled)
    {
        Matrix4x4 orthogonalMatrix = Matrix4x4::CreateRotationX(DegToRad(40.0f));
        EXPECT_THAT(orthogonalMatrix.GetReciprocalScaled(), IsClose(orthogonalMatrix));
        const AZ::Vector3 scale(2.8f, 0.7f, 1.3f);
        AZ::Matrix4x4 scaledMatrix = orthogonalMatrix;
        scaledMatrix.MultiplyByScale(scale);
        AZ::Matrix4x4 reciprocalScaledMatrix = orthogonalMatrix;
        reciprocalScaledMatrix.MultiplyByScale(scale.GetReciprocal());
        EXPECT_THAT(scaledMatrix.GetReciprocalScaled(), IsClose(reciprocalScaledMatrix));
    }
}
