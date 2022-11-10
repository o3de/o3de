/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <NumericalMethods/DoublePrecisionMath/Quaternion.h>

namespace NumericalMethods::DoublePrecisionMath
{
    static const float Tolerance = 1e-6f;

    TEST(DoublePrecisionMath_Quaternion, Getters)
    {
        const double x = 0.22;
        const double y = 0.26;
        const double z = 0.02;
        const double w = 0.94;
        const Quaternion quaternion(x, y, z, w);
        EXPECT_NEAR(quaternion.GetX(), x, Tolerance);
        EXPECT_NEAR(quaternion.GetY(), y, Tolerance);
        EXPECT_NEAR(quaternion.GetZ(), z, Tolerance);
        EXPECT_NEAR(quaternion.GetW(), w, Tolerance);
    }

    TEST(DoublePrecisionMath_Quaternion, CopyConstructor)
    {
        const double x = -0.46;
        const double y = -0.26;
        const double z = 0.58;
        const double w = 0.62;
        const Quaternion quaternion1(x, y, z, w);
        const Quaternion quaternion2(quaternion1);
        EXPECT_NEAR(quaternion2.GetX(), x, Tolerance);
        EXPECT_NEAR(quaternion2.GetY(), y, Tolerance);
        EXPECT_NEAR(quaternion2.GetZ(), z, Tolerance);
        EXPECT_NEAR(quaternion2.GetW(), w, Tolerance);
    }

    TEST(DoublePrecisionMath_Quaternion, FromSingle)
    {
        const float x = 0.58f;
        const float y = -0.22f;
        const float z = 0.26f;
        const float w = 0.74f;
        const AZ::Quaternion quaternionSingle(x, y, z, w);
        const Quaternion quaternionDouble(quaternionSingle);
        EXPECT_NEAR(quaternionDouble.GetX(), x, Tolerance);
        EXPECT_NEAR(quaternionDouble.GetY(), y, Tolerance);
        EXPECT_NEAR(quaternionDouble.GetZ(), z, Tolerance);
        EXPECT_NEAR(quaternionDouble.GetW(), w, Tolerance);
    }

    TEST(DoublePrecisionMath_Quaternion, ToSingle)
    {
        const float x = -0.32f;
        const float y = 0.16f;
        const float z = -0.16f;
        const float w = 0.92f;
        const Quaternion quaternionDouble(x, y, z, w);
        const AZ::Quaternion quaternionSingle = quaternionDouble.ToSingle();
        EXPECT_NEAR(quaternionSingle.GetX(), x, Tolerance);
        EXPECT_NEAR(quaternionSingle.GetY(), y, Tolerance);
        EXPECT_NEAR(quaternionSingle.GetZ(), z, Tolerance);
        EXPECT_NEAR(quaternionSingle.GetW(), w, Tolerance);
    }

    TEST(DoublePrecisionMath_Quaternion, Multiply)
    {
        const Quaternion quaternion1(0.1, 0.1, 0.7, 0.7);
        const Quaternion quaternion2(0.1, 0.7, 0.7, 0.1);
        const Quaternion product = quaternion1 * quaternion2;
        EXPECT_NEAR(product.GetX(), -0.34, Tolerance);
        EXPECT_NEAR(product.GetY(), 0.5, Tolerance);
        EXPECT_NEAR(product.GetZ(), 0.62, Tolerance);
        EXPECT_NEAR(product.GetW(), -0.5, Tolerance);
    }

    TEST(DoublePrecisionMath_Quaternion, Negate)
    {
        const double x = -0.48;
        const double y = 0.24;
        const double z = 0.44;
        const double w = -0.72;
        const Quaternion quaternion1(x, y, z, w);
        const Quaternion quaternion2 = -quaternion1;
        EXPECT_NEAR(quaternion2.GetX(), -x, Tolerance);
        EXPECT_NEAR(quaternion2.GetY(), -y, Tolerance);
        EXPECT_NEAR(quaternion2.GetZ(), -z, Tolerance);
        EXPECT_NEAR(quaternion2.GetW(), -w, Tolerance);
    }

    TEST(DoublePrecisionMath_Quaternion, GetNormalized)
    {
        const double x = 0.22;
        const double y = -0.74;
        const double z = -0.14;
        const double w = 0.62;
        const double magnitude = 1.5;
        const Quaternion quaternion(magnitude * x, magnitude * y, magnitude * z, magnitude * w);
        const Quaternion quaternionNormalized = quaternion.GetNormalized();
        EXPECT_NEAR(quaternionNormalized.GetX(), x, Tolerance);
        EXPECT_NEAR(quaternionNormalized.GetY(), y, Tolerance);
        EXPECT_NEAR(quaternionNormalized.GetZ(), z, Tolerance);
        EXPECT_NEAR(quaternionNormalized.GetW(), w, Tolerance);
    }

    TEST(DoublePrecisionMath_Quaternion, GetConjugate)
    {
        const double x = 0.16;
        const double y = 0.64;
        const double z = -0.68;
        const double w = 0.32;
        const Quaternion quaternion(x, y, z, w);
        const Quaternion conjugate = quaternion.GetConjugate();
        EXPECT_NEAR(conjugate.GetX(), -x, Tolerance);
        EXPECT_NEAR(conjugate.GetY(), -y, Tolerance);
        EXPECT_NEAR(conjugate.GetZ(), -z, Tolerance);
        EXPECT_NEAR(conjugate.GetW(), w, Tolerance);
    }
} // namespace NumericalMethods::DoublePrecisionMath
