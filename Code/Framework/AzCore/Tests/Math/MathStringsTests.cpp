/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

namespace UnitTest
{
    class MathStrings : public AllocatorsTestFixture
    {

    };

    TEST_F(MathStrings, TestVectorStringConverters)
    {
        AZ::Vector2 vec2 = AZ::Vector2::CreateOne();
        AZ::Vector3 vec3 = AZ::Vector3::CreateOne();
        AZ::Vector4 vec4 = AZ::Vector4::CreateOne();
        AZ::Quaternion quat = AZ::Quaternion::CreateIdentity();

        EXPECT_EQ(AZStd::to_string(vec2), "1.00000000,1.00000000");
        EXPECT_EQ(AZStd::to_string(vec3), "1.00000000,1.00000000,1.00000000");
        EXPECT_EQ(AZStd::to_string(vec4), "1.00000000,1.00000000,1.00000000,1.00000000");
        EXPECT_EQ(AZStd::to_string(quat), "0.00000000,0.00000000,0.00000000,1.00000000");


        EXPECT_EQ(AZStd::to_string(vec2, AZStd::MathStringFormat::Vector2ScriptFormat), "(x=1.0000000,y=1.0000000)");
        EXPECT_EQ(AZStd::to_string(vec3, AZStd::MathStringFormat::Vector3ScriptFormat), "(x=1.0000000,y=1.0000000,z=1.0000000");
        EXPECT_EQ(AZStd::to_string(vec4, AZStd::MathStringFormat::Vector4ScriptFormat), "(x=1.0000000,y=1.0000000,z=1.0000000,w=1.0000000)");
        EXPECT_EQ(AZStd::to_string(quat, AZStd::MathStringFormat::QuaternionScriptFormat), "(x=0.0000000,y=0.0000000,z=0.0000000,w=1.0000000)");
    }

    TEST_F(MathStrings, TestMatrixStringConverters)
    {
        AZ::Matrix3x3 mat33 = AZ::Matrix3x3::CreateIdentity();
        AZ::Matrix4x4 mat44 = AZ::Matrix4x4::CreateIdentity();
        AZ::Transform xform = AZ::Transform::CreateIdentity();

        EXPECT_EQ(AZStd::to_string(mat33), "1.00000000,0.00000000,0.00000000\n0.00000000,1.00000000,0.00000000\n0.00000000,0.00000000,1.00000000");
        EXPECT_EQ(AZStd::to_string(mat44), "1.00000000,0.00000000,0.00000000,0.00000000\n0.00000000,1.00000000,0.00000000,0.00000000\n0.00000000,0.00000000,1.00000000,0.00000000\n0.00000000,0.00000000,0.00000000,1.00000000");
        EXPECT_EQ(AZStd::to_string(xform), "1.00000000,0.00000000,0.00000000\n0.00000000,1.00000000,0.00000000\n0.00000000,0.00000000,1.00000000\n0.00000000,0.00000000,0.00000000");

        EXPECT_EQ(AZStd::to_string(mat33, AZStd::MathStringFormat::Matrix3x3ScriptFormat), "(x=1.0000000,y=0.0000000,z=0.0000000),(x=0.0000000,y=1.0000000,z=0.0000000),(x=0.0000000,y=0.0000000,z=1.0000000)");
        EXPECT_EQ(AZStd::to_string(mat44, AZStd::MathStringFormat::Matrix4x4ScriptFormat), "(x=1.0000000,y=0.0000000,z=0.0000000,w=0.0000000),(x=0.0000000,y=1.0000000,z=0.0000000,w=0.0000000),(x=0.0000000,y=0.0000000,z=1.0000000,w=0.0000000),(x=0.0000000,y=0.0000000,z=0.0000000,w=1.0000000)");
    }

    TEST_F(MathStrings, TestAabbStringConverter)
    {
        AZ::Aabb aabb = AZ::Aabb::CreateFromMinMaxValues(0.f, 0.f, 0.f, 1.f, 1.f, 1.f);
        EXPECT_EQ(AZStd::to_string(aabb), "0.00000000,0.00000000,0.00000000\n1.00000000,1.00000000,1.00000000");
    }

    TEST_F(MathStrings, TestColorStringConverter)
    {
        EXPECT_EQ(AZStd::to_string(AZ::Colors::Black), "R:0, G:0, B:0 A:255");
        EXPECT_EQ(AZStd::to_string(AZ::Colors::Black,AZStd::MathStringFormat::ColorScriptFormat), "(r=0,g=0,b=0,a=255)");
    }
}
