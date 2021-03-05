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
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Shape/PolygonPrismShapeComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class PolygonPrismShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_polygonPrismShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_polygonPrismShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::PolygonPrismShapeComponent::CreateDescriptor());
            m_polygonPrismShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_polygonPrismShapeComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
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
            LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(502.0f, 585.0f, 32.0f));
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
            AZ::Transform::CreateScale(AZ::Vector3(3.0f)), 2.0f,
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
            AZ::Transform::CreateScale(AZ::Vector3(3.0f)), 1.5f,
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

    TEST_F(AllocatorsFixture, PolygonPrismFilledMeshClearedWithLessThanThreeVertices)
    {
        // given
        // invalid vertex data (less than three vertices)
        const auto vertices = AZStd::vector<AZ::Vector2>{AZ::Vector2(0.0f, 0.0f), AZ::Vector2(1.0f, 0.0f)};

        // fill polygon prism mesh with some initial triangle data (to ensure it's cleared)
        LmbrCentral::PolygonPrismMesh polygonPrismMesh;
        polygonPrismMesh.m_triangles = AZStd::vector<AZ::Vector3>{
            AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f)};

        // when
        LmbrCentral::GeneratePolygonPrismMesh(vertices, 1.0f, polygonPrismMesh);

        // then
        EXPECT_TRUE(polygonPrismMesh.m_triangles.empty());
    }
}
