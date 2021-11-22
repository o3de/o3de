/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/VectorConversions.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include "MathTestData.h"

namespace UnitTest
{
    using Matrix3x4CreateFixture = ::testing::TestWithParam<AZ::Vector3>;

    TEST_P(Matrix3x4CreateFixture, CreateIdentity)
    {
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateIdentity();
        const AZ::Vector3 vector = GetParam();
        EXPECT_THAT(matrix * vector, IsClose(vector));
    }

    TEST_P(Matrix3x4CreateFixture, Identity)
    {
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::Identity();
        const AZ::Vector3 vector = GetParam();
        EXPECT_THAT(matrix * vector, IsClose(vector));
    }

    TEST_P(Matrix3x4CreateFixture, CreateZero)
    {
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateZero();
        const AZ::Vector3 vector = GetParam();
        EXPECT_THAT(matrix * vector, IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_P(Matrix3x4CreateFixture, CreateTranslation)
    {
        const AZ::Vector3 translation(0.8f, 2.3f, -1.9f);
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateTranslation(translation);
        const AZ::Vector3 vector = GetParam();
        EXPECT_THAT(matrix * vector, IsClose(vector + translation));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4CreateFixture, ::testing::ValuesIn(MathTestData::Vector3s));

    TEST(MATH_Matrix3x4, CreateFromValue)
    {
        const float value = 2.3f;
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromValue(value);
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                EXPECT_NEAR(matrix.GetElement(row, col), value, 1e-3f);
            }
        }
    }

    TEST(MATH_Matrix3x4, CreateFromRowMajorFloat12)
    {
        const float values[12] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f };
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromRowMajorFloat12(values);
        float storedValues[12];
        matrix.StoreToRowMajorFloat12(storedValues);
        EXPECT_THAT(storedValues, ::testing::Pointwise(::testing::FloatNear(1e-5f), values));
        EXPECT_THAT(matrix.GetColumn(0), IsClose(AZ::Vector3(1.0f, 5.0f, 9.0f)));
        EXPECT_THAT(matrix.GetColumn(1), IsClose(AZ::Vector3(2.0f, 6.0f, 10.0f)));
        EXPECT_THAT(matrix.GetColumn(2), IsClose(AZ::Vector3(3.0f, 7.0f, 11.0f)));
        EXPECT_THAT(matrix.GetColumn(3), IsClose(AZ::Vector3(4.0f, 8.0f, 12.0f)));
    }

    TEST(MATH_Matrix3x4, CreateFromColumnMajorFloat12)
    {
        const float values[12] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f };
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromColumnMajorFloat12(values);
        float storedValues[12];
        matrix.StoreToColumnMajorFloat12(storedValues);
        EXPECT_THAT(storedValues, ::testing::Pointwise(::testing::FloatNear(1e-5f), values));
        EXPECT_THAT(matrix.GetColumn(0), IsClose(AZ::Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_THAT(matrix.GetColumn(1), IsClose(AZ::Vector3(4.0f, 5.0f, 6.0f)));
        EXPECT_THAT(matrix.GetColumn(2), IsClose(AZ::Vector3(7.0f, 8.0f, 9.0f)));
        EXPECT_THAT(matrix.GetColumn(3), IsClose(AZ::Vector3(10.0f, 11.0f, 12.0f)));
    }

    TEST(MATH_Matrix3x4, CreateFromColumnMajorFloat16)
    {
        const float values[16] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromColumnMajorFloat16(values);
        float storedValues[16];
        matrix.StoreToColumnMajorFloat16(storedValues);
        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                EXPECT_THAT(storedValues[4 * col + row], ::testing::FloatNear(values[4 * col + row], 1e-5f));
            }
        }
        EXPECT_THAT(matrix.GetColumn(0), IsClose(AZ::Vector3(1.0f, 2.0f, 3.0f)));
        EXPECT_THAT(matrix.GetColumn(1), IsClose(AZ::Vector3(5.0f, 6.0f, 7.0f)));
        EXPECT_THAT(matrix.GetColumn(2), IsClose(AZ::Vector3(9.0f, 10.0f, 11.0f)));
        EXPECT_THAT(matrix.GetColumn(3), IsClose(AZ::Vector3(13.0f, 14.0f, 15.0f)));
    }

    using Matrix3x4CreateRotationFixture = ::testing::TestWithParam<float>;

    TEST_P(Matrix3x4CreateRotationFixture, CreateRotationX)
    {
        const float angle = GetParam();
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateRotationX(angle);
        EXPECT_TRUE(matrix.IsOrthogonal());
        const AZ::Vector3 vector(1.5f, -0.2f, 2.7f);
        const AZ::Vector3 rotatedVector = matrix.TransformVector(vector);
        // rotating a vector should not affect its length
        EXPECT_TRUE(AZ::IsClose(rotatedVector.GetLengthSq(), vector.GetLengthSq()));

        // rotating about the X axis should not affect the X component
        EXPECT_NEAR(rotatedVector.GetX(), vector.GetX(), AZ::Constants::Tolerance);

        // when projected into the Y-Z plane, the angle between the rotated vector and the original vector
        // should wrap to the same as the input angle parameter
        const float xSquared = vector.GetX() * vector.GetX();
        const float projectedDotProduct = rotatedVector.Dot(vector) - xSquared;
        const float projectedMagnitudeSq = vector.Dot(vector) - xSquared;
        EXPECT_NEAR(projectedDotProduct, projectedMagnitudeSq * cosf(angle), 1e-3f);
    }

    TEST_P(Matrix3x4CreateRotationFixture, CreateRotationY)
    {
        const float angle = GetParam();
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateRotationY(angle);
        EXPECT_TRUE(matrix.IsOrthogonal());
        const AZ::Vector3 vector(1.5f, -0.2f, 2.7f);
        const AZ::Vector3 rotatedVector = matrix.TransformVector(vector);
        // rotating a vector should not affect its length
        EXPECT_TRUE(AZ::IsClose(rotatedVector.GetLengthSq(), vector.GetLengthSq()));

        // rotating about the Y axis should not affect the Y component
        EXPECT_NEAR(rotatedVector.GetY(), vector.GetY(), AZ::Constants::Tolerance);

        // when projected into the X-Z plane, the angle between the rotated vector and the original vector
        // should wrap to the same as the input angle parameter
        const float ySquared = vector.GetY() * vector.GetY();
        const float projectedDotProduct = rotatedVector.Dot(vector) - ySquared;
        const float projectedMagnitudeSq = vector.Dot(vector) - ySquared;
        EXPECT_NEAR(projectedDotProduct, projectedMagnitudeSq * cosf(angle), 1e-3f);
    }

    TEST_P(Matrix3x4CreateRotationFixture, CreateRotationZ)
    {
        const float angle = GetParam();
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateRotationZ(angle);
        EXPECT_TRUE(matrix.IsOrthogonal());
        const AZ::Vector3 vector(1.5f, -0.2f, 2.7f);
        const AZ::Vector3 rotatedVector = matrix.TransformVector(vector);
        // rotating a vector should not affect its length
        EXPECT_TRUE(AZ::IsClose(rotatedVector.GetLengthSq(), vector.GetLengthSq()));

        // rotating about the Z axis should not affect the Z component
        EXPECT_NEAR(rotatedVector.GetZ(), vector.GetZ(), AZ::Constants::Tolerance);

        // when projected into the X-Y plane, the angle between the rotated vector and the original vector
        // should wrap to the same as the input angle parameter
        const float zSquared = vector.GetZ() * vector.GetZ();
        const float projectedDotProduct = rotatedVector.Dot(vector) - zSquared;
        const float projectedMagnitudeSq = vector.Dot(vector) - zSquared;
        EXPECT_NEAR(projectedDotProduct, projectedMagnitudeSq * cosf(angle), 1e-3f);
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4CreateRotationFixture, ::testing::ValuesIn(MathTestData::Angles));

    TEST(MATH_Matrix3x4, CreateFromRows)
    {
        const AZ::Vector4 row0(1.488f, 2.56f, 0.096f, 2.3f);
        const AZ::Vector4 row1(0.384f, -1.92f, 0.428f, -1.6f);
        const AZ::Vector4 row2(1.28f, -2.4f, -0.24f, 3.7f);
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromRows(row0, row1, row2);
        const AZ::Vector3 vector(0.2f, 0.1f, -0.3f);
        const AZ::Vector3 transformedVector = matrix * vector;
        const AZ::Vector3 expected(2.8248f, -1.8436f, 3.788f);
        EXPECT_THAT(transformedVector, IsClose(expected));
    }

    TEST(MATH_Matrix3x4, GetSetRows)
    {
        const AZ::Vector4 row0(1.488f, 2.56f, 0.096f, 2.3f);
        const AZ::Vector4 row1(0.384f, -1.92f, 0.428f, -1.6f);
        const AZ::Vector4 row2(1.28f, -2.4f, -0.24f, 3.7f);
        AZ::Matrix3x4 matrix;
        matrix.SetRows(row0, row1, row2);
        AZ::Vector4 rows[3];
        matrix.GetRows(&rows[0], &rows[1], &rows[2]);
        EXPECT_THAT(matrix.GetRow(0), IsClose(rows[0]));
        EXPECT_THAT(matrix.GetRow(1), IsClose(rows[1]));
        EXPECT_THAT(matrix.GetRow(2), IsClose(rows[2]));
        const AZ::Vector3 vector(0.2f, 0.1f, -0.3f);
        const AZ::Vector3 transformedVector = matrix * vector;
        const AZ::Vector3 expected(2.8248f, -1.8436f, 3.788f);
        EXPECT_THAT(transformedVector, IsClose(expected));
    }

    TEST(MATH_Matrix3x4, GetSetRow)
    {
        AZ::Matrix3x4 matrix;
        const AZ::Vector4 row0(1.488f, 2.56f, 0.096f, 2.3f);
        matrix.SetRow(0, row0);
        const AZ::Vector3 row1(0.384f, -1.92f, 0.428f);
        const float w1 = -1.6f;
        matrix.SetRow(1, row1, w1);
        const float x2 = 1.28f;
        const float y2 = -2.4f;
        const float z2 = -0.24f;
        const float w2 = 3.7f;
        matrix.SetRow(2, x2, y2, z2, w2);
        EXPECT_THAT(matrix.GetRow(0), IsClose(row0));
        EXPECT_THAT(matrix.GetRow(2), IsClose(AZ::Vector4(x2, y2, z2, w2)));
        EXPECT_THAT(matrix.GetRowAsVector3(0), IsClose(row0.GetAsVector3()));
        EXPECT_THAT(matrix.GetRowAsVector3(1), IsClose(row1));
        EXPECT_THAT(matrix.GetRowAsVector3(2), IsClose(AZ::Vector3(x2, y2, z2)));
        const AZ::Vector3 vector(0.2f, 0.1f, -0.3f);
        const AZ::Vector3 transformedVector = matrix * vector;
        const AZ::Vector3 expected(2.8248f, -1.8436f, 3.788f);
        EXPECT_THAT(transformedVector, IsClose(expected));
    }

    TEST(MATH_Matrix3x4, CreateFromColumns)
    {
        const AZ::Vector3 col0(1.488f, 0.384f, 1.28f);
        const AZ::Vector3 col1(2.56f, -1.92f, -2.4f);
        const AZ::Vector3 col2(0.096f, 0.428f, -0.24f);
        const AZ::Vector3 col3(2.3f, -1.6f, 3.7f);
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromColumns(col0, col1, col2, col3);
        const AZ::Vector3 vector(0.2f, 0.1f, -0.3f);
        const AZ::Vector3 transformedVector = matrix * vector;
        const AZ::Vector3 expected(2.8248f, -1.8436f, 3.788f);
        EXPECT_THAT(transformedVector, IsClose(expected));
    }

    TEST(MATH_Matrix3x4, GetSetColumns)
    {
        const AZ::Vector3 inputCol0(2.2f, -0.4f, -1.2f);
        const AZ::Vector3 inputCol1(-0.3f, 1.2f, 0.2f);
        const AZ::Vector3 inputCol2(0.6f, -0.2f, 1.7f);
        const AZ::Vector3 inputCol3(3.2f, 1.7f, -0.9f);
        AZ::Matrix3x4 matrix;
        matrix.SetColumns(inputCol0, inputCol1, inputCol2, inputCol3);
        AZ::Vector3 col0, col1, col2, col3;
        matrix.GetColumns(&col0, &col1, &col2, &col3);
        EXPECT_THAT(col0, IsClose(inputCol0));
        EXPECT_THAT(col1, IsClose(inputCol1));
        EXPECT_THAT(col2, IsClose(inputCol2));
        EXPECT_THAT(col3, IsClose(inputCol3));
    }

    TEST(MATH_Matrix3x4, GetSetColumn)
    {
        const AZ::Vector3 inputCol0(1.3f, 1.4f, -0.2f);
        const float x = -0.7f;
        const float y = 1.2f;
        const float z = -0.4f;
        AZ::Matrix3x4 matrix;
        matrix.SetColumn(0, inputCol0);
        matrix.SetColumn(1, x, y, z);
        EXPECT_THAT(matrix.GetColumn(0), IsClose(inputCol0));
        EXPECT_THAT(matrix.GetColumn(1), IsClose(AZ::Vector3(x, y, z)));
    }

    TEST(MATH_Matrix3x4, GetSetBasisAndTranslation)
    {
        const AZ::Vector3 inputBasisX(-1.9f, -0.2f, 2.3f);
        const AZ::Vector3 inputBasisY(1.4f, 0.1f, -1.9f);
        const AZ::Vector3 inputBasisZ(2.1f, 0.6f, 1.1f);
        const AZ::Vector3 inputTranslation(-0.4f, -0.9f, -1.3f);
        AZ::Matrix3x4 matrix;
        matrix.SetBasisAndTranslation(inputBasisX, inputBasisY, inputBasisZ, inputTranslation);
        AZ::Vector3 basisX, basisY, basisZ, translation;
        matrix.GetBasisAndTranslation(&basisX, &basisY, &basisZ, &translation);
        EXPECT_THAT(basisX, IsClose(inputBasisX));
        EXPECT_THAT(basisY, IsClose(inputBasisY));
        EXPECT_THAT(basisZ, IsClose(inputBasisZ));
        EXPECT_THAT(translation, IsClose(inputTranslation));
    }

    TEST(MATH_Matrix3x4, GetSetBasisX)
    {
        const AZ::Vector3 inputBasisX(1.6f, 1.3f, -0.8f);
        const float x = 0.2f;
        const float y = 1.3f;
        const float z = -3.4f;
        AZ::Matrix3x4 matrix;
        matrix.SetBasisX(inputBasisX);
        EXPECT_THAT(matrix.GetBasisX(), IsClose(inputBasisX));
        matrix.SetBasisX(x, y, z);
        EXPECT_THAT(matrix.GetBasisX(), IsClose(AZ::Vector3(x, y, z)));
    }

    TEST(MATH_Matrix3x4, GetSetBasisY)
    {
        const AZ::Vector3 inputBasisY(-0.7f, 0.9f, 2.7f);
        const float x = -0.5f;
        const float y = -0.3f;
        const float z = -0.6f;
        AZ::Matrix3x4 matrix;
        matrix.SetBasisY(inputBasisY);
        EXPECT_THAT(matrix.GetBasisY(), IsClose(inputBasisY));
        matrix.SetBasisY(x, y, z);
        EXPECT_THAT(matrix.GetBasisY(), IsClose(AZ::Vector3(x, y, z)));
    }

    TEST(MATH_Matrix3x4, GetSetBasisZ)
    {
        const AZ::Vector3 inputBasisZ(2.8f, 0.9f, -2.9f);
        const float x = 0.1f;
        const float y = 0.6f;
        const float z = 0.9f;
        AZ::Matrix3x4 matrix;
        matrix.SetBasisZ(inputBasisZ);
        EXPECT_THAT(matrix.GetBasisZ(), IsClose(inputBasisZ));
        matrix.SetBasisZ(x, y, z);
        EXPECT_THAT(matrix.GetBasisZ(), IsClose(AZ::Vector3(x, y, z)));
    }

    TEST(MATH_Matrix3x4, GetSetTranslation)
    {
        const AZ::Vector3 inputTranslation(-1.2f, -0.2f, 1.9f);
        const float x = -2.7f;
        const float y = -0.7f;
        const float z = -1.2f;
        AZ::Matrix3x4 matrix;
        matrix.SetTranslation(inputTranslation);
        EXPECT_THAT(matrix.GetTranslation(), IsClose(inputTranslation));
        matrix.SetTranslation(x, y, z);
        EXPECT_THAT(matrix.GetTranslation(), IsClose(AZ::Vector3(x, y, z)));
    }

    using Matrix3x4CreateFromQuaternionFixture = ::testing::TestWithParam<AZ::Quaternion>;

    TEST_P(Matrix3x4CreateFromQuaternionFixture, CreateFromQuaternion)
    {
        const AZ::Quaternion quaternion = GetParam();
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromQuaternion(quaternion);
        EXPECT_THAT(matrix.GetTranslation(), IsClose(AZ::Vector3::CreateZero()));
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(matrix * vector, IsClose(quaternion.TransformVector(vector)));
    }

    TEST_P(Matrix3x4CreateFromQuaternionFixture, CreateFromQuaternionAndTranslation)
    {
        const AZ::Quaternion quaternion = GetParam();
        const AZ::Vector3 translation(-2.6f, 1.7f, 0.8f);
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(quaternion, translation);
        EXPECT_THAT(matrix.GetTranslation(), IsClose(translation));
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(matrix * vector, IsClose(quaternion.TransformVector(vector) + translation));
    }

    TEST_P(Matrix3x4CreateFromQuaternionFixture, SetRotationPartFromQuaternion)
    {
        const AZ::Quaternion quaternion = GetParam();
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateIdentity();
        matrix.SetRotationPartFromQuaternion(quaternion);
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(matrix * vector, IsClose(quaternion.TransformVector(vector)));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4CreateFromQuaternionFixture, ::testing::ValuesIn(MathTestData::UnitQuaternions));

    using Matrix3x4CreateFromMatrix3x3Fixture = ::testing::TestWithParam<AZ::Matrix3x3>;

    TEST_P(Matrix3x4CreateFromMatrix3x3Fixture, CreateFromMatrix3x3)
    {
        const AZ::Matrix3x3 matrix3x3 = GetParam();
        const AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromMatrix3x3(matrix3x3);
        EXPECT_THAT(matrix3x4.GetTranslation(), IsClose(AZ::Vector3::CreateZero()));
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(matrix3x4.TransformVector(vector), IsClose(matrix3x3 * vector));
    }

    TEST_P(Matrix3x4CreateFromMatrix3x3Fixture, CreateFromMatrix3x3AndTranslation)
    {
        const AZ::Matrix3x3 matrix3x3 = GetParam();
        const AZ::Vector3 translation(-2.6f, 1.7f, 0.8f);
        const AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(matrix3x3, translation);
        EXPECT_THAT(matrix3x4.GetTranslation(), IsClose(translation));
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(matrix3x4 * vector, IsClose(matrix3x3 * vector + translation));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4CreateFromMatrix3x3Fixture, ::testing::ValuesIn(MathTestData::Matrix3x3s));

    using Matrix3x4CreateFromMatrix4x4Fixture = ::testing::TestWithParam<AZ::Matrix4x4>;

    TEST_P(Matrix3x4CreateFromMatrix4x4Fixture, UnsafeCreateFromMatrix4x4)
    {
        const AZ::Matrix4x4 matrix4x4 = GetParam();
        const AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::UnsafeCreateFromMatrix4x4(matrix4x4);
        EXPECT_THAT(matrix3x4.GetTranslation(), IsClose(matrix4x4.GetTranslation()));
        const AZ::Vector3 vector(2.3f, -0.6, 1.8f);
        EXPECT_THAT(matrix3x4.TransformVector(vector), IsClose((matrix4x4 * AZ::Vector3ToVector4(vector, 0.0f)).GetAsVector3()));
        const AZ::Vector3 point(12.3f, -5.6, 7.3f);
        EXPECT_THAT(matrix3x4.TransformPoint(point), IsClose((matrix4x4 * AZ::Vector3ToVector4(point, 1.0f)).GetAsVector3()));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4CreateFromMatrix4x4Fixture, ::testing::ValuesIn(MathTestData::Matrix4x4s));

    TEST(MATH_Matrix3x4, TransformPoint)
    {
        const AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateRotationY(AZ::DegToRad(90.0f)), AZ::Vector3(5.0f, 0.0f, 0.0f));

        const AZ::Vector3 result = matrix3x4.TransformPoint(AZ::Vector3(1.0f, 0.0f, 0.0f));
        const AZ::Vector3 expected = AZ::Vector3(5.0f, 0.0f, -1.0f);

        EXPECT_THAT(result, expected);
    }

    TEST(MATH_Matrix3x4, CreateScale)
    {
        const AZ::Vector3 scale(1.7f, 0.3f, 2.4f);
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateScale(scale);
        const AZ::Vector3 vector(0.2f, -1.6f, 0.4f);
        EXPECT_THAT(matrix.GetTranslation(), IsClose(AZ::Vector3::CreateZero()));
        const AZ::Vector3 transformedVector = matrix * vector;
        const AZ::Vector3 expected(0.34f, -0.48f, 0.96f);
        EXPECT_THAT(transformedVector, IsClose(expected));
    }

    TEST(MATH_Matrix3x4, CreateDiagonal)
    {
        const AZ::Vector3 diagonal(0.6f, -1.4f, -0.7f);
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateDiagonal(diagonal);
        const AZ::Vector3 vector(-0.3f, -0.6f, 0.2f);
        EXPECT_THAT(matrix.GetTranslation(), IsClose(AZ::Vector3::CreateZero()));
        const AZ::Vector3 transformedVector = matrix * vector;
        const AZ::Vector3 expected(-0.18f, 0.84f, -0.14f);
        EXPECT_THAT(transformedVector, IsClose(expected));
    }

    TEST(MATH_Matrix3x4, GetSetElement)
    {
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateIdentity();
        EXPECT_NEAR(matrix(1, 1), 1.0f, 1e-3f);
        matrix.SetElement(1, 1, -2.3f);
        EXPECT_NEAR(matrix(1, 1), -2.3f, 1e-3f);
    }

    using Matrix3x4CreateLookAtFixture = ::testing::TestWithParam<MathTestData::AxisPair>;

    TEST_P(Matrix3x4CreateLookAtFixture, CreateLookAt)
    {
        const AZ::Matrix3x4::Axis axis = GetParam().first;
        const AZ::Vector3 axisDirection = GetParam().second;
        const AZ::Vector3 from(2.5f, 0.2f, 3.6f);
        const AZ::Vector3 to(1.3f, 0.5f, 3.2f);
        const AZ::Vector3 expectedForward = to - from;
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateLookAt(from, to, axis);
        EXPECT_TRUE(matrix.IsOrthogonal());
        EXPECT_THAT(matrix.GetColumn(0).Cross(matrix.GetColumn(1)), IsClose(matrix.GetColumn(2)));
        EXPECT_THAT(matrix.GetTranslation(), IsClose(from));
        // the column of the matrix corresponding to the axis direction should be parallel to the expected forward direction
        const AZ::Vector3 forward = matrix.Multiply3x3(axisDirection);
        EXPECT_THAT(forward, IsClose(expectedForward.GetNormalized()));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4CreateLookAtFixture, ::testing::ValuesIn(MathTestData::Axes));

    TEST(MATH_Matrix3x4, CreateLookAtDegenerateCases)
    {
        const AZ::Vector3 from(2.5f, 0.2f, 3.6f);
        // to and from are the same, should generate an error
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(AZ::Matrix3x4::CreateLookAt(from, from).IsClose(AZ::Matrix3x4::Identity()));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // to - from is parallel to usual up direction
        const AZ::Vector3 to(2.5f, 0.2f, 5.2f);
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateLookAt(from, to);
        EXPECT_TRUE(matrix.IsOrthogonal());
        EXPECT_THAT(matrix.GetTranslation(), IsClose(from));

        // the default is for the Y basis of the look at matrix to be the forward direction
        const AZ::Vector3 axisDirection = AZ::Vector3::CreateAxisY();
        const AZ::Vector3 forwardDirection = (to - from).GetNormalized();

        EXPECT_THAT(matrix.Multiply3x3(axisDirection), IsClose(forwardDirection));
    }

    TEST(MATH_Matrix3x4, TestMatrixMultiplication)
    {
        AZ::Matrix3x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        AZ::Matrix3x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);
        AZ::Matrix3x4 m3 = m1 * m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(74.0f, 80.0f, 86.0f, 96.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(206.0f, 224.0f, 242.0f, 268.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(338.0f, 368.0f, 398.0f, 440.0f)));
        AZ::Matrix3x4 m4 = m1;
        m4 *= m2;
        EXPECT_THAT(m4.GetRow(0), IsClose(AZ::Vector4(74.0f, 80.0f, 86.0f, 96.0f)));
        EXPECT_THAT(m4.GetRow(1), IsClose(AZ::Vector4(206.0f, 224.0f, 242.0f, 268.0f)));
        EXPECT_THAT(m4.GetRow(2), IsClose(AZ::Vector4(338.0f, 368.0f, 398.0f, 440.0f)));
    }

    TEST(MATH_Matrix3x4, TestSum)
    {
        AZ::Matrix3x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        AZ::Matrix3x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);

        AZ::Matrix3x4 m3 = m1 + m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(8.0f, 10.0f, 12.0f, 14.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(16.0f, 18.0f, 20.0f, 22.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(24.0f, 26.0f, 28.0f, 30.0f)));

        m3 = m1;
        m3 += m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(8.0f, 10.0f, 12.0f, 14.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(16.0f, 18.0f, 20.0f, 22.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(24.0f, 26.0f, 28.0f, 30.0f)));
    }

    TEST(MATH_Matrix3x4, TestDifference)
    {
        AZ::Matrix3x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        AZ::Matrix3x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);

        AZ::Matrix3x4 m3 = m1 - m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        m3 = m1;
        m3 -= m2;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(-6.0f, -6.0f, -6.0f, -6.0f)));
    }

    TEST(MATH_Matrix3x4, TestScalarMultiplication)
    {
        AZ::Matrix3x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        AZ::Matrix3x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);

        AZ::Matrix3x4 m3 = m1 * 2.0f;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
        m3 = m1;
        m3 *= 2.0f;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
        m3 = 2.0f * m1;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
    }

    TEST(MATH_Matrix3x4, TestScalarDivision)
    {
        AZ::Matrix3x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        AZ::Matrix3x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);

        AZ::Matrix3x4 m3 = m1 / 0.5f;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
        m3 = m1;
        m3 /= 0.5f;
        EXPECT_THAT(m3.GetRow(0), IsClose(AZ::Vector4(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_THAT(m3.GetRow(1), IsClose(AZ::Vector4(10.0f, 12.0f, 14.0f, 16.0f)));
        EXPECT_THAT(m3.GetRow(2), IsClose(AZ::Vector4(18.0f, 20.0f, 22.0f, 24.0f)));
    }

    TEST(MATH_Matrix3x4, TestNegation)
    {
        AZ::Matrix3x4 m1;
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        EXPECT_THAT(-(-m1), IsClose(m1));
        EXPECT_THAT(-AZ::Matrix3x4::CreateZero(), IsClose(AZ::Matrix3x4::CreateZero()));

        AZ::Matrix3x4 m2 = -m1;
        EXPECT_THAT(m2.GetRow(0), IsClose(AZ::Vector4(-1.0f, -2.0f, -3.0f, -4.0f)));
        EXPECT_THAT(m2.GetRow(1), IsClose(AZ::Vector4(-5.0f, -6.0f, -7.0f, -8.0f)));
        EXPECT_THAT(m2.GetRow(2), IsClose(AZ::Vector4(-9.0f, -10.0f, -11.0f, -12.0f)));

        AZ::Matrix3x4 m3 = m1 + (-m1);
        EXPECT_THAT(m3, IsClose(AZ::Matrix3x4::CreateZero()));
    }

    TEST(MATH_Matrix3x4, MultiplyByVector3)
    {
        const AZ::Vector4 row0(1.488f, 2.56f, 0.096f, 2.3f);
        const AZ::Vector4 row1(0.384f, -1.92f, 0.428f, -1.6f);
        const AZ::Vector4 row2(1.28f, -2.4f, -0.24f, 3.7f);
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromRows(row0, row1, row2);
        const AZ::Vector3 vector(0.2f, 0.1f, -0.3f);
        const AZ::Vector3 expected(2.8248f, -1.8436f, 3.788f);
        EXPECT_THAT(matrix * vector, IsClose(expected));
    }

    TEST(MATH_Matrix3x4, Multiply3x3)
    {
        const AZ::Vector4 row0(1.488f, 2.56f, 0.096f, 2.3f);
        const AZ::Vector4 row1(0.384f, -1.92f, 0.428f, -1.6f);
        const AZ::Vector4 row2(1.28f, -2.4f, -0.24f, 3.7f);
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromRows(row0, row1, row2);
        const AZ::Vector3 vector(0.2f, 0.1f, -0.3f);
        const AZ::Vector3 expected(0.5248f, -0.2436f, 0.088f);
        EXPECT_THAT(matrix.Multiply3x3(vector), IsClose(expected));
        matrix.SetTranslation(AZ::Vector3(0.9f, 2.6f, -2.2f));
        EXPECT_THAT(matrix.Multiply3x3(vector), IsClose(expected));
    }

    TEST(MATH_Matrix3x4, MultiplyByVector4)
    {
        const AZ::Vector4 row0(-0.4f, 0.5f, 0.4f, -0.5f);
        const AZ::Vector4 row1(0.9f, -1.0f, 0.6f, 0.4f);
        const AZ::Vector4 row2(0.4f, 0.3f, 0.3f, -0.8f);
        const AZ::Vector4 vector(0.4f, -1.0f, -0.2f, 0.3f);
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromRows(row0, row1, row2);
        const AZ::Vector4 product = matrix * vector;
        const AZ::Vector4 expected(-0.89f, 1.36f, -0.44f, 0.3f);
        EXPECT_THAT(product, IsClose(expected));
    }

    using Matrix3x4TransposeFixture = ::testing::TestWithParam<AZ::Matrix3x4>;

    TEST_P(Matrix3x4TransposeFixture, GetTranspose)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        const AZ::Matrix3x4 transpose = matrix.GetTranspose();
        EXPECT_THAT(transpose.GetTranslation(), IsClose(AZ::Vector3::CreateZero()));
        EXPECT_THAT(transpose.GetColumn(0), IsClose(matrix.GetRowAsVector3(0)));
        EXPECT_THAT(transpose.GetColumn(1), IsClose(matrix.GetRowAsVector3(1)));
        EXPECT_THAT(transpose.GetColumn(2), IsClose(matrix.GetRowAsVector3(2)));
    }

    TEST_P(Matrix3x4TransposeFixture, Transpose)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        AZ::Matrix3x4 transpose = matrix;
        transpose.Transpose();
        EXPECT_THAT(transpose.GetTranslation(), IsClose(AZ::Vector3::CreateZero()));
        EXPECT_THAT(transpose.GetColumn(0), IsClose(matrix.GetRowAsVector3(0)));
        EXPECT_THAT(transpose.GetColumn(1), IsClose(matrix.GetRowAsVector3(1)));
        EXPECT_THAT(transpose.GetColumn(2), IsClose(matrix.GetRowAsVector3(2)));
    }

    TEST_P(Matrix3x4TransposeFixture, GetTranspose3x3)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        const AZ::Matrix3x4 transpose = matrix.GetTranspose3x3();
        EXPECT_THAT(transpose.GetTranslation(), IsClose(matrix.GetTranslation()));
        EXPECT_THAT(transpose.GetColumn(0), IsClose(matrix.GetRowAsVector3(0)));
        EXPECT_THAT(transpose.GetColumn(1), IsClose(matrix.GetRowAsVector3(1)));
        EXPECT_THAT(transpose.GetColumn(2), IsClose(matrix.GetRowAsVector3(2)));
    }

    TEST_P(Matrix3x4TransposeFixture, Transpose3x3)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        AZ::Matrix3x4 transpose = matrix;
        transpose.Transpose3x3();
        EXPECT_THAT(transpose.GetTranslation(), IsClose(matrix.GetTranslation()));
        EXPECT_THAT(transpose.GetColumn(0), IsClose(matrix.GetRowAsVector3(0)));
        EXPECT_THAT(transpose.GetColumn(1), IsClose(matrix.GetRowAsVector3(1)));
        EXPECT_THAT(transpose.GetColumn(2), IsClose(matrix.GetRowAsVector3(2)));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4TransposeFixture, ::testing::ValuesIn(MathTestData::NonOrthogonalMatrix3x4s));

    using Matrix3x4InvertFullFixture = ::testing::TestWithParam<AZ::Matrix3x4>;

    TEST_P(Matrix3x4InvertFullFixture, GetInverseFull)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        const AZ::Matrix3x4 inverse = matrix.GetInverseFull();
        const AZ::Vector3 vector(0.9f, 3.2f, -1.4f);
        EXPECT_THAT((inverse * matrix) * vector, IsClose(vector));
        EXPECT_THAT((matrix * inverse) * vector, IsClose(vector));
        EXPECT_TRUE((inverse * matrix).IsClose(AZ::Matrix3x4::Identity()));
    }

    TEST_P(Matrix3x4InvertFullFixture, InvertFull)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        AZ::Matrix3x4 inverse = matrix;
        inverse.InvertFull();
        const AZ::Vector3 vector(2.8f, -1.3f, 2.6f);
        EXPECT_THAT((inverse * matrix) * vector, IsClose(vector));
        EXPECT_THAT((matrix * inverse) * vector, IsClose(vector));
        EXPECT_TRUE((inverse * matrix).IsClose(AZ::Matrix3x4::Identity()));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4InvertFullFixture, ::testing::ValuesIn(MathTestData::NonOrthogonalMatrix3x4s));

#if AZ_TRAIT_DISABLE_FAILED_MATH_TESTS
    TEST(MATH_Matrix3x4, DISABLED_GetInverseFullSingularMatrix)
#else
    TEST(MATH_Matrix3x4, GetInverseFullSingularMatrix)
#endif // AZ_TRAIT_DISABLE_FAILED_MATH_TESTS
    {
        const AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromValue(1.4f);
        const AZ::Matrix3x4 inverse = matrix.GetInverseFull();
        EXPECT_THAT(inverse.GetBasisX(), IsClose(AZ::Vector3::CreateAxisX()));
        EXPECT_THAT(inverse.GetBasisY(), IsClose(AZ::Vector3::CreateAxisY()));
        EXPECT_THAT(inverse.GetBasisZ(), IsClose(AZ::Vector3::CreateAxisZ()));
        EXPECT_THAT(inverse.GetTranslation(), IsClose(AZ::Vector3(-1.4f)));
    }

    using Matrix3x4InvertFastFixture = ::testing::TestWithParam<AZ::Matrix3x4>;

    TEST_P(Matrix3x4InvertFastFixture, GetInverseFast)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        const AZ::Matrix3x4 inverseFast = matrix.GetInverseFast();
        const AZ::Matrix3x4 inverseFull = matrix.GetInverseFull();
        const AZ::Vector3 vector(0.9f, 3.2f, -1.4f);
        EXPECT_THAT((inverseFast * matrix) * vector, IsClose(vector));
        EXPECT_THAT((matrix * inverseFast) * vector, IsClose(vector));
        EXPECT_TRUE((inverseFast * matrix).IsClose(AZ::Matrix3x4::Identity()));
        EXPECT_TRUE(inverseFast.IsClose(inverseFull));
    }

    TEST_P(Matrix3x4InvertFastFixture, InvertFast)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        AZ::Matrix3x4 inverseFast = matrix;
        inverseFast.InvertFast();
        AZ::Matrix3x4 inverseFull = matrix;
        inverseFull.InvertFull();
        const AZ::Vector3 vector(2.8f, -1.3f, 2.6f);
        EXPECT_THAT((inverseFast * matrix) * vector, IsClose(vector));
        EXPECT_THAT((matrix * inverseFast) * vector, IsClose(vector));
        EXPECT_TRUE((inverseFast * matrix).IsClose(AZ::Matrix3x4::Identity()));
        EXPECT_TRUE(inverseFast.IsClose(inverseFull));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4InvertFastFixture, ::testing::ValuesIn(MathTestData::OrthogonalMatrix3x4s));

    using Matrix3x4ScaleFixture = ::testing::TestWithParam<AZ::Matrix3x4>;

    TEST_P(Matrix3x4ScaleFixture, Scale)
    {
        const AZ::Matrix3x4 orthogonalMatrix = GetParam();
        EXPECT_THAT(orthogonalMatrix.RetrieveScale(), IsClose(AZ::Vector3::CreateOne()));
        AZ::Matrix3x4 unscaledMatrix = orthogonalMatrix;
        unscaledMatrix.ExtractScale();
        EXPECT_THAT(unscaledMatrix.RetrieveScale(), IsClose(AZ::Vector3::CreateOne()));
        const AZ::Vector3 scale(2.8f, 0.7f, 1.3f);
        AZ::Matrix3x4 scaledMatrix = orthogonalMatrix;
        scaledMatrix.MultiplyByScale(scale);
        EXPECT_THAT(scaledMatrix.RetrieveScale(), IsClose(scale));
        scaledMatrix.ExtractScale();
        EXPECT_THAT(scaledMatrix.RetrieveScale(), IsClose(AZ::Vector3::CreateOne()));
    }

    TEST_P(Matrix3x4ScaleFixture, ScaleSq)
    {
        const AZ::Matrix3x4 orthogonalMatrix = GetParam();
        EXPECT_THAT(orthogonalMatrix.RetrieveScaleSq(), IsClose(AZ::Vector3::CreateOne()));
        AZ::Matrix3x4 unscaledMatrix = orthogonalMatrix;
        unscaledMatrix.ExtractScale();
        EXPECT_THAT(unscaledMatrix.RetrieveScaleSq(), IsClose(AZ::Vector3::CreateOne()));
        const AZ::Vector3 scale(2.8f, 0.7f, 1.3f);
        AZ::Matrix3x4 scaledMatrix = orthogonalMatrix;
        scaledMatrix.MultiplyByScale(scale);
        EXPECT_THAT(scaledMatrix.RetrieveScaleSq(), IsClose(scale * scale));
        EXPECT_THAT(scaledMatrix.RetrieveScaleSq(), IsClose(scaledMatrix.RetrieveScale() * scaledMatrix.RetrieveScale()));
        scaledMatrix.ExtractScale();
        EXPECT_THAT(scaledMatrix.RetrieveScaleSq(), IsClose(AZ::Vector3::CreateOne()));
    }

    TEST_P(Matrix3x4ScaleFixture, GetReciprocalScaled)
    {
        const AZ::Matrix3x4 orthogonalMatrix = GetParam();
        EXPECT_THAT(orthogonalMatrix.GetReciprocalScaled(), IsClose(orthogonalMatrix));
        const AZ::Vector3 scale(2.8f, 0.7f, 1.3f);
        AZ::Matrix3x4 scaledMatrix = orthogonalMatrix;
        scaledMatrix.MultiplyByScale(scale);
        AZ::Matrix3x4 reciprocalScaledMatrix = orthogonalMatrix;
        reciprocalScaledMatrix.MultiplyByScale(scale.GetReciprocal());
        EXPECT_THAT(scaledMatrix.GetReciprocalScaled(), IsClose(reciprocalScaledMatrix));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4ScaleFixture, ::testing::ValuesIn(MathTestData::OrthogonalMatrix3x4s));

    TEST(MATH_Matrix3x4, IsOrthogonal)
    {
        EXPECT_TRUE(AZ::Matrix3x4::CreateIdentity().IsOrthogonal());
        EXPECT_TRUE(AZ::Matrix3x4::CreateRotationZ(0.3f).IsOrthogonal());
        EXPECT_FALSE(AZ::Matrix3x4::CreateFromValue(1.0f).IsOrthogonal());
        EXPECT_FALSE(AZ::Matrix3x4::CreateDiagonal(AZ::Vector3(0.8f, 0.3f, 1.2f)).IsOrthogonal());
        EXPECT_TRUE(AZ::Matrix3x4::CreateFromQuaternion(AZ::Quaternion(-0.52f, -0.08f, 0.56f, 0.64f)).IsOrthogonal());
        AZ::Matrix3x4 matrix3x4;
        matrix3x4.SetFromEulerRadians(AZ::Vector3(0.2f, 0.4f, 0.1f));
        EXPECT_TRUE(matrix3x4.IsOrthogonal());

        // want to test each possible way the matrix could fail to be orthogonal, which we can do by testing for one
        // axis, then using a rotation which cycles the axes
        const AZ::Matrix3x4 axisCycle = AZ::Matrix3x4::CreateFromQuaternion(AZ::Quaternion(0.5f, 0.5f, 0.5f, 0.5f));

        // a matrix which is normalized in 2 axes, but not the third
        AZ::Matrix3x4 nonOrthogonalMatrix1 = AZ::Matrix3x4::CreateDiagonal(AZ::Vector3(1.0f, 1.0f, 2.0f));

        // a matrix which is normalized in all 3 axes, and 2 pairs of axes are perpendicular, but not the third pair
        AZ::Matrix3x4 nonOrthogonalMatrix2 = AZ::Matrix3x4::Identity();
        nonOrthogonalMatrix2.SetRow(2, AZ::Vector3(0.0f, 0.8f, 0.6f), 0.0f);

        for (int i = 0; i < 3; i++)
        {
            EXPECT_FALSE(nonOrthogonalMatrix1.IsOrthogonal());
            EXPECT_FALSE(nonOrthogonalMatrix2.IsOrthogonal());
            nonOrthogonalMatrix1 = axisCycle * nonOrthogonalMatrix1;
            nonOrthogonalMatrix2 = axisCycle * nonOrthogonalMatrix2;
        }
    }

    TEST(MATH_Matrix3x4, GetOrthogonalized)
    {
        // a matrix which is already orthogonal should be unchanged
        const AZ::Matrix3x4 orthogonalMatrix = AZ::Matrix3x4::CreateRotationZ(0.7f);
        EXPECT_TRUE(orthogonalMatrix.IsOrthogonal());
        EXPECT_TRUE(orthogonalMatrix.GetOrthogonalized().IsClose(orthogonalMatrix));

        // a matrix which isn't already orthogonal should be made orthogonal
        const AZ::Matrix3x4 nonOrthogonalMatrix = AZ::Matrix3x4::CreateScale(AZ::Vector3(3.0f, 4.0f, 5.0f));
        EXPECT_FALSE(nonOrthogonalMatrix.IsOrthogonal());
        EXPECT_TRUE(nonOrthogonalMatrix.GetOrthogonalized().IsOrthogonal());
    }

    TEST(MATH_Matrix3x4, Orthogonalize)
    {
        // a matrix which is already orthogonal should be unchanged
        AZ::Matrix3x4 orthogonalMatrix = AZ::Matrix3x4::CreateRotationY(-0.2f);
        EXPECT_TRUE(orthogonalMatrix.IsOrthogonal());
        orthogonalMatrix.Orthogonalize();
        EXPECT_TRUE(orthogonalMatrix.IsClose(AZ::Matrix3x4::CreateRotationY(-0.2f)));

        // a matrix which isn't already orthogonal should be made orthogonal
        AZ::Matrix3x4 nonOrthogonalMatrix = AZ::Matrix3x4::CreateScale(AZ::Vector3(0.7f, 0.7f, 0.2f));
        EXPECT_FALSE(nonOrthogonalMatrix.IsOrthogonal());
        nonOrthogonalMatrix.Orthogonalize();
        EXPECT_TRUE(nonOrthogonalMatrix.IsOrthogonal());
    }

    TEST(MATH_Matrix3x4, IsClose)
    {
        const AZ::Matrix3x4 matrix1 = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.12f, 0.24f, -0.72f, 0.64f),
            AZ::Vector3(0.3f, 0.2f, -0.7f)
        );

        AZ::Matrix3x4 matrix2 = matrix1;
        EXPECT_TRUE(matrix2.IsClose(matrix1, 1e-6f));
        matrix2.SetElement(0, 2, matrix2(0, 2) + 1e-2f);
        matrix2.SetElement(2, 3, matrix2(2, 3) + 1e-4f);
        matrix2.SetElement(1, 1, matrix2(1, 1) - 1e-6f);
        EXPECT_TRUE(matrix2.IsClose(matrix1, 1e-1f));
        EXPECT_FALSE(matrix2.IsClose(matrix1, 1e-3f));
        EXPECT_FALSE(matrix2.IsClose(matrix1, 1e-5f));
        EXPECT_FALSE(matrix2.IsClose(matrix1, 1e-7f));
    }

    TEST(MATH_Matrix3x4, Equality)
    {
        const AZ::Matrix3x4 matrix1 = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.12f, 0.24f, -0.72f, 0.64f),
            AZ::Vector3(0.3f, 0.2f, -0.7f)
        );

        AZ::Matrix3x4 matrix2 = matrix1;
        EXPECT_TRUE(matrix2 == matrix1);
        EXPECT_FALSE(matrix2 != matrix1);
        for (int row = 0; row < 3; row++)
        {
            for (int col = 0; col < 3; col++)
            {
                matrix2 = matrix1;
                matrix2.SetElement(row, col, matrix2(row, col) + 1e-4f);
                EXPECT_FALSE(matrix2 == matrix1);
                EXPECT_TRUE(matrix2 != matrix1);
            }
        }
    }

    using Matrix3x4SetFromEulerDegreesFixture = ::testing::TestWithParam<AZ::Vector3>;

    TEST_P(Matrix3x4SetFromEulerDegreesFixture, SetFromEulerDegrees)
    {
        const AZ::Vector3 eulerDegrees = GetParam();
        AZ::Matrix3x4 matrix;
        matrix.SetFromEulerDegrees(eulerDegrees);
        const AZ::Vector3 eulerRadians = AZ::Vector3DegToRad(eulerDegrees);
        const AZ::Matrix3x4 rotX = AZ::Matrix3x4::CreateRotationX(eulerRadians.GetX());
        const AZ::Matrix3x4 rotY = AZ::Matrix3x4::CreateRotationY(eulerRadians.GetY());
        const AZ::Matrix3x4 rotZ = AZ::Matrix3x4::CreateRotationZ(eulerRadians.GetZ());
        EXPECT_TRUE(matrix.IsClose(rotX * rotY * rotZ));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4SetFromEulerDegreesFixture, ::testing::ValuesIn(MathTestData::EulerAnglesDegrees));

    using Matrix3x4SetFromEulerRadiansFixture = ::testing::TestWithParam<AZ::Vector3>;

    TEST_P(Matrix3x4SetFromEulerRadiansFixture, SetFromEulerRadians)
    {
        const AZ::Vector3 eulerRadians = GetParam();
        AZ::Matrix3x4 matrix;
        matrix.SetFromEulerRadians(eulerRadians);
        const AZ::Matrix3x4 rotX = AZ::Matrix3x4::CreateRotationX(eulerRadians.GetX());
        const AZ::Matrix3x4 rotY = AZ::Matrix3x4::CreateRotationY(eulerRadians.GetY());
        const AZ::Matrix3x4 rotZ = AZ::Matrix3x4::CreateRotationZ(eulerRadians.GetZ());
        EXPECT_TRUE(matrix.IsClose(rotX * rotY * rotZ));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4SetFromEulerRadiansFixture, ::testing::ValuesIn(MathTestData::EulerAnglesRadians));

    using Matrix3x4GetEulerFixture = ::testing::TestWithParam<AZ::Matrix3x4>;

    TEST_P(Matrix3x4GetEulerFixture, GetEuler)
    {
        // there isn't a one to one mapping between matrices and Euler angles, so testing for a particular set of Euler
        // angles to be returned would be fragile, but getting the Euler angles and creating a new matrix from them
        // should return the original matrix
        AZ::Matrix3x4 matrix = GetParam();
        matrix.SetTranslation(AZ::Vector3::CreateZero());
        const AZ::Vector3 eulerDegrees = matrix.GetEulerDegrees();
        AZ::Matrix3x4 eulerMatrix;
        eulerMatrix.SetFromEulerDegrees(eulerDegrees);
        EXPECT_TRUE(eulerMatrix.IsClose(matrix));
        const AZ::Vector3 eulerRadians = matrix.GetEulerRadians();
        eulerMatrix = AZ::Matrix3x4::Identity();
        eulerMatrix.SetFromEulerRadians(eulerRadians);
        EXPECT_TRUE(eulerMatrix.IsClose(matrix));
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4GetEulerFixture, ::testing::ValuesIn(MathTestData::OrthogonalMatrix3x4s));

    using Matrix3x4GetDeterminantFixture = ::testing::TestWithParam<AZ::Matrix3x4>;

    TEST_P(Matrix3x4GetDeterminantFixture, GetDeterminantOfOrthogonalMatrices)
    {
        const AZ::Matrix3x4 matrix = GetParam();
        EXPECT_NEAR(matrix.GetDeterminant3x3(), 1.0f, 1e-3f);
    }

    INSTANTIATE_TEST_CASE_P(MATH_Matrix3x4, Matrix3x4GetDeterminantFixture, ::testing::ValuesIn(MathTestData::OrthogonalMatrix3x4s));

    TEST(MATH_Matrix3x4, GetDeterminantOfArbitraryMatrices)
    {
        const AZ::Matrix3x4 matrix1 = AZ::Matrix3x4::CreateFromValue(0.8f);
        const AZ::Matrix3x4 matrix2 = AZ::Matrix3x4::CreateDiagonal(AZ::Vector3(0.2f, 1.5f, 0.6f));
        const AZ::Matrix3x4 matrix3 = AZ::Matrix3x4::CreateRotationY(0.2f) * AZ::Matrix3x4::CreateScale(AZ::Vector3(2.0f, 0.5f, 1.2f));
        const AZ::Matrix3x4 matrix4 = AZ::Matrix3x4::CreateFromRows(matrix3.GetRow(0), matrix3.GetRow(2), matrix3.GetRow(1));
        const float expected1 = 0.0f;
        const float expected2 = 0.18f;
        const float expected3 = 1.2f;
        const float expected4 = -expected3;
        EXPECT_NEAR(matrix1.GetDeterminant3x3(), expected1, 1e-3f);
        EXPECT_NEAR(matrix2.GetDeterminant3x3(), expected2, 1e-3f);
        EXPECT_NEAR(matrix3.GetDeterminant3x3(), expected3, 1e-3f);
        EXPECT_NEAR(matrix4.GetDeterminant3x3(), expected4, 1e-3f);
        EXPECT_NEAR((matrix2 * matrix3).GetDeterminant3x3(), expected2 * expected3, 1e-3f);
        EXPECT_NEAR(matrix2.GetTranspose3x3().GetDeterminant3x3(), expected2, 1e-3f);
    }

#if AZ_TRAIT_DISABLE_FAILED_MATH_TESTS
    TEST(MATH_Matrix3x4, DISABLED_IsFinite)
#else
    TEST(MATH_Matrix3x4, IsFinite)
#endif // AZ_TRAIT_DISABLE_FAILED_MATH_TESTS
    {
        AZ::Matrix3x4 matrix = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(-0.42f, -0.46f, 0.66f, 0.42f),
            AZ::Vector3(0.8f, -2.3f, 2.2f)
        );
        EXPECT_TRUE(matrix.IsFinite());
        for (int row = 0; row < 3; row++)
        {
            for (int col = 0; col < 3; col++)
            {
                float value = matrix.GetElement(row, col);
                matrix.SetElement(row, col, acosf(2.0f));
                EXPECT_FALSE(matrix.IsFinite());
                matrix.SetElement(row, col, value);
            }
        }
    }
} // namespace UnitTest
