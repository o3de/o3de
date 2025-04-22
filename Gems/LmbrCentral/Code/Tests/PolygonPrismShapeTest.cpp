/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <Shape/PolygonPrismShapeComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <ShapeThreadsafeTest.h>

namespace UnitTest
{
    class PolygonPrismShapeTest
        : public LeakDetectionFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_polygonPrismShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_nonUniformScaleComponentDescriptor;

    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_polygonPrismShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::PolygonPrismShapeComponent::CreateDescriptor());
            m_polygonPrismShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_nonUniformScaleComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::NonUniformScaleComponent::CreateDescriptor());
            m_nonUniformScaleComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_polygonPrismShapeComponentDescriptor.reset();
            m_nonUniformScaleComponentDescriptor.reset();
            m_serializeContext.reset();
            LeakDetectionFixture::TearDown();
        }
    };

    void CreatePolygonPrism(
        const AZ::Transform& transform, const float height, const AZStd::vector<AZ::Vector2>& vertices, AZ::Entity& entity)
    {
        entity.CreateComponent<LmbrCentral::PolygonPrismShapeComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);

        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetHeight, height);
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetVertices, vertices);
    }

    void CreatePolygonPrismWithNonUniformScale(
        const AZ::Transform& transform, const float height, const AZStd::vector<AZ::Vector2>& vertices,
        const AZ::Vector3& nonUniformScale, AZ::Entity& entity)
    {
        entity.CreateComponent<LmbrCentral::PolygonPrismShapeComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<AzFramework::NonUniformScaleComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        AZ::NonUniformScaleRequestBus::Event(entity.GetId(), &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);

        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetHeight, height);
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetVertices, vertices);
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_IsPointInside)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateIdentity(), 10.0f,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(0.0f, 10.0f),
                AZ::Vector2(10.0f, 10.0f),
                AZ::Vector2(10.0f, 0.0f)
            }), 
            entity);

        // verify point inside returns true
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(5.0f, 5.0f, 5.0f));
            EXPECT_TRUE(inside);
        }

        // verify point outside return false
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(5.0f, 5.0f, 20.0f));
            EXPECT_TRUE(!inside);
        }

        // verify points at polygon edge return true
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(0.0f, 0.0f, 0.0f));
            EXPECT_TRUE(inside);
        }

        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(0.0f, 10.0f, 0.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(10.0f, 10.0f, 0.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(10.0f, 0.0f, 0.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(5.0f, 10.0f, 0.0f));
            EXPECT_TRUE(inside);
        }

        // verify point lies just inside
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(5.0f, 9.5f, 0.0f));
            EXPECT_TRUE(inside);
        }
        // verify point lies just outside
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(5.0f, 10.1f, 0.0f));
            EXPECT_FALSE(inside);
        }

        // note: the shape and positions/transforms were defined in the editor and replicated here - this
        // gave a good way to create various test cases and replicate them here
        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateFromMatrix3x3AndTranslation(AZ::Matrix3x3::CreateIdentity(), AZ::Vector3(497.0f, 595.0f, 32.0f)));
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 9.0f),
                AZ::Vector2(6.5f, 6.5f),
                AZ::Vector2(9.0f, 0.0f),
                AZ::Vector2(6.5f, -6.5f),
                AZ::Vector2(0.0f, -9.0f),
                AZ::Vector2(-6.5f, -6.5f),
                AZ::Vector2(-9.0f, 0.0f),
                AZ::Vector2(-6.5f, 6.5f)
            }
        ));

        // verify point inside aabb but not inside polygon returns false
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(488.62f, 588.88f, 32.0f));
            EXPECT_FALSE(inside);
        }

        // verify point inside aabb and inside polygon returns true - when intersecting two vertices
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(496.62f, 595.0f, 32.0f));
            EXPECT_TRUE(inside);
        }

        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(10.0f, 0.0f),
                AZ::Vector2(5.0f, 10.0f)
            }
        ));

        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(496.62f, 595.0f, 32.0f));
            EXPECT_FALSE(inside);
        }

        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 10.0f),
                AZ::Vector2(10.0f, 10.0f),
                AZ::Vector2(5.0f, 0.0f)
            }
        ));

        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(496.62f, 595.0f, 32.0f));
            EXPECT_FALSE(inside);
        }

        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(10.0f, 0.0f),
                AZ::Vector2(5.0f, -10.0f)
            }
        ));

        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(496.62f, 595.0f, 32.0f));
            EXPECT_FALSE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(502.0f, 585.1f, 32.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(499.62f, 595.0f, 32.0f));
            EXPECT_TRUE(inside);
        }

        // U shape
        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateIdentity());
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(0.0f, 10.0f),
                AZ::Vector2(5.0f, 10.0f),
                AZ::Vector2(5.0f, 5.0f),
                AZ::Vector2(10.0f, 5.0f),
                AZ::Vector2(10.0f, 10.0f),
                AZ::Vector2(15.0f, 15.0f),
                AZ::Vector2(15.0f, 0.0f),
            }
        ));

        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(7.5f, 7.5f, 0.0f));
            EXPECT_FALSE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(12.5f, 7.5f, 0.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(12.5f, 7.5f, 12.0f));
            EXPECT_FALSE(inside);
        }

        // check polygon prism with rotation
        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateRotationX(AZ::DegToRad(45.0f)));
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetHeight, 10.0f);
        LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(-5.0f, -5.0f),
                AZ::Vector2(-5.0f, 5.0f),
                AZ::Vector2(5.0f, 5.0f),
                AZ::Vector2(5.0f, -5.0f)
            }
        ));

        // check below
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(2.0f, 3.5f, 2.0f));
            EXPECT_FALSE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(2.0f, -8.0f, -2.0f));
            EXPECT_FALSE(inside);
        }
        // check above
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(2.0f, -8.0f, 8.0f));
            EXPECT_FALSE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(2.0f, 2.0f, 8.0f));
            EXPECT_FALSE(inside);
        }
        // check inside
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(2.0f, -3.0f, 8.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(2.0f, -3.0f, -2.0f));
            EXPECT_TRUE(inside);
        }
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_IsPointInsideWithNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationY(AZ::DegToRad(45.0f)), AZ::Vector3(3.0f, 4.0f, 5.0f));
        transform.MultiplyByUniformScale(1.5f);
        const float height = 1.2f;
        const AZ::Vector3 nonUniformScale(2.0f, 1.2f, 0.5f);
        const AZStd::vector<AZ::Vector2> vertices =
        {
            AZ::Vector2(1.0f, -1.0f),
            AZ::Vector2(2.0f, 0.0f),
            AZ::Vector2(-2.0f, 1.0f),
            AZ::Vector2(-1.0f, -1.0f)
        };

        CreatePolygonPrismWithNonUniformScale(transform, height, vertices, nonUniformScale, entity);

        // several points which should be outside the prism
        auto outsidePoints = {
            AZ::Vector3(4.0f, 5.0f, 4.5f),
            AZ::Vector3(1.0f, 1.0f, 7.5f),
            AZ::Vector3(7.5f, 3.0f, 2.5f),
            AZ::Vector3(-1.0, 6.0f, 11.0f),
            AZ::Vector3(2.0f, 4.0f, 5.5f),
            AZ::Vector3(4.0f, 3.5f, 5.5f)
        };

        // several points which should be just inside the prism
        auto insidePoints = {
            AZ::Vector3(0.0f, 5.5f, 9.0f),
            AZ::Vector3(1.5f, 2.5f, 7.5f),
            AZ::Vector3(5.5f, 2.5f, 3.75f),
            AZ::Vector3(7.75f, 4.0f, 1.5f),
            AZ::Vector3(2.5f, 3.0f, 5.6f),
            AZ::Vector3(4.0f, 4.5f, 5.25f)
        };

        for (const auto& point : outsidePoints)
        {
            bool inside = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(),
                &LmbrCentral::ShapeComponentRequests::IsPointInside, point);
            EXPECT_FALSE(inside);
        }

        for (const auto& point : insidePoints)
        {
            bool inside = false;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(),
                &LmbrCentral::ShapeComponentRequests::IsPointInside, point);
            EXPECT_TRUE(inside);
        }
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_DistanceFromPoint)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateIdentity(), 
            10.0f, AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(0.0f, 10.0f),
                AZ::Vector2(10.0f, 10.0f),
                AZ::Vector2(10.0f, 0.0f)
            }), 
            entity);

        {
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(15.0f, 5.0f, 0.0f));
            EXPECT_TRUE(AZ::IsCloseMag(distance, 5.0f));
        }

        {
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(5.0f, 5.0f, 5.0f));
            EXPECT_TRUE(AZ::IsCloseMag(distance, 0.0f));
        }

        {
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(5.0f, 5.0f, 0.0f));
            EXPECT_TRUE(AZ::IsCloseMag(distance, 0.0f));
        }

        {
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(1.0f, 1.0f, -1.0f));
            EXPECT_TRUE(AZ::IsCloseMag(distance, 1.0f));
        }

        {
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(10.0f, 10.0f, 10.0f));
            EXPECT_TRUE(AZ::IsCloseMag(distance, 0.0f));
        }

        {
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(5.0f, 5.0f, 15.0f));
            EXPECT_TRUE(AZ::IsCloseMag(distance, 5.0f));
        }

        {
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(5.0f, 5.0f, -10.0f));
            EXPECT_TRUE(AZ::IsCloseMag(distance, 10.0f));
        }

        {
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(5.0f, 13.0f, 14.0f));
            EXPECT_TRUE(AZ::IsCloseMag(distance, 5.0f));
        }
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_DistanceFromPointWithNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationY(AZ::DegToRad(45.0f)), AZ::Vector3(3.0f, 4.0f, 5.0f));
        transform.MultiplyByUniformScale(1.5f);
        const float height = 1.2f;
        const AZ::Vector3 nonUniformScale(2.0f, 1.2f, 0.5f);
        const AZStd::vector<AZ::Vector2> vertices =
        {
            AZ::Vector2(1.0f, -1.0f),
            AZ::Vector2(2.0f, 0.0f),
            AZ::Vector2(-2.0f, 1.0f),
            AZ::Vector2(-1.0f, -1.0f)
        };

        CreatePolygonPrismWithNonUniformScale(transform, height, vertices, nonUniformScale, entity);

        float distance = AZ::Constants::FloatMax;

        // a point which should be closest to one of the rectangular faces of the prism
        LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(),
            &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(4.0f, 5.0f, 4.5f));
        EXPECT_NEAR(distance, 0.2562f, 1e-3f);

        // a point which should be closest to one of the edges connecting the two polygonal faces
        LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(),
            &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(1.0f, 1.0f, 7.5f));
        EXPECT_NEAR(distance, 1.2137f, 1e-3f);

        // a point which should be closest to an edge of the top polygonal face
        LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(),
            &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(7.5f, 3.0f, 2.5f));
        EXPECT_NEAR(distance, 0.6041f, 1e-3f);

        // a point which should be closest to a corner of the top polygonal face
        LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(),
            &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(-1.0, 6.0f, 11.0f));
        EXPECT_NEAR(distance, 1.2048f, 1e-3f);

        // a point which should be closest to the bottom polygonal face
        LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(),
            &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(2.0f, 4.0f, 5.5f));
        EXPECT_NEAR(distance, 0.3536f, 1e-3f);

        // a point which should be closest to the top polygonal face
        LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(),
            &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(4.0f, 3.5f, 5.5f));
        EXPECT_NEAR(distance, 0.1607f, 1e-3f);

        // several points which should be just inside the prism
        auto insidePoints = {
            AZ::Vector3(0.0f, 5.5f, 9.0f),
            AZ::Vector3(1.5f, 2.5f, 7.5f),
            AZ::Vector3(5.5f, 2.5f, 3.75f),
            AZ::Vector3(7.75f, 4.0f, 1.5f),
            AZ::Vector3(2.5f, 3.0f, 5.6f),
            AZ::Vector3(4.0f, 4.5f, 5.25f)
        };

        for (const auto& point : insidePoints)
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(distance, entity.GetId(),
                &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(point));
            EXPECT_NEAR(distance, 0.0f, 1e-3f);
        }
    }

    // ccw
    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismSuccess1)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateIdentity(),
            10.0f, AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(0.0f, 10.0f),
                AZ::Vector2(10.0f, 10.0f),
                AZ::Vector2(10.0f, 0.0f)
            }),
            entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(5.0f, 5.0f, 15.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 5.0f, 1e-2f);
    }

    // cw
    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismSuccess2)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateIdentity(),
            10.0f, AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(10.0f, 0.0f),
                AZ::Vector2(10.0f, 10.0f),
                AZ::Vector2(0.0f, 10.0f),
            }),
            entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(5.0f, 5.0f, 15.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 5.0f, 1e-2f);
    }

    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismSuccess3)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi),
                AZ::Vector3(2.0f, 0.0f, 5.0f)),
            2.0f, 
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(1.0f, 0.0f),
                AZ::Vector2(-1.0f, -2.0f),
                AZ::Vector2(-4.0f, -2.0f),
                AZ::Vector2(-6.0f, 0.0f),
                AZ::Vector2(-4.0f, 2.0f),
                AZ::Vector2(-1.0f, 2.0f)
            }),
            entity);

        {
            bool rayHit = false;
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
                AZ::Vector3(0.0f, 5.0f, 5.0f), AZ::Vector3(0.0f, -1.0f, 0.0f), distance);

            EXPECT_TRUE(rayHit);
            EXPECT_NEAR(distance, 5.0f, 1e-2f);
        }

        {
            bool rayHit = false;
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
                AZ::Vector3(0.0f, -1.0f, 9.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

            EXPECT_TRUE(rayHit);
            EXPECT_NEAR(distance, 2.0f, 1e-2f);
        }
    }

    // transformed scaled
    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismSuccess4)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 15.0f, 40.0f)) *
            AZ::Transform::CreateUniformScale(3.0f), 2.0f,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(-2.0f, -2.0f),
                AZ::Vector2(2.0f, -2.0f),
                AZ::Vector2(2.0f, 2.0f),
                AZ::Vector2(-2.0f, 2.0f)
            }),
            entity);

        {
            bool rayHit = false;
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
                AZ::Vector3(5.0f, 15.0f, 51.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

            EXPECT_TRUE(rayHit);
            EXPECT_NEAR(distance, 5.0f, 1e-2f);
        }

        {
            bool rayHit = false;
            float distance;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
                AZ::Vector3(15.0f, 15.0f, 43.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f), distance);

            EXPECT_TRUE(rayHit);
            EXPECT_NEAR(distance, 4.0f, 1e-2f);
        }
    }

    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismFailure)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateIdentity(),
            1.0f, AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(0.0f, 10.0f),
                AZ::Vector2(10.0f, 10.0f),
                AZ::Vector2(10.0f, 0.0f)
            }),
            entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-3.0f, -1.0f, 2.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(PolygonPrismShapeTest, GetRayIntersectWithNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationY(AZ::DegToRad(60.0f)), AZ::Vector3(1.0f, 2.5f, -1.0f));
        transform.MultiplyByUniformScale(2.0f);
        const float height = 1.5f;
        const AZ::Vector3 nonUniformScale(0.5f, 1.5f, 2.0f);

        const AZStd::vector<AZ::Vector2> vertices = {
            AZ::Vector2(0.0f, -2.0f),
            AZ::Vector2(2.0f, 0.0f),
            AZ::Vector2(-1.0f, 2.0f)
        };

        CreatePolygonPrismWithNonUniformScale(transform, height, vertices, nonUniformScale, entity);

        // should hit one of the rectangular faces
        bool rayHit = false;
        AZ::Vector3 rayOrigin(3.0f, 3.0f, -3.0f);
        AZ::Vector3 rayDirection = AZ::Vector3::CreateAxisZ();
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            rayOrigin, rayDirection, distance);
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 1.1340f, 1e-3f);

        // should hit a different rectangular face
        rayHit = false;
        rayOrigin = AZ::Vector3(2.0f, 2.0f, -3.0f);
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            rayOrigin, rayDirection, distance);
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.4604f, 1e-3f);

        // should hit one of the triangular end faces
        rayHit = false;
        rayOrigin = AZ::Vector3(1.0f, 1.0f, -3.0f);
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            rayOrigin, rayDirection, distance);
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 2.0f, 1e-3f);

        // should miss the prism
        rayHit = true;
        rayOrigin = AZ::Vector3(0.0f, 0.0f, -3.0f);
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            rayOrigin, rayDirection, distance);
        EXPECT_FALSE(rayHit);
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_GetAabb1)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 5.0f, 5.0f)), 10.0f,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(0.0f, 0.0f),
                AZ::Vector2(0.0f, 10.0f),
                AZ::Vector2(10.0f, 10.0f),
                AZ::Vector2(10.0f, 0.0f)
            }),
            entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(5.0f, 5.0f, 5.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(15.0f, 15.0f, 15.0f)));
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_GetAabb2)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi) *
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi),
                AZ::Vector3(5.0f, 15.0f, 20.0f)), 5.0f,
                AZStd::vector<AZ::Vector2>(
                {
                    AZ::Vector2(-2.0f, -2.0f),
                    AZ::Vector2(2.0f, -2.0f),
                    AZ::Vector2(2.0f, 2.0f),
                    AZ::Vector2(-2.0f, 2.0f)
                }),
            entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(3.5857f, 10.08578f, 17.5857f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(9.9497f, 17.41413f, 24.9142f)));
    }

    // transformed scaled
    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_GetAabb3)
    {
        AZ::Entity entity;
        CreatePolygonPrism(
            AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 15.0f, 40.0f)) *
            AZ::Transform::CreateUniformScale(3.0f), 1.5f,
            AZStd::vector<AZ::Vector2>(
            {
                AZ::Vector2(-2.0f, -2.0f),
                AZ::Vector2(2.0f, -2.0f),
                AZ::Vector2(2.0f, 2.0f),
                AZ::Vector2(-2.0f, 2.0f)
            }),
            entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-1.0f, 9.0f, 40.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(11.0f, 21.0f, 44.5f)));
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_GetAabbWithNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationX(AZ::DegToRad(30.0f)), AZ::Vector3(2.0f, -5.0f, 3.0f));
        transform.MultiplyByUniformScale(2.0f);
        const float height = 1.2f;
        const AZ::Vector3 nonUniformScale(1.5f, 0.8f, 2.0f);
        const AZStd::vector<AZ::Vector2> vertices =
        {
            AZ::Vector2(-2.0f, -2.0f),
            AZ::Vector2(1.0f, 0.0f),
            AZ::Vector2(2.0f, 3.0f),
            AZ::Vector2(-1.0f, 4.0f),
            AZ::Vector2(-3.0f, 2.0f)
        };

        CreatePolygonPrismWithNonUniformScale(transform, height, vertices, nonUniformScale, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-7.0f, -10.171281f, 1.4f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(8.0f, 0.542563f, 10.356922f)));
    }

    TEST_F(PolygonPrismShapeTest, CopyingPolygonPrismDoesNotAssertInEbusSystem)
    {
        AZ::EntityId testEntityId{ 42 };
        LmbrCentral::PolygonPrismShape sourceShape;
        sourceShape.Activate(testEntityId);
        // The assignment shouldn't assert in the EBusEventHandler::BusConnect call 
        LmbrCentral::PolygonPrismShape targetShape;
        AZ_TEST_START_TRACE_SUPPRESSION;
        targetShape = sourceShape;
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
        // The copy constructor also should assert
        AZ_TEST_START_TRACE_SUPPRESSION;
        LmbrCentral::PolygonPrismShape copyShape(sourceShape);
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
        sourceShape.Deactivate();
    }

    TEST_F(LeakDetectionFixture, PolygonPrismFilledMeshClearedWithLessThanThreeVertices)
    {
        // given
        // invalid vertex data (less than three vertices)
        const auto vertices = AZStd::vector<AZ::Vector2>{AZ::Vector2(0.0f, 0.0f), AZ::Vector2(1.0f, 0.0f)};

        // fill polygon prism mesh with some initial triangle data (to ensure it's cleared)
        LmbrCentral::PolygonPrismMesh polygonPrismMesh;
        polygonPrismMesh.m_triangles = AZStd::vector<AZ::Vector3>{
            AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f)};

        // when
        const AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        LmbrCentral::GeneratePolygonPrismMesh(vertices, 1.0f, nonUniformScale, polygonPrismMesh);

        // then
        EXPECT_TRUE(polygonPrismMesh.m_triangles.empty());
    }

    TEST_F(PolygonPrismShapeTest, ShapeHasThreadsafeGetSetCalls)
    {
        // Verify that setting values from one thread and querying values from multiple other threads in parallel produces
        // correct, consistent results.

        // Create our polygon prism centered at 0 with our height and a starting radius.
        AZ::Entity entity;

        const AZ::Vector2 baseVertices[] = { AZ::Vector2(-1.0f, -1.0f),
                                             AZ::Vector2( 1.0f, -1.0f),
                                             AZ::Vector2( 1.0f,  1.0f),
                                             AZ::Vector2(-1.0f,  1.0f)};

        CreatePolygonPrism(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), ShapeThreadsafeTest::ShapeHeight / 2.0f,
            AZStd::vector<AZ::Vector2>({ baseVertices[0] * ShapeThreadsafeTest::MinDimension,
                                         baseVertices[1] * ShapeThreadsafeTest::MinDimension,
                                         baseVertices[2] * ShapeThreadsafeTest::MinDimension,
                                         baseVertices[3] * ShapeThreadsafeTest::MinDimension }),
            entity);

        // Define the function for setting unimportant dimensions on the shape while queries take place.
        auto setDimensionFn = [baseVertices](AZ::EntityId shapeEntityId, float minDimension, uint32_t dimensionVariance, float height)
        {
            LmbrCentral::PolygonPrismShapeComponentRequestBus::Event(
                shapeEntityId, &LmbrCentral::PolygonPrismShapeComponentRequestBus::Events::SetHeight, height / 2.0f);

            AZ::PolygonPrismPtr polygonPrism;
            LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(
                polygonPrism, shapeEntityId, &LmbrCentral::PolygonPrismShapeComponentRequestBus::Events::GetPolygonPrism);
            for (size_t index = 0; index < 4; index++)
            {
                float vertexScale = minDimension + aznumeric_cast<float>(rand() % dimensionVariance);
                polygonPrism->m_vertexContainer.UpdateVertex(index, baseVertices[index] * vertexScale);
            }
        };

        // Run the test, which will run multiple queries in parallel with each other and with the dimension-setting function.
        // The number of iterations is arbitrary - it's set high enough to catch most failures, but low enough to keep the test
        // time to a minimum.
        const int numIterations = 30000;
        ShapeThreadsafeTest::TestShapeGetSetCallsAreThreadsafe(entity, numIterations, setDimensionFn);
    }

    TEST_F(PolygonPrismShapeTest, StaleCallbacksAreNotCalledDuringActivation)
    {
        // If callbacks are set on the underlying polygon prism for the PolygonPrismShape Component, they should get cleared out
        // and reset on every deactivation / activation. There was previously a bug in which stale callbacks would get triggered
        // during the Activate call before getting cleared out at the end of Activate().

        // Create a simple polygon prism component.

        AZ::Entity entity;
        constexpr float ShapeHeight = 2.0f;
        CreatePolygonPrism(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), ShapeHeight,
            AZStd::vector<AZ::Vector2>(
                { AZ::Vector2(-2.0f, -2.0f), AZ::Vector2(2.0f, -2.0f), AZ::Vector2(2.0f, 2.0f), AZ::Vector2(-2.0f, 2.0f) }),
            entity);

        // Set the callbacks on the underlying polygonPrism shape so that we can detect when they get called.

        AZ::PolygonPrismPtr polygonPrism;
        LmbrCentral::PolygonPrismShapeComponentRequestBus::EventResult(
            polygonPrism, entity.GetId(), &LmbrCentral::PolygonPrismShapeComponentRequestBus::Events::GetPolygonPrism);

        int numCalls = 0;
        auto notificationCallback = [&numCalls]()
        {
            numCalls++;
        };

        auto vertexNotificationCallback = [&numCalls]([[maybe_unused]] size_t index)
        {
            numCalls++;
        };

        polygonPrism->SetCallbacks(
            vertexNotificationCallback, vertexNotificationCallback, vertexNotificationCallback,
            notificationCallback, notificationCallback, notificationCallback, notificationCallback);

        // Deactivate the component.
        entity.Deactivate();

        // No callbacks should have been triggered during the deactivate.
        EXPECT_EQ(numCalls, 0);

        // Activate the component.
        entity.Activate();

        // Our callbacks should not have been triggered during an activation.
        EXPECT_EQ(numCalls, 0);

        // Verify that setting the height at this point doesn't trigger our callbacks - they should have been reset back to default
        // during the component activation.
        polygonPrism->SetHeight(ShapeHeight + 1.0f);
        EXPECT_EQ(numCalls, 0);
    }
} // namespace UnitTest
