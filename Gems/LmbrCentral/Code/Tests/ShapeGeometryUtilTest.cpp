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

#include "LmbrCentral_precompiled.h"
#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <Shape/ShapeGeometryUtil.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Vector3.h>

namespace UnitTest
{
    class ShapeGeometryUtilTest
        : public AllocatorsFixture
    {
    public:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    // ccw
    TEST_F(ShapeGeometryUtilTest, GenerateTrianglesCCW)
    {
        AZStd::vector<AZ::Vector3> triangles = 
            LmbrCentral::GenerateTriangles(
            {
                AZ::Vector2(0.0f, 0.0f), AZ::Vector2(1.0f, 0.0f),
                AZ::Vector2(1.0f, 1.0f), AZ::Vector2(0.0f, 1.0f) 
            }
        );

        EXPECT_TRUE(triangles.size() == 6);
    }

    // cw
    TEST_F(ShapeGeometryUtilTest, GenerateTrianglesCW)
    {
        AZStd::vector<AZ::Vector3> triangles =
            LmbrCentral::GenerateTriangles(
            {
                AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 1.0f),
                AZ::Vector2(1.0f, 1.0f), AZ::Vector2(1.0f, 0.0f)
            }
        );

        EXPECT_TRUE(triangles.size() == 6);
    }

    // non-simple
    TEST_F(ShapeGeometryUtilTest, GenerateTrianglesFailureNonSimple)
    {
        AZStd::vector<AZ::Vector3> triangles =
            LmbrCentral::GenerateTriangles(
            {
                AZ::Vector2(0.0f, -2.0f), AZ::Vector2(2.0f, -2.0f),
                AZ::Vector2(2.0f, 1.0f), AZ::Vector2(4.0f, 1.0f)
            }
        );

        EXPECT_TRUE(triangles.size() == 0);
    }

    // simple-concave
    TEST_F(ShapeGeometryUtilTest, GenerateTrianglesSimpleConcave)
    {
        AZStd::vector<AZ::Vector3> triangles =
            LmbrCentral::GenerateTriangles(
            {
                AZ::Vector2(1.0f, -1.0f), AZ::Vector2(0.0f, -2.0f),
                AZ::Vector2(1.0f, -2.0f), AZ::Vector2(2.0f, -2.0f),
                AZ::Vector2(2.0f, 0.0f), AZ::Vector2(2.0f, 1.0f),
                AZ::Vector2(1.0f, 0.0f), AZ::Vector2(-1.0f, 0.0f)
            }
        );

        EXPECT_TRUE(triangles.size() == 18);
    }
}
