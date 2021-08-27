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

    class MatrixUtilsTests : public AllocatorsTestFixture
    {
    protected:
        const float floatEpsilon = 0.001f;
    };

    TEST_F(MatrixUtilsTests, PerspectiveMatrixFovRH)
    {
        Matrix4x4 matrix;

        constexpr float near_value = 10;
        constexpr float far_value = 1000;
        float fovY = Constants::HalfPi;
        float aspectRatio = 1.f;
        auto matPtr = MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, near_value, far_value);
        EXPECT_NE(matPtr, nullptr);
        EXPECT_EQ(matPtr, &matrix);

        // transform some positions and valid the output
        // point on near plane
        Vector3 nearPos(0, 0, -near_value);
        Vector3 result = MatrixTransformPosition(matrix, nearPos);
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 0, floatEpsilon));

        // point on far plane
        Vector3 farPos(0, 0, -far_value);
        result = MatrixTransformPosition(matrix, farPos);
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 1, floatEpsilon));

        // point further than far plane
        Vector3 furtherFarPos(0, 0, -far_value - 1000);
        result = MatrixTransformPosition(matrix, furtherFarPos);
        EXPECT_TRUE(result.GetZ() > 1.f);

        // point closer then near plane
        Vector3 closerNearPos(0, 0, -near_value + 5);
        result = MatrixTransformPosition(matrix, closerNearPos);
        EXPECT_TRUE(result.GetZ() < 0.f);

        // point between near and far
        Vector3 betweenPos(0, 0, -(far_value + near_value) / 2);
        result = MatrixTransformPosition(matrix, betweenPos);
        EXPECT_TRUE(result.GetZ() > 0.f);
        EXPECT_TRUE(result.GetZ() < 1.f);

        // x and y value
        Vector3 insideLeftTopPos(-900, 900, -far_value);
        Vector3 insideRightBottomPos(900, -900, -far_value);
        Vector3 outsideLeftTopPos(-1100, 1100, -far_value);
        Vector3 outsideRightBottomPos(1100, -1100, -far_value);

        result = MatrixTransformPosition(matrix, insideLeftTopPos);
        EXPECT_TRUE(result.GetX() < 0);
        EXPECT_TRUE(result.GetX() > -1);
        EXPECT_TRUE(result.GetY() > 0);
        EXPECT_TRUE(result.GetY() < 1);
        result = MatrixTransformPosition(matrix, insideRightBottomPos);
        EXPECT_TRUE(result.GetX() > 0);
        EXPECT_TRUE(result.GetX() < 1);
        EXPECT_TRUE(result.GetY() < 0);
        EXPECT_TRUE(result.GetY() > -1);
        result = MatrixTransformPosition(matrix, outsideLeftTopPos);
        EXPECT_TRUE(result.GetX() < -1);
        EXPECT_TRUE(result.GetY() > 1);
        result = MatrixTransformPosition(matrix, outsideRightBottomPos);
        EXPECT_TRUE(result.GetX() > 1);
        EXPECT_TRUE(result.GetY() < -1);

        // create a reverse depth perspective projection matrix
        MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, near_value, far_value, true);
        result = MatrixTransformPosition(matrix, nearPos);
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 1, floatEpsilon));
        result = MatrixTransformPosition(matrix, farPos);
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 0, floatEpsilon));
        result = MatrixTransformPosition(matrix, furtherFarPos);
        EXPECT_TRUE(result.GetZ() < 0.f);
        result = MatrixTransformPosition(matrix, closerNearPos);
        EXPECT_TRUE(result.GetZ() > 1.f);
        result = MatrixTransformPosition(matrix, betweenPos);
        EXPECT_TRUE(result.GetZ() > 0.f);
        EXPECT_TRUE(result.GetZ() < 1.f);
        result = MatrixTransformPosition(matrix, insideLeftTopPos);
        EXPECT_TRUE(result.GetX() < 0);
        EXPECT_TRUE(result.GetX() > -1);
        EXPECT_TRUE(result.GetY() > 0);
        EXPECT_TRUE(result.GetY() < 1);
        result = MatrixTransformPosition(matrix, insideRightBottomPos);
        EXPECT_TRUE(result.GetX() > 0);
        EXPECT_TRUE(result.GetX() < 1);
        EXPECT_TRUE(result.GetY() < 0);
        EXPECT_TRUE(result.GetY() > -1);
        result = MatrixTransformPosition(matrix, outsideLeftTopPos);
        EXPECT_TRUE(result.GetX() < -1);
        EXPECT_TRUE(result.GetY() > 1);
        result = MatrixTransformPosition(matrix, outsideRightBottomPos);
        EXPECT_TRUE(result.GetX() > 1);
        EXPECT_TRUE(result.GetY() < -1);

        // bad input
        UnitTest::TestRunner::Instance().StartAssertTests();
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, fovY, -1, near_value, far_value));
        EXPECT_EQ(1, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, 0, aspectRatio, near_value, far_value));
        EXPECT_EQ(2, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, -near_value, far_value));
        EXPECT_EQ(3, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, near_value, -far_value));
        EXPECT_EQ(4, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        EXPECT_FALSE(MakePerspectiveFovMatrixRH(matrix, fovY, aspectRatio, 0, far_value));
        EXPECT_EQ(5, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        UnitTest::TestRunner::Instance().StopAssertTests();
    }

    TEST_F(MatrixUtilsTests, OrthographicMatrixRH)
    {
        Matrix4x4 matrix;

        auto ptr = MakeOrthographicMatrixRH(matrix, -100, 0, 100, 200, 0, 1000);
        EXPECT_NE(ptr, nullptr);
        EXPECT_EQ(ptr, &matrix);

        Vector3 result;

        // center position
        result = MatrixTransformPosition(matrix, Vector3(-50, 150, -500));
        EXPECT_TRUE(AZ::IsClose(result.GetX(), 0, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetY(), 0, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 0.5f, floatEpsilon));

        // left top and far
        result = MatrixTransformPosition(matrix, Vector3(-100, 200, -1000));
        EXPECT_TRUE(AZ::IsClose(result.GetX(), -1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetY(), 1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 1, floatEpsilon));

        // right bottom and near
        result = MatrixTransformPosition(matrix, Vector3(0, 100, 0));
        EXPECT_TRUE(AZ::IsClose(result.GetX(), 1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetY(), -1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 0, floatEpsilon));

        // further than far
        result = MatrixTransformPosition(matrix, Vector3(-50, 150, -2000));
        EXPECT_TRUE(result.GetZ() > 1);

        // closer than near
        result = MatrixTransformPosition(matrix, Vector3(-50, 150, 200));
        EXPECT_TRUE(result.GetZ() < 0);
    }

    TEST_F(MatrixUtilsTests, FrustumMatrixRH)
    {
        Matrix4x4 matrix;

        auto ptr = MakeFrustumMatrixRH(matrix, -100, 0, 100, 200, 1, 1000);
        EXPECT_NE(ptr, nullptr);
        EXPECT_EQ(ptr, &matrix);

        Vector3 result;

        // left top and near
        Vector3 leftTopNear(-100, 200, -1);
        result = MatrixTransformPosition(matrix, leftTopNear);
        EXPECT_TRUE(AZ::IsClose(result.GetX(), -1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetY(), 1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 0, floatEpsilon));

        // right bottom and near
        Vector3 rightBottomNear(0, 100, -1);
        result = MatrixTransformPosition(matrix, rightBottomNear);
        EXPECT_TRUE(AZ::IsClose(result.GetX(), 1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetY(), -1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 0, floatEpsilon));

        // further than far
        Vector3 far_value(-50, 150, -1000);
        result = MatrixTransformPosition(matrix, far_value);
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 1, floatEpsilon));
        Vector3 furtherFar(-50, 150, -2000);
        result = MatrixTransformPosition(matrix, furtherFar);
        EXPECT_TRUE(result.GetZ() > 1);

        // closer than near
        Vector3 closerNear(-50, 150, -0.5f);
        result = MatrixTransformPosition(matrix, closerNear);
        EXPECT_TRUE(result.GetZ() < 0);

        // reverse depth
        MakeFrustumMatrixRH(matrix, -100, 0, 100, 200, 1, 1000, true);
        result = MatrixTransformPosition(matrix, leftTopNear);
        EXPECT_TRUE(AZ::IsClose(result.GetX(), -1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetY(), 1, floatEpsilon));
        result = MatrixTransformPosition(matrix, rightBottomNear);
        EXPECT_TRUE(AZ::IsClose(result.GetX(), 1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetY(), -1, floatEpsilon));
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 1, floatEpsilon));
        result = MatrixTransformPosition(matrix, far_value);
        EXPECT_TRUE(AZ::IsClose(result.GetZ(), 0, floatEpsilon));

        // bad input
        UnitTest::TestRunner::Instance().StartAssertTests();
        // near = 0
        EXPECT_FALSE(MakeFrustumMatrixRH(matrix, -100, 0, 100, 200, 0, 1000));
        EXPECT_EQ(1, UnitTest::TestRunner::Instance().m_numAssertsFailed);
        UnitTest::TestRunner::Instance().StopAssertTests();
    }
}
