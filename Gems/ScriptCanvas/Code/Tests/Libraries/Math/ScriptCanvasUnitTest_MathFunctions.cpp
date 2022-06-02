/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Math/MathFunctions.h>
#include <Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    class ScriptCanvasUnitTestMathFunctions
        : public ScriptCanvasUnitTestFixture
    {
    public:
        void SetUp() override
        {
            ScriptCanvasUnitTestFixture::SetUp();

            MathNodeUtilities::RandomEngineInit();
        }

        void TearDown() override
        {
            MathNodeUtilities::RandomEngineReset();

            ScriptCanvasUnitTestFixture::TearDown();
        }
    };

    TEST_F(ScriptCanvasUnitTestMathFunctions, MultiplyAndAdd_Call_GetExpectedResult)
    {
        auto actualResult = MathFunctions::MultiplyAndAdd(1, 1, 1);
        EXPECT_EQ(actualResult, 2);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, StringToNumber_Call_GetExpectedResult)
    {
        auto actualResult = MathFunctions::StringToNumber("123");
        EXPECT_EQ(actualResult, 123);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomColor_Call_GetExpectedResult)
    {
        AZ::Color min = AZ::Color::CreateZero();
        AZ::Color max = AZ::Color::CreateOne();
        auto actualResult = MathRandoms::RandomColor(min, max);
        EXPECT_TRUE(actualResult.GetR() >= min.GetR() && actualResult.GetR() <= max.GetR());
        EXPECT_TRUE(actualResult.GetG() >= min.GetG() && actualResult.GetG() <= max.GetG());
        EXPECT_TRUE(actualResult.GetB() >= min.GetB() && actualResult.GetB() <= max.GetB());
        EXPECT_TRUE(actualResult.GetA() >= min.GetA() && actualResult.GetA() <= max.GetA());
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomGrayscale_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomGrayscale(0, 1);
        EXPECT_TRUE(actualResult.GetR() >= 0 && actualResult.GetR() <= 1);
        EXPECT_TRUE(actualResult.GetG() >= 0 && actualResult.GetG() <= 1);
        EXPECT_TRUE(actualResult.GetB() >= 0 && actualResult.GetB() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomInteger_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomInteger(0, 1);
        EXPECT_TRUE(actualResult >= 0 && actualResult <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomNumber_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomNumber(0, 1);
        EXPECT_TRUE(actualResult >= 0 && actualResult <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointInBox_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointInBox(AZ::Vector3(1, 1, 1));
        EXPECT_TRUE(actualResult.GetX() >= -0.5 && actualResult.GetX() <= 0.5);
        EXPECT_TRUE(actualResult.GetY() >= -0.5 && actualResult.GetY() <= 0.5);
        EXPECT_TRUE(actualResult.GetZ() >= -0.5 && actualResult.GetZ() <= 0.5);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointOnCircle_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointOnCircle(1);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointInCone_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointInCone(1, 180);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
        EXPECT_TRUE(actualResult.GetZ() >= 0 && actualResult.GetZ() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointInCylinder_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointInCylinder(1, 1);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
        EXPECT_TRUE(actualResult.GetZ() >= -0.5 && actualResult.GetZ() <= 0.5);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointInCircle_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointInCircle(1);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointInEllipsoid_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointInEllipsoid(AZ::Vector3(1, 2, 3));
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -2 && actualResult.GetY() <= 2);
        EXPECT_TRUE(actualResult.GetY() >= -3 && actualResult.GetY() <= 3);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointInSphere_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointInSphere(1);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
        EXPECT_TRUE(actualResult.GetZ() >= -1 && actualResult.GetZ() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointInSquare_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointInSquare(AZ::Vector2(1, 1));
        EXPECT_TRUE(actualResult.GetX() >= -0.5 && actualResult.GetX() <= 0.5);
        EXPECT_TRUE(actualResult.GetY() >= -0.5 && actualResult.GetY() <= 0.5);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomPointOnSphere_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomPointOnSphere(1);
        EXPECT_TRUE(AZStd::abs(actualResult.GetLength() - 1) < 0.01);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
        EXPECT_TRUE(actualResult.GetZ() >= -1 && actualResult.GetZ() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomQuaternion_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomQuaternion(0, 30);
        EXPECT_TRUE(actualResult.GetAngle() >= 0 && actualResult.GetX() <= 30);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
        EXPECT_TRUE(actualResult.GetZ() >= -1 && actualResult.GetZ() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomUnitVector2_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomUnitVector2();
        EXPECT_TRUE(AZStd::abs(actualResult.GetLength() - 1) < 0.01);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomUnitVector3_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomUnitVector3();
        EXPECT_TRUE(AZStd::abs(actualResult.GetLength() - 1) < 0.01);
        EXPECT_TRUE(actualResult.GetX() >= -1 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= -1 && actualResult.GetY() <= 1);
        EXPECT_TRUE(actualResult.GetZ() >= -1 && actualResult.GetZ() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomVector2_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomVector2(AZ::Vector2(0, 0), AZ::Vector2(1, 1));
        EXPECT_TRUE(actualResult.GetX() >= 0 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= 0 && actualResult.GetY() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomVector3_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomVector3(AZ::Vector3(0, 0, 0), AZ::Vector3(1, 1, 1));
        EXPECT_TRUE(actualResult.GetX() >= 0 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= 0 && actualResult.GetY() <= 1);
        EXPECT_TRUE(actualResult.GetZ() >= 0 && actualResult.GetZ() <= 1);
    }

    TEST_F(ScriptCanvasUnitTestMathFunctions, RandomVector4_Call_GetExpectedResult)
    {
        auto actualResult = MathRandoms::RandomVector4(AZ::Vector4(0, 0, 0, 0), AZ::Vector4(1, 1, 1, 1));
        EXPECT_TRUE(actualResult.GetX() >= 0 && actualResult.GetX() <= 1);
        EXPECT_TRUE(actualResult.GetY() >= 0 && actualResult.GetY() <= 1);
        EXPECT_TRUE(actualResult.GetZ() >= 0 && actualResult.GetZ() <= 1);
        EXPECT_TRUE(actualResult.GetW() >= 0 && actualResult.GetW() <= 1);
    }
} // namespace ScriptCanvasUnitTest
