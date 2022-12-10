/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MatrixUtils.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    using namespace AZ;

    class MatrixUtilsTests : public LeakDetectionFixture
    {
    protected:
        const float floatEpsilon = 0.001f;
    };

    TEST_F(MatrixUtilsTests, PerspectiveMatrixFovRH)
    {
        Matrix4x4 matrix;

        constexpr float near_value = 10.0f;
        constexpr float far_value = 1000.0f;
        float fovY = Constants::HalfPi;
        float aspectRatio = 1.0f;
        auto matPtr = MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, near_value, far_value);
        EXPECT_NE(matPtr, nullptr);
        EXPECT_EQ(matPtr, &matrix);

        // transform some positions and valid the output
        // point on near plane
        Vector3 nearPos(0.0f, 0.0f, -near_value);
        Vector3 result = MatrixTransformPosition(matrix, nearPos);
        EXPECT_NEAR(result.GetZ(), 0.0f, floatEpsilon);

        // point on far plane
        Vector3 farPos(0.0f, 0.0f, -far_value);
        result = MatrixTransformPosition(matrix, farPos);
        EXPECT_NEAR(result.GetZ(), 1.0f, floatEpsilon);

        // point further than far plane
        Vector3 furtherFarPos(0.0f, 0.0f, -far_value - 1000.0f);
        result = MatrixTransformPosition(matrix, furtherFarPos);
        EXPECT_GT(result.GetZ(), 1.0f);

        // point closer then near plane
        Vector3 closerNearPos(0.0f, 0.0f, -near_value + 5.0f);
        result = MatrixTransformPosition(matrix, closerNearPos);
        EXPECT_LT(result.GetZ(), 0.0f);

        // point between near and far
        Vector3 betweenPos(0.0f, 0.0f, -(far_value + near_value) / 2.0f);
        result = MatrixTransformPosition(matrix, betweenPos);
        EXPECT_GT(result.GetZ(), 0.0f);
        EXPECT_LT(result.GetZ(), 1.0f);

        // x and y value
        Vector3 insideLeftTopPos(-900.0f, 900.0f, -far_value);
        Vector3 insideRightBottomPos(900.0f, -900.0f, -far_value);
        Vector3 outsideLeftTopPos(-1100.0f, 1100.0f, -far_value);
        Vector3 outsideRightBottomPos(1100.0f, -1100.0f, -far_value);

        result = MatrixTransformPosition(matrix, insideLeftTopPos);
        EXPECT_LT(result.GetX(), 0.0f);
        EXPECT_GT(result.GetX(), -1.0f);
        EXPECT_GT(result.GetY(), 0.0f);
        EXPECT_LT(result.GetY(), 1.0f);
        result = MatrixTransformPosition(matrix, insideRightBottomPos);
        EXPECT_GT(result.GetX(), 0.0f);
        EXPECT_LT(result.GetX(), 1.0f);
        EXPECT_LT(result.GetY(), 0.0f);
        EXPECT_GT(result.GetY(), -1.0f);
        result = MatrixTransformPosition(matrix, outsideLeftTopPos);
        EXPECT_LT(result.GetX(), -1.0f);
        EXPECT_GT(result.GetY(), 1.0f);
        result = MatrixTransformPosition(matrix, outsideRightBottomPos);
        EXPECT_GT(result.GetX(), 1.0f);
        EXPECT_LT(result.GetY(), -1.0f);

        // create a reverse depth perspective projection matrix
        MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, near_value, far_value, true);
        result = MatrixTransformPosition(matrix, nearPos);
        EXPECT_NEAR(result.GetZ(), 1.0f, floatEpsilon);
        result = MatrixTransformPosition(matrix, farPos);
        EXPECT_NEAR(result.GetZ(), 0.0f, floatEpsilon);
        result = MatrixTransformPosition(matrix, furtherFarPos);
        EXPECT_LT(result.GetZ(), 0.0f);
        result = MatrixTransformPosition(matrix, closerNearPos);
        EXPECT_GT(result.GetZ(), 1.0f);
        result = MatrixTransformPosition(matrix, betweenPos);
        EXPECT_GT(result.GetZ(), 0.0f);
        EXPECT_LT(result.GetZ(), 1.0f);
        result = MatrixTransformPosition(matrix, insideLeftTopPos);
        EXPECT_LT(result.GetX(), 0.0f);
        EXPECT_GT(result.GetX(), -1.0f);
        EXPECT_GT(result.GetY(), 0.0f);
        EXPECT_LT(result.GetY(), 1.0f);
        result = MatrixTransformPosition(matrix, insideRightBottomPos);
        EXPECT_GT(result.GetX(), 0.0f);
        EXPECT_LT(result.GetX(), 1.0f);
        EXPECT_LT(result.GetY(), 0.0f);
        EXPECT_GT(result.GetY(), -1.0f);
        result = MatrixTransformPosition(matrix, outsideLeftTopPos);
        EXPECT_LT(result.GetX(), -1.0f);
        EXPECT_GT(result.GetY(), 1.0f);
        result = MatrixTransformPosition(matrix, outsideRightBottomPos);
        EXPECT_GT(result.GetX(), 1.0f);
        EXPECT_LT(result.GetY(), -1.0f);

        // bad input
        UnitTest::TestRunner::Instance().StartAssertTests();
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, fovY, -1.0f, near_value, far_value));
        EXPECT_EQ(1, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, 0.0f, aspectRatio, near_value, far_value));
        EXPECT_EQ(2, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, -near_value, far_value));
        EXPECT_EQ(3, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, near_value, -far_value));
        EXPECT_EQ(4, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, 0.0f, far_value));
        EXPECT_EQ(5, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        UnitTest::TestRunner::Instance().StopAssertTests();
    }

    TEST_F(MatrixUtilsTests, OrthographicMatrixRH)
    {
        Matrix4x4 matrix;

        auto ptr = MakeOrthographicMatrixRH(matrix, -100.0f, 0.0f, 100.0f, 200.0f, 0.0f, 1000.0f);
        EXPECT_NE(ptr, nullptr);
        EXPECT_EQ(ptr, &matrix);

        Vector3 result;

        // center position
        result = MatrixTransformPosition(matrix, Vector3(-50.0f, 150.0f, -500.0f));
        EXPECT_NEAR(result.GetX(), 0.0f, floatEpsilon);
        EXPECT_NEAR(result.GetY(), 0.0f, floatEpsilon);
        EXPECT_NEAR(result.GetZ(), 0.5f, floatEpsilon);

        // left top and far
        result = MatrixTransformPosition(matrix, Vector3(-100.0f, 200.0f, -1000.0f));
        EXPECT_NEAR(result.GetX(), -1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetY(), 1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetZ(), 1.0f, floatEpsilon);

        // right bottom and near
        result = MatrixTransformPosition(matrix, Vector3(0.0f, 100.0f, 0.0f));
        EXPECT_NEAR(result.GetX(), 1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetY(), -1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetZ(), 0.0f, floatEpsilon);

        // further than far
        result = MatrixTransformPosition(matrix, Vector3(-50.0f, 150.0f, -2000.0f));
        EXPECT_GT(result.GetZ(), 1.0f);

        // closer than near
        result = MatrixTransformPosition(matrix, Vector3(-50.0f, 150.0f, 200.0f));
        EXPECT_LT(result.GetZ(), 0.0f);
    }

    TEST_F(MatrixUtilsTests, FrustumMatrixRH)
    {
        Matrix4x4 matrix;

        auto ptr = MakeFrustumMatrixRH(matrix, -100.0f, 0.0f, 100.0f, 200.0f, 1.0f, 1000.0f);
        EXPECT_NE(ptr, nullptr);
        EXPECT_EQ(ptr, &matrix);

        Vector3 result;

        // left top and near
        Vector3 leftTopNear(-100.0f, 200.0f, -1.0f);
        result = MatrixTransformPosition(matrix, leftTopNear);
        EXPECT_NEAR(result.GetX(), -1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetY(), 1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetZ(), 0.0f, floatEpsilon);

        // right bottom and near
        Vector3 rightBottomNear(0.0f, 100.0f, -1.0f);
        result = MatrixTransformPosition(matrix, rightBottomNear);
        EXPECT_NEAR(result.GetX(), 1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetY(), -1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetZ(), 0.0f, floatEpsilon);

        // further than far
        Vector3 far_value(-50.0f, 150.0f, -1000.0f);
        result = MatrixTransformPosition(matrix, far_value);
        EXPECT_NEAR(result.GetZ(), 1.0f, floatEpsilon);
        Vector3 furtherFar(-50.0f, 150.0f, -2000.0f);
        result = MatrixTransformPosition(matrix, furtherFar);
        EXPECT_GT(result.GetZ(), 1.0f);

        // closer than near
        Vector3 closerNear(-50.0f, 150.0f, -0.5f);
        result = MatrixTransformPosition(matrix, closerNear);
        EXPECT_LT(result.GetZ(), 0.0f);

        // reverse depth
        MakeFrustumMatrixRH(matrix, -100.0f, 0.0f, 100.0f, 200.0f, 1.0f, 1000.0f, true);
        result = MatrixTransformPosition(matrix, leftTopNear);
        EXPECT_NEAR(result.GetX(), -1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetY(), 1.0f, floatEpsilon);
        result = MatrixTransformPosition(matrix, rightBottomNear);
        EXPECT_NEAR(result.GetX(), 1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetY(), -1.0f, floatEpsilon);
        EXPECT_NEAR(result.GetZ(), 1.0f, floatEpsilon);
        result = MatrixTransformPosition(matrix, far_value);
        EXPECT_NEAR(result.GetZ(), 0.0f, floatEpsilon);

        // bad input
        UnitTest::TestRunner::Instance().StartAssertTests();
        // near = 0
        EXPECT_FALSE(MakeFrustumMatrixRH(matrix, -100.0f, 0.0f, 100.0f, 200.0f, 0.0f, 1000.0f));
        EXPECT_EQ(1, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        UnitTest::TestRunner::Instance().StopAssertTests();
    }
}
