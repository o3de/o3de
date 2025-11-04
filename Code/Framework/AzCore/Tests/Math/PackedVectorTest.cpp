/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/PackedVector4.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/Math/PackedVector2.h>
#include <Math/MathTest.h>

namespace UnitTest
{
    TEST(MATH_PackedVector2, TestConstructors)
    {
        EXPECT_THAT(static_cast<AZ::Vector2>(AZ::PackedVector2f(1.0f, 3.0f)), IsClose(AZ::Vector2(1.0f, 3.0f)));
        EXPECT_THAT(static_cast<AZ::Vector2>(AZ::PackedVector2f(1.0f)), IsClose(AZ::Vector2(1.0f, 1.0f)));

        AZ::PackedVector2f vC(1.0f, 2.0f);
        EXPECT_FLOAT_EQ(vC.GetX(), 1.0f);
        EXPECT_FLOAT_EQ(vC.GetY(), 2.0f);

        AZ::PackedVector2f vb(1.0f, 2.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(0), 1.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(1), 2.0f);
    }

    TEST(MATH_PackedVector2, TestStoreFloat)
    {
        AZ::PackedVector2f values(0.0f, 0.0f);

        AZ::Vector2 vA(1.0f, 2.0f);
        vA.StoreToFloat2(static_cast<float*>(values));
        EXPECT_EQ(values.GetX(), 1.0f);
        EXPECT_EQ(values.GetY(), 2.0f);
    }

    TEST(MATH_PackedVector3, TestConstructors)
    {
        EXPECT_THAT(static_cast<AZ::Vector3>(AZ::PackedVector3f(1.0f, 3.0f, 2.0f)), IsClose(AZ::Vector3(1.0f, 3.0f, 2.0f)));
        EXPECT_THAT(static_cast<AZ::Vector3>(AZ::PackedVector3f(1.0f)), IsClose(AZ::Vector3(1.0f, 1.0f, 1.0f)));

        AZ::PackedVector3f vC(1.0f, 2.0f, 4.0f);
        EXPECT_FLOAT_EQ(vC.GetX(), 1.0f);
        EXPECT_FLOAT_EQ(vC.GetY(), 2.0f);
        EXPECT_FLOAT_EQ(vC.GetZ(), 4.0f);

        AZ::PackedVector3f vb(1.0f, 2.0f, 4.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(0), 1.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(1), 2.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(2), 4.0f);
    }

    TEST(MATH_PackedVector3, TestStoreFloat)
    {
        AZ::PackedVector3f values(0.0f, 0.0f, 0.0f);

        AZ::Vector3 vA(1.0f, 2.0f, 5.0f);
        vA.StoreToFloat3(static_cast<float*>(values));
        EXPECT_EQ(values.GetX(), 1.0f);
        EXPECT_EQ(values.GetY(), 2.0f);
        EXPECT_EQ(values.GetZ(), 5.0f);
    }

    TEST(MATH_PackedVector4, TestConstructors)
    {
        EXPECT_THAT(static_cast<AZ::Vector4>(AZ::PackedVector4f(1.0f, 3.0f, 5.0f, 1.0f)), IsClose(AZ::Vector4(1.0f, 3.0f, 5.0f, 1.0f)));
        EXPECT_THAT(static_cast<AZ::Vector4>(AZ::PackedVector4f(1.0f)), IsClose(AZ::Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

        AZ::PackedVector4f vC(1.0f, 2.0f, 5.0f, 3.0f);
        EXPECT_FLOAT_EQ(vC.GetX(), 1.0f);
        EXPECT_FLOAT_EQ(vC.GetY(), 2.0f);
        EXPECT_FLOAT_EQ(vC.GetZ(), 5.0f);
        EXPECT_FLOAT_EQ(vC.GetW(), 3.0f);

        AZ::PackedVector4f vb(1.0f, 2.0f, 5.0f, 3.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(0), 1.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(1), 2.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(2), 5.0f);
        EXPECT_FLOAT_EQ(vb.GetElement(3), 3.0f);
    }

    TEST(MATH_PackedVector4, TestStoreFloat)
    {
        AZ::PackedVector4f values(0.0f, 0.0f, 0.0f, 0.0f);

        AZ::Vector4 vA(1.0f, 2.0f, 5.0f, 6.0f);
        vA.StoreToFloat4(static_cast<float*>(values));
        EXPECT_EQ(values.GetX(), 1.0f);
        EXPECT_EQ(values.GetY(), 2.0f);
        EXPECT_EQ(values.GetZ(), 5.0f);
        EXPECT_EQ(values.GetW(), 6.0f);
    }

} // namespace UnitTest
