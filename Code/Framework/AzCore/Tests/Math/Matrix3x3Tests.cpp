/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

using namespace AZ;

namespace UnitTest
{
    static constexpr int32_t testIndices3x3[] = { 0, 1, 2, 3 };
    float testFloats[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
    float testFloatsMtx[9];

    TEST(MATH_Matrix3x3, TestCreateIdentity)
    {
        Matrix3x3 m1 = Matrix3x3::CreateIdentity();
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector3(1.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector3(0.0f, 1.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector3(0.0f, 0.0f, 1.0f), 1e-6f));
    }

    TEST(MATH_Matrix3x3, TestCreateZero)
    {
        Matrix3x3 m1 = Matrix3x3::CreateZero();
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector3(0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector3(0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector3(0.0f), 1e-6f));
    }

    TEST(MATH_Matrix3x3, TestCreateFromValue)
    {
        Matrix3x3 m1 = Matrix3x3::CreateFromValue(2.0f);
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector3(2.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector3(2.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector3(2.0f), 1e-6f));
    }

    TEST(MATH_Matrix3x3, TestCreateFromRowMajorFloat9)
    {
        Matrix3x3 m1 = Matrix3x3::CreateFromRowMajorFloat9(testFloats);
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector3(1.0f, 2.0f, 3.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector3(4.0f, 5.0f, 6.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector3(7.0f, 8.0f, 9.0f), 1e-6f));
        m1.StoreToRowMajorFloat9(testFloatsMtx);
        EXPECT_EQ(memcmp(testFloatsMtx, testFloats, sizeof(testFloatsMtx)), 0);
    }

    TEST(MATH_Matrix3x3, TestCreateFromColumnMajorFloat9)
    {
        Matrix3x3 m1 = Matrix3x3::CreateFromColumnMajorFloat9(testFloats);
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector3(1.0f, 4.0f, 7.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector3(2.0f, 5.0f, 8.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector3(3.0f, 6.0f, 9.0f), 1e-6f));
        m1.StoreToColumnMajorFloat9(testFloatsMtx);
        EXPECT_EQ(memcmp(testFloatsMtx, testFloats, sizeof(testFloatsMtx)), 0);
    }

    TEST(MATH_Matrix3x3, TestStoreToRowMajorFloat11)
    {
        Matrix3x3 m1 = Matrix3x3::CreateFromRowMajorFloat9(testFloats);
        float output[12];
        output[11] = -1;
        // we expect the first two rows to be written with 4 floats, with the last value as garbage,
        // but the last row only with 3 floats, such that the final value remains untouched
        // clang-format off
        float expected[12] = {
            testFloats[0], testFloats[1], testFloats[2], 0.0f,
            testFloats[3], testFloats[4], testFloats[5], 0.0f,
            testFloats[6], testFloats[7], testFloats[8], -1.0f};
        // clang-format on
        m1.StoreToRowMajorFloat11(output);
        EXPECT_EQ(memcmp(&expected[0], &output[0], sizeof(float) * 3), 0);
        EXPECT_EQ(memcmp(&expected[3], &output[3], sizeof(float) * 3), 0);
        EXPECT_EQ(memcmp(&expected[6], &output[6], sizeof(float) * 3), 0);
        EXPECT_EQ(output[11], -1);
    }

    TEST(MATH_Matrix3x3, TestStoreToColumnMajorFloat11)
    {
        Matrix3x3 m1 = Matrix3x3::CreateFromRowMajorFloat9(testFloats);
        float output[12];
        output[11] = -1;
        // we expect the first two columns to be written with 4 floats, with the last value as garbage,
        // but the last column only with 3 floats, such that the final value remains untouched
        // clang-format off
        float expected[12] = {
            testFloats[0], testFloats[3], testFloats[6], 0.0f,
            testFloats[1], testFloats[4], testFloats[7], 0.0f,
            testFloats[2], testFloats[5], testFloats[8], -1.0f};
        // clang-format on
        m1.StoreToColumnMajorFloat11(output);
        EXPECT_EQ(memcmp(&expected[0], &output[0], sizeof(float) * 3), 0);
        EXPECT_EQ(memcmp(&expected[3], &output[3], sizeof(float) * 3), 0);
        EXPECT_EQ(memcmp(&expected[6], &output[6], sizeof(float) * 3), 0);
        EXPECT_EQ(output[11], -1);
    }

    TEST(MATH_Matrix3x3, TestCreateRotationX)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(0.0f, 0.866f, -0.5f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(0.0f, 0.5f, 0.866f)));
    }

    TEST(MATH_Matrix3x3, TestCreateRotationY)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationY(DegToRad(30.0f));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(0.866f, 0.0f, 0.5f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(0.0f, 1.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(-0.5f, 0.0f, 0.866f)));
    }

    TEST(MATH_Matrix3x3, TestCreateRotationZ)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationZ(DegToRad(30.0f));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(0.866f, -0.5f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(0.5f, 0.866f, 0.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Matrix3x3, TestCreateFromTransform)
    {
        Matrix3x3 m1 = Matrix3x3::CreateFromTransform(Transform::CreateRotationX(DegToRad(30.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(0.0f, 0.866f, -0.5f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(0.0f, 0.5f, 0.866f)));
    }

    TEST(MATH_Matrix3x3, TestCreateFromMatrix4x4)
    {
        Matrix3x3 m1 = Matrix3x3::CreateFromMatrix4x4(Matrix4x4::CreateRotationX(DegToRad(30.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(0.0f, 0.866f, -0.5f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(0.0f, 0.5f, 0.866f)));
    }

    TEST(MATH_Matrix3x3, TestCreateFromQuaternion)
    {
        Matrix3x3 m1 = Matrix3x3::CreateFromQuaternion(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(0.0f, 0.866f, -0.5f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(0.0f, 0.5f, 0.866f)));
    }

    TEST(MATH_Matrix3x3, TestCreateScale)
    {
        Matrix3x3 m1 = Matrix3x3::CreateScale(Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector3(1.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector3(0.0f, 2.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector3(0.0f, 0.0f, 3.0f), 1e-6f));
    }

    TEST(MATH_Matrix3x3, TestCreateDiagonal)
    {
        Matrix3x3 m1 = Matrix3x3::CreateDiagonal(Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector3(2.0f, 0.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector3(0.0f, 3.0f, 0.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector3(0.0f, 0.0f, 4.0f), 1e-6f));
    }

    TEST(MATH_Matrix3x3, TestCreateCrossProduct)
    {
        Matrix3x3 m1 = Matrix3x3::CreateCrossProduct(Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_THAT(m1.GetRow(0), IsCloseTolerance(Vector3(0.0f, -3.0f, 2.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(1), IsCloseTolerance(Vector3(3.0f, 0.0f, -1.0f), 1e-6f));
        EXPECT_THAT(m1.GetRow(2), IsCloseTolerance(Vector3(-2.0f, 1.0f, 0.0f), 1e-6f));
    }

    TEST(MATH_Matrix3x3, TestElementAccess)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        EXPECT_NEAR(m1.GetElement(1, 2), -0.5f, 2e-3f);
        EXPECT_NEAR(m1.GetElement(2, 2), 0.866f, 2e-3f);
        m1.SetElement(2, 1, 5.0f);
        EXPECT_NEAR(m1.GetElement(2, 1), 5.0f, 1e-6f);

        EXPECT_NEAR(m1(1, 2), -0.5f, 2e-3f);
        EXPECT_NEAR(m1(2, 2), 0.866f, 2e-3f);
        m1.SetElement(2, 1, 15.0f);
        EXPECT_NEAR(m1(2, 1), 15.0f, 1e-6f);
    }

    TEST(MATH_Matrix3x3, TestRowAccess)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        m1.SetRow(1, Vector3(4.0f, 5.0f, 6.0f));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        m1.SetRow(2, Vector3(7.0f, 8.0f, 9.0f));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(7.0f, 8.0f, 9.0f)));

        // test GetRow with non-constant, we have different implementations for constants and variables
        EXPECT_THAT(m1.GetRow(testIndices3x3[0]), IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_THAT(m1.GetRow(testIndices3x3[1]), IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        EXPECT_THAT(m1.GetRow(testIndices3x3[2]), IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        Vector3 row0, row1, row2;
        m1.GetRows(&row0, &row1, &row2);
        EXPECT_THAT(row0, IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_THAT(row1, IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        EXPECT_THAT(row2, IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        m1.SetRows(Vector3(10.0f, 11.0f, 12.0f), Vector3(13.0f, 14.0f, 15.0f), Vector3(16.0f, 17.0f, 18.0f));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(16.0f, 17.0f, 18.0f)));
    }

    TEST(MATH_Matrix3x3, TestColumnAccess)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        EXPECT_THAT(m1.GetColumn(1), IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        m1.SetColumn(2, 1.0f, 2.0f, 3.0f);
        EXPECT_THAT(m1.GetColumn(2), IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(1.0f, 0.0f, 1.0f)));  // checking all components in case others get messed up with the shuffling
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(0.0f, 0.866f, 2.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(0.0f, 0.5f, 3.0f)));
        m1.SetColumn(0, Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(m1.GetColumn(0), IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(m1.GetRow(0), IsClose(Vector3(2.0f, 0.0f, 1.0f)));
        EXPECT_THAT(m1.GetRow(1), IsClose(Vector3(3.0f, 0.866f, 2.0f)));
        EXPECT_THAT(m1.GetRow(2), IsClose(Vector3(4.0f, 0.5f, 3.0f)));

        // test GetColumn with non-constant, we have different implementations for constants and variables
        EXPECT_THAT(m1.GetColumn(testIndices3x3[0]), IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(m1.GetColumn(testIndices3x3[1]), IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        EXPECT_THAT(m1.GetColumn(testIndices3x3[2]), IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        Vector3 col0, col1, col2;
        m1.GetColumns(&col0, &col1, &col2);
        EXPECT_THAT(col0, IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(col1, IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        EXPECT_THAT(col2, IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        m1.SetColumns(Vector3(10.0f, 11.0f, 12.0f), Vector3(13.0f, 14.0f, 15.0f), Vector3(16.0f, 17.0f, 18.0f));
        EXPECT_THAT(m1.GetColumn(0), IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        EXPECT_THAT(m1.GetColumn(1), IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        EXPECT_THAT(m1.GetColumn(2), IsClose(Vector3(16.0f, 17.0f, 18.0f)));
    }

    TEST(MATH_Matrix3x3, TestBasisAccess)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        EXPECT_THAT(m1.GetBasisX(), IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        EXPECT_THAT(m1.GetBasisY(), IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        EXPECT_THAT(m1.GetBasisZ(), IsClose(Vector3(0.0f, -0.5f, 0.866f)));
        m1.SetBasisX(1.0f, 2.0f, 3.0f);
        EXPECT_THAT(m1.GetBasisX(), IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        m1.SetBasisY(4.0f, 5.0f, 6.0f);
        EXPECT_THAT(m1.GetBasisY(), IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        m1.SetBasisZ(7.0f, 8.0f, 9.0f);
        EXPECT_THAT(m1.GetBasisZ(), IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        m1.SetBasisX(Vector3(10.0f, 11.0f, 12.0f));
        EXPECT_THAT(m1.GetBasisX(), IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        m1.SetBasisY(Vector3(13.0f, 14.0f, 15.0f));
        EXPECT_THAT(m1.GetBasisY(), IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        m1.SetBasisZ(Vector3(16.0f, 17.0f, 18.0f));
        EXPECT_THAT(m1.GetBasisZ(), IsClose(Vector3(16.0f, 17.0f, 18.0f)));

        Vector3 basis0, basis1, basis2;
        m1.GetBasis(&basis0, &basis1, &basis2);
        EXPECT_THAT(basis0, IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        EXPECT_THAT(basis1, IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        EXPECT_THAT(basis2, IsClose(Vector3(16.0f, 17.0f, 18.0f)));
        m1.SetBasis(Vector3(1.0f, 2.0f, 3.0f), Vector3(4.0f, 5.0f, 6.0f), Vector3(7.0f, 8.0f, 9.0f));
        EXPECT_THAT(m1.GetBasisX(), IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_THAT(m1.GetBasisY(), IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        EXPECT_THAT(m1.GetBasisZ(), IsClose(Vector3(7.0f, 8.0f, 9.0f)));
    }

    TEST(MATH_Matrix3x3, TestMatrixMultiplication)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f);
        m2.SetRow(1, 10.0f, 11.0f, 12.0f);
        m2.SetRow(2, 13.0f, 14.0f, 15.0f);

        Matrix3x3 m3 = m1 * m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(66.0f, 72.0f, 78.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(156.0f, 171.0f, 186.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(246.0f, 270.0f, 294.0f)));

        Matrix3x3 m4 = m1;
        m4 *= m2;
        EXPECT_THAT(m4.GetRow(0), IsClose(Vector3(66.0f, 72.0f, 78.0f)));
        EXPECT_THAT(m4.GetRow(1), IsClose(Vector3(156.0f, 171.0f, 186.0f)));
        EXPECT_THAT(m4.GetRow(2), IsClose(Vector3(246.0f, 270.0f, 294.0f)));
        m3 = m1.TransposedMultiply(m2);
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(138.0f, 150.0f, 162.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(168.0f, 183.0f, 198.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(198.0f, 216.0f, 234.0f)));
    }

    TEST(MATH_Matrix3x3, TestVectorMultiplication)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f);
        m2.SetRow(1, 10.0f, 11.0f, 12.0f);
        m2.SetRow(2, 13.0f, 14.0f, 15.0f);

        EXPECT_THAT((m1 * Vector3(1.0f, 2.0f, 3.0f)), IsClose(Vector3(14.0f, 32.0f, 50.0f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        EXPECT_THAT((v1 * m1), IsClose(Vector3(30.0f, 36.0f, 42.0f)));
        v1 *= m1;
        EXPECT_THAT(v1, IsClose(Vector3(30.0f, 36.0f, 42.0f)));
    }

    TEST(MATH_Matrix3x3, TestSum)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f);
        m2.SetRow(1, 10.0f, 11.0f, 12.0f);
        m2.SetRow(2, 13.0f, 14.0f, 15.0f);

        Matrix3x3 m3 = m1 + m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(20.0f, 22.0f, 24.0f)));

        m3 = m1;
        m3 += m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(20.0f, 22.0f, 24.0f)));
    }

    TEST(MATH_Matrix3x3, TestDifference)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f);
        m2.SetRow(1, 10.0f, 11.0f, 12.0f);
        m2.SetRow(2, 13.0f, 14.0f, 15.0f);

        Matrix3x3 m3 = m1 - m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        m3 = m1;
        m3 -= m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
    }

    TEST(MATH_Matrix3x3, TestScalarMultiplication)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f);
        m2.SetRow(1, 10.0f, 11.0f, 12.0f);
        m2.SetRow(2, 13.0f, 14.0f, 15.0f);

        Matrix3x3 m3 = m1 * 2.0f;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        m3 = m1;
        m3 *= 2.0f;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        m3 = 2.0f * m1;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(14.0f, 16.0f, 18.0f)));
    }

    TEST(MATH_Matrix3x3, TestScalarDivision)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f);
        m2.SetRow(1, 10.0f, 11.0f, 12.0f);
        m2.SetRow(2, 13.0f, 14.0f, 15.0f);

        Matrix3x3 m3 = m1 / 0.5f;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        m3 = m1;
        m3 /= 0.5f;
        EXPECT_THAT(m3.GetRow(0), IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(Vector3(14.0f, 16.0f, 18.0f)));
    }

    TEST(MATH_Matrix3x3, TestNegation)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        EXPECT_THAT(-(-m1), IsClose(m1));
        EXPECT_THAT(-Matrix3x3::CreateZero(), IsClose(Matrix3x3::CreateZero()));

        Matrix3x3 m2 = -m1;
        EXPECT_THAT(m2.GetRow(0), IsClose(Vector3(-1.0f, -2.0f, -3.0f)));
        EXPECT_THAT(m2.GetRow(1), IsClose(Vector3(-4.0f, -5.0f, -6.0f)));
        EXPECT_THAT(m2.GetRow(2), IsClose(Vector3(-7.0f, -8.0f, -9.0f)));

        Matrix3x3 m3 = m1 + (-m1);
        EXPECT_THAT(m3, IsClose(Matrix3x3::CreateZero()));
    }

    TEST(MATH_Matrix3x3, TestTranspose)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2 = m1.GetTranspose();
        EXPECT_THAT(m2.GetRow(0), IsClose(Vector3(1.0f, 4.0f, 7.0f)));
        EXPECT_THAT(m2.GetRow(1), IsClose(Vector3(2.0f, 5.0f, 8.0f)));
        EXPECT_THAT(m2.GetRow(2), IsClose(Vector3(3.0f, 6.0f, 9.0f)));
        m2 = m1;
        m2.Transpose();
        EXPECT_THAT(m2.GetRow(0), IsClose(Vector3(1.0f, 4.0f, 7.0f)));
        EXPECT_THAT(m2.GetRow(1), IsClose(Vector3(2.0f, 5.0f, 8.0f)));
        EXPECT_THAT(m2.GetRow(2), IsClose(Vector3(3.0f, 6.0f, 9.0f)));
    }

    TEST(MATH_Matrix3x3, TestFastInverse)
    {
        // orthogonal matrix only
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(1.0f);
        EXPECT_THAT((m1 * m1.GetInverseFast()), IsCloseTolerance(Matrix3x3::CreateIdentity(), 0.02f));
        Matrix3x3 m2 = Matrix3x3::CreateRotationZ(2.0f) * Matrix3x3::CreateRotationX(1.0f);
        Matrix3x3 m3 = m2.GetInverseFast();

        // allow a little bigger threshold, because of the 2 rot matrices (sin,cos differences)
        EXPECT_THAT((m2 * m3), IsCloseTolerance(Matrix3x3::CreateIdentity(), 0.1f));
        EXPECT_THAT(m3.GetRow(0), IsCloseTolerance(Vector3(-0.420f, 0.909f, 0.0f), 0.06f));
        EXPECT_THAT(m3.GetRow(1), IsCloseTolerance(Vector3(-0.493f, -0.228f, 0.841f), 0.06f));
        EXPECT_THAT(m3.GetRow(2), IsCloseTolerance(Vector3(0.765f, 0.353f, 0.542f), 0.06f));
    }

    TEST(MATH_Matrix3x3, TestFullInverse)
    {
        Matrix3x3 m1;
        m1.SetRow(0, -1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, -9.0f);
        EXPECT_THAT((m1 * m1.GetInverseFull()), IsClose(Matrix3x3::CreateIdentity()));
    }

    TEST(MATH_Matrix3x3, TestScaleAccess)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(40.0f)) * Matrix3x3::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(m1.RetrieveScale(), IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(m1.ExtractScale(), IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        EXPECT_THAT(m1.RetrieveScale(), IsClose(Vector3::CreateOne()));
        m1.MultiplyByScale(Vector3(3.0f, 4.0f, 5.0f));
        EXPECT_THAT(m1.RetrieveScale(), IsClose(Vector3(3.0f, 4.0f, 5.0f)));
    }

    TEST(MATH_Matrix3x3, TestScaleSqAccess)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(40.0f)) * Matrix3x3::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        EXPECT_THAT(m1.RetrieveScaleSq(), IsClose(Vector3(4.0f, 9.0f, 16.0f)));
        m1.ExtractScale();
        EXPECT_THAT(m1.RetrieveScaleSq(), IsClose(Vector3::CreateOne()));
        m1.MultiplyByScale(Vector3(3.0f, 4.0f, 5.0f));
        EXPECT_THAT(m1.RetrieveScaleSq(), IsClose(Vector3(9.0f, 16.0f, 25.0f)));
    }

    TEST(MATH_Matrix3x3, TestReciprocalScaled)
    {
        Matrix3x3 orthogonalMatrix = Matrix3x3::CreateRotationX(DegToRad(40.0f));
        EXPECT_THAT(orthogonalMatrix.GetReciprocalScaled(), IsClose(orthogonalMatrix));
        const AZ::Vector3 scale(2.8f, 0.7f, 1.3f);
        AZ::Matrix3x3 scaledMatrix = orthogonalMatrix;
        scaledMatrix.MultiplyByScale(scale);
        AZ::Matrix3x3 reciprocalScaledMatrix = orthogonalMatrix;
        reciprocalScaledMatrix.MultiplyByScale(scale.GetReciprocal());
        EXPECT_THAT(scaledMatrix.GetReciprocalScaled(), IsClose(reciprocalScaledMatrix));
    }

    TEST(MATH_Matrix3x3, TestPolarDecomposition)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        Matrix3x3 m2 = Matrix3x3::CreateScale(Vector3(5.0f, 6.0f, 7.0f));
        Matrix3x3 m3 = m1 * m2;
        EXPECT_THAT(m3.GetPolarDecomposition(), IsClose(m1));
        Matrix3x3 m4, m5;
        m3.GetPolarDecomposition(&m4, &m5);
        EXPECT_THAT((m4 * m5), IsClose(m3));
        EXPECT_THAT(m4, IsClose(m1));
        EXPECT_THAT(m5, IsCloseTolerance(m2, 0.01f));
    }

    TEST(MATH_Matrix3x3, TestOrthogonalize)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(AZ::DegToRad(30.0f)) * Matrix3x3::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        m1.SetElement(0, 1, 0.2f);
        Matrix3x3 m2 = m1.GetOrthogonalized();
        EXPECT_NEAR(m2.GetRow(0).GetLength(), 1.0f, AZ::Constants::Tolerance);
        EXPECT_NEAR(m2.GetRow(1).GetLength(), 1.0f, AZ::Constants::Tolerance);
        EXPECT_NEAR(m2.GetRow(2).GetLength(), 1.0f, AZ::Constants::Tolerance);
        EXPECT_NEAR(m2.GetRow(0).Dot(m2.GetRow(1)), 0.0f, AZ::Constants::Tolerance);
        EXPECT_NEAR(m2.GetRow(0).Dot(m2.GetRow(2)), 0.0f, AZ::Constants::Tolerance);
        EXPECT_NEAR(m2.GetRow(1).Dot(m2.GetRow(2)), 0.0f, AZ::Constants::Tolerance);
        EXPECT_THAT(m2.GetRow(0).Cross(m2.GetRow(1)), IsClose(m2.GetRow(2)));
        EXPECT_THAT(m2.GetRow(1).Cross(m2.GetRow(2)), IsClose(m2.GetRow(0)));
        EXPECT_THAT(m2.GetRow(2).Cross(m2.GetRow(0)), IsClose(m2.GetRow(1)));
        m1.Orthogonalize();
        EXPECT_THAT(m1, IsClose(m2));
    }

    TEST(MATH_Matrix3x3, TestOrthogonal)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(AZ::DegToRad(30.0f));
        EXPECT_TRUE(m1.IsOrthogonal(0.05f));
        m1.SetRow(1, m1.GetRow(1) * 2.0f);
        EXPECT_FALSE(m1.IsOrthogonal(0.05f));
        m1 = Matrix3x3::CreateRotationX(AZ::DegToRad(30.0f));
        m1.SetRow(1, Vector3(0.0f, 1.0f, 0.0f));
        EXPECT_FALSE(m1.IsOrthogonal(0.05f));
    }

    TEST(MATH_Matrix3x3, TestIsClose)
    {
        Matrix3x3 m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        Matrix3x3 m2 = m1;
        EXPECT_TRUE(m1.IsClose(m2));
        m2.SetElement(0, 0, 2.0f);
        EXPECT_FALSE(m1.IsClose(m2));
        m2 = m1;
        m2.SetElement(0, 2, 2.0f);
        EXPECT_FALSE(m1.IsClose(m2));
    }

    TEST(MATH_Matrix3x3, TestGetDiagonal)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        EXPECT_THAT(m1.GetDiagonal(), IsCloseTolerance(Vector3(1.0f, 5.0f, 9.0f), 1e-6f));
    }

    TEST(MATH_Matrix3x3, TestDeterminant)
    {
        Matrix3x3 m1;
        m1.SetRow(0, -1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, -9.0f);
        EXPECT_NEAR(m1.GetDeterminant(), 240.0f, AZ::Constants::Tolerance);
    }

    TEST(MATH_Matrix3x3, TestAdjugate)
    {
        Matrix3x3 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2 = m1.GetAdjugate();
        EXPECT_THAT(m2.GetRow(0), IsClose(Vector3(-3.0f, 6.0f, -3.0f)));
        EXPECT_THAT(m2.GetRow(1), IsClose(Vector3(6.0f, -12.0f, 6.0f)));
        EXPECT_THAT(m2.GetRow(2), IsClose(Vector3(-3.0f, 6.0f, -3.0f)));
    }
}
