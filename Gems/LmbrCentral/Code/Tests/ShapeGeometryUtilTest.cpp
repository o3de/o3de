/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzTest/AzTest.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Shape/ShapeGeometryUtil.h>

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

    // thin
    TEST_F(ShapeGeometryUtilTest, GenerateTrianglesThin)
    {
        // given a series of vertices that are known to cause an infinite loop in the past
        // due to numerical precision issues with very thin triangles
        AZStd::vector<AZ::Vector3> triangles =
            LmbrCentral::GenerateTriangles(
            {
                AZ::Vector2( 2.00000000f, -1.50087357f),
                AZ::Vector2( 2.00000000f, -1.24706364f),
                AZ::Vector2( 1.99930608f, -0.999682188f),
                AZ::Vector2( 1.99859631f, -0.746669292f),
                AZ::Vector2( 1.99789453f, -0.496492654f),
                AZ::Vector2( 1.89999998f, 34.4000015f),
                AZ::Vector2( 1.95483327f, 0.787139893f),
                AZ::Vector2( 1.95505607f, 0.650562286f),
                AZ::Vector2( 1.95553458f, 0.357242584f),
                AZ::Vector2( 1.95596826f, 0.0913925171f),
                AZ::Vector2( 1.95620418f, -0.0532035828f),
                AZ::Vector2( 1.95642424f, -0.188129425f),
                AZ::Vector2( 1.95684254f, -0.444545746f),
                AZ::Vector2( 1.95693028f, -0.498298645f),
                AZ::Vector2( 1.95734584f, -0.753005981f),
                AZ::Vector2( 1.95775008f, -1.00079727f),
                AZ::Vector2( 1.95814919f, -1.24542999f),
                AZ::Vector2( 1.95856297f, -1.49910200f)
            }
        );

        // expect the algorithm completes and produces triangles (num verts - 2) * 3
        EXPECT_TRUE(triangles.size() == 48);
    }

    // test double to record if DrawTrianglesIndexed or DrawLines are called
    class DebugShapeDebugDisplayRequests : public AzFramework::DebugDisplayRequests
    {
    public:
        void DrawTrianglesIndexed(
            [[maybe_unused]] const AZStd::vector<AZ::Vector3>& vertices,
            [[maybe_unused]] const AZStd::vector<AZ::u32>& indices,
            [[maybe_unused]] const AZ::Color& color) override
        {
            m_drawTrianglesIndexedCalled = true;
        }

        void DrawLines([[maybe_unused]] const AZStd::vector<AZ::Vector3>& lines, [[maybe_unused]] const AZ::Color& color) override
        {
            m_drawLinesCalled = true;
        }

        bool m_drawTrianglesIndexedCalled = false;
        bool m_drawLinesCalled = false;
    };

    // DrawShape internally calls DrawTrianglesIndexed and DrawLines - with no geometry
    // we want to make sure the shape is not submitted to be drawn
    TEST(ShapeGeometry, Shape_not_attempted_to_be_drawn_with_no_geometry)
    {
        using ::testing::Eq;

        // given
        DebugShapeDebugDisplayRequests debugDisplayRequests;

        // when
        LmbrCentral::DrawShape(
            debugDisplayRequests, LmbrCentral::ShapeDrawParams{ AZ::Colors::White, AZ::Colors::White, true }, LmbrCentral::ShapeMesh{});

        // then
        EXPECT_THAT(debugDisplayRequests.m_drawTrianglesIndexedCalled, Eq(false));
        EXPECT_THAT(debugDisplayRequests.m_drawLinesCalled, Eq(false));
    }
}
