/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "CrySystem_precompiled.h"
#include <AzTest/AzTest.h>

#include <MathConversion.h>
#include <Cry_Quat.h>
#include <Cry_Matrix34.h>

//namespace MathConversionUnitTests
//{
const float kEpsilon = 0.01f;

bool IsNearlyEqual(const AZ::Vector3& az, const Vec3& ly)
{
    return fcmp(az.GetX(), ly.x, kEpsilon)
           && fcmp(az.GetY(), ly.y, kEpsilon)
           && fcmp(az.GetZ(), ly.z, kEpsilon);
}

bool IsNearlyEqual(const AZ::Quaternion& az, const Quat& ly)
{
    return fcmp(az.GetX(), ly.v.x, kEpsilon)
           && fcmp(az.GetY(), ly.v.y, kEpsilon)
           && fcmp(az.GetZ(), ly.v.z, kEpsilon)
           && fcmp(az.GetW(), ly.w, kEpsilon);
}

bool IsNearlyEqual(const AZ::Transform& az, const Matrix34& ly)
{
    float azFloats[12];
    const AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(az);
    matrix3x4.StoreToRowMajorFloat12(azFloats);

    const float* lyFloats = ly.GetData();

    for (int i = 0; i < 12; ++i)
    {
        if (!fcmp(azFloats[i], lyFloats[i], kEpsilon))
        {
            return false;
        }
    }
    return true;
}

bool IsNearlyEqual(const AZ::Transform& az, const QuatT& ly)
{
    return IsNearlyEqual(az.GetTranslation(), ly.t)
           && IsNearlyEqual(az.GetRotation(), ly.q);
}

TEST(MathConversionTests, BasicConversions)
{
    {     // check vector3 comparisons
        AZ::Vector3 az(1.f, 2.f, 3.f);
        Vec3 ly(1.f, 2.f, 3.f);
        EXPECT_TRUE(IsNearlyEqual(az, ly));

        // reverse XYZ
        ly = Vec3(3.f, 2.f, 1.f);
        EXPECT_TRUE(!IsNearlyEqual(az, ly));

        // off by 0.1
        ly = Vec3(1.1f, 2.1f, 3.1f);
        EXPECT_TRUE(!IsNearlyEqual(az, ly));
    }

    {     // check vector3 conversions
        Vec3 ly1(1.f, 2.f, 3.f);
        AZ::Vector3 az = LYVec3ToAZVec3(ly1);
        EXPECT_TRUE(IsNearlyEqual(az, ly1));

        Vec3 ly2 = AZVec3ToLYVec3(az);
        EXPECT_TRUE(IsNearlyEqual(az, ly1));
        EXPECT_TRUE(ly1.IsEquivalent(ly2));
    }

    {     // check quaternion comparisons
        AZ::Quaternion az(AZ::Quaternion::CreateIdentity());
        Quat ly(IDENTITY);
        EXPECT_TRUE(IsNearlyEqual(az, ly));

        az = AZ::Quaternion(1.f, 2.f, 3.f, 4.f);
        ly = Quat(4.f, 1.f, 2.f, 3.f);
        EXPECT_TRUE(IsNearlyEqual(az, ly));

        // w in wrong place
        ly = Quat(1.f, 2.f, 3.f, 4.f);
        EXPECT_TRUE(!IsNearlyEqual(az, ly));
    }

    {     // check quaternion conversions
        Quat ly1(4.f, 1.f, 2.f, 3.f);
        AZ::Quaternion az = LYQuaternionToAZQuaternion(ly1);
        EXPECT_TRUE(IsNearlyEqual(az, ly1));

        Quat ly2 = AZQuaternionToLYQuaternion(az);
        EXPECT_TRUE(IsNearlyEqual(az, ly2));
        EXPECT_TRUE(Quat::IsEquivalent(ly1, ly2));
    }

    {     // check transform comparisons
        AZ::Transform az = AZ::Transform::Identity();
        Matrix34 ly = Matrix34::CreateIdentity();
        EXPECT_TRUE(IsNearlyEqual(az, ly));

        // rotating pi/2 will get us a non-symmetric matrix.
        // good for testing that we're not confusing rows & columns
        float rotation = gf_PI / 2.f;

        ly = Matrix34::CreateRotationX(rotation, Vec3(1.f, 2.f, 3.f));
        az = AZ::Transform::CreateRotationX(rotation);
        az.SetTranslation(1.f, 2.f, 3.f);
        EXPECT_TRUE(IsNearlyEqual(az, ly));

        // rotate around different axis
        ly = Matrix34::CreateRotationY(rotation, Vec3(1.f, 2.f, 3.f));
        EXPECT_TRUE(!IsNearlyEqual(az, ly));
    }

    {     // check transform conversions
        Matrix34 ly1 = Matrix34::CreateRotationXYZ(Ang3(0.1f, 0.5f, 0.9f), Vec3(1.f, 2.f, 3.f));
        AZ::Transform az = LYTransformToAZTransform(ly1);
        EXPECT_TRUE(IsNearlyEqual(az, ly1));

        Matrix34 ly2 = AZTransformToLYTransform(az);
        EXPECT_TRUE(IsNearlyEqual(az, ly2));
        EXPECT_TRUE(Matrix34::IsEquivalent(ly1, ly2));
    }

    {     // check QuatT comparisons
        AZ::Transform az = AZ::Transform::Identity();
        QuatT ly(IDENTITY);
        EXPECT_TRUE(IsNearlyEqual(az, ly));

        az = AZ::Transform::CreateRotationX(AZ::Constants::HalfPi);
        az.SetTranslation(1.f, 2.f, 3.f);
        ly.q.SetRotationX(AZ::Constants::HalfPi);
        ly.t.Set(1.f, 2.f, 3.f);
        EXPECT_TRUE(IsNearlyEqual(az, ly));

        // off by 0.1
        ly.t.z += 0.1f;
        EXPECT_TRUE(!IsNearlyEqual(az, ly));
    }

    {     // check QuatT conversions
        QuatT ly1(Quat::CreateRotationX(AZ::Constants::HalfPi), Vec3(5.f, 6.f, 7.f));
        AZ::Transform az = LYQuatTToAZTransform(ly1);
        EXPECT_TRUE(IsNearlyEqual(az, ly1));

        QuatT ly2 = AZTransformToLYQuatT(az);
        EXPECT_TRUE(IsNearlyEqual(az, ly2));
        EXPECT_TRUE(QuatT::IsEquivalent(ly1, ly2));
    }
}
//} // namespace MathConversionUnitTests
