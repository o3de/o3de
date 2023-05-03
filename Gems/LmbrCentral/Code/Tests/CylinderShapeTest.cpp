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
#include <AzCore/Math/Random.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Shape/CylinderShapeComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <ShapeThreadsafeTest.h>

namespace UnitTest
{
    class CylinderShapeTest
        : public LeakDetectionFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_cylinderShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_cylinderShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::CylinderShapeComponent::CreateDescriptor());
            m_cylinderShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_cylinderShapeComponentDescriptor.reset();
            m_serializeContext.reset();
            LeakDetectionFixture::TearDown();
        }
    };

    using CylinderParams = std::tuple<AZ::Transform, float, float>;
    using BoundingBoxResult = std::tuple<AZ::Vector3, AZ::Vector3>;
    using BoundingBoxParams = std::tuple<CylinderParams, BoundingBoxResult>;
    using IsPointInsideParams = std::tuple<CylinderParams, AZ::Vector3, bool>;
    using RayParams = std::tuple<AZ::Vector3, AZ::Vector3>;
    using RayIntersectResult = std::tuple<bool, float, float>;
    using RayIntersectParams = std::tuple<RayParams, CylinderParams, RayIntersectResult>;
    using DistanceResultParams = std::tuple<float, float>;
    using DistanceFromPointParams = std::tuple<CylinderParams, AZ::Vector3, DistanceResultParams>;

    class CylinderShapeRayIntersectTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<RayIntersectParams>
    {
    public:
        static const std::vector<RayIntersectParams> ShouldPass;
        static const std::vector<RayIntersectParams> ShouldFail;
    };

    class CylinderShapeAABBTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<BoundingBoxParams>
    {
    public:
        static const std::vector<BoundingBoxParams> ShouldPass;
    };

    class CylinderShapeTransformAndLocalBoundsTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<BoundingBoxParams>
    {
    public:
        static const std::vector<BoundingBoxParams> ShouldPass;
    };

    class CylinderShapeIsPointInsideTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<IsPointInsideParams>
    {
    public:
        static const std::vector<IsPointInsideParams> ShouldPass;
        static const std::vector<IsPointInsideParams> ShouldFail;
    }; 

    class CylinderShapeDistanceFromPointTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<DistanceFromPointParams>
    {
    public:
        static const std::vector<DistanceFromPointParams> ShouldPass;
    };

    const std::vector<RayIntersectParams> CylinderShapeRayIntersectTest::ShouldPass = {
        // Test case 0
        {   // Ray: src, dir
            { AZ::Vector3(0.0f, 5.0f, 5.0f), AZ::Vector3(0.0f, -1.0f, 0.0f) },
             // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(
             AZ::Vector3(0.0f, 0.0f, 5.0f)),
              0.5f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 4.5f, 1e-4f }   },
        // Test case 1
        {   // Ray: src, dir
            { AZ::Vector3(-10.0f, -20.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(-10.0f, -10.0f, 0.0f)),
                1.0f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 7.5f, 1e-2f }   },
        // Test case 2
        {// Ray: src, dir
            { AZ::Vector3(-10.0f, -10.0f, -10.0f), AZ::Vector3(0.0f, 0.0f, 1.0f) },
            // Cylinder: transform, radius, height
            { AZ::Transform::CreateFromQuaternionAndTranslation(
                    AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(-10.0f, -10.0f, 0.0f)),
                    1.0f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 9.0f, 1e-2f }   },
        // Test case 3
        {
            // Ray: src, dir
            { AZ::Vector3(-9.0f, -14.0f, -1.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(-14.0f, -14.0f, -1.0f)) *
                    AZ::Transform::CreateRotationY(AZ::Constants::HalfPi) *
                    AZ::Transform::CreateRotationZ(AZ::Constants::HalfPi) *
                    AZ::Transform::CreateUniformScale(4.0f),
                    1.0f, 1.25f },
            // Result: hit, distance, epsilon
            { true, 2.5f, 1e-2f }
        },
        // Test case 4
        {   // Ray: src, dir
            { AZ::Vector3(0.0f, 5.0f, 5.0f), AZ::Vector3(0.0f, -1.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(
              AZ::Vector3(0.0f, 0.0f, 5.0f)),
              0.0f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 0.0f, 1e-4f }   },
        // Test case 5
        {   // Ray: src, dir
            { AZ::Vector3(0.0f, 5.0f, 5.0f), AZ::Vector3(0.0f, -1.0f, 0.0f) },
            // Cylinder: transform, radius, height
              { AZ::Transform::CreateTranslation(
              AZ::Vector3(0.0f, 0.0f, 5.0f)),
              0.0f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 0.0f, 1e-4f }  },
        // Test case 6
        {   // Ray: src, dir
            { AZ::Vector3(0.0f, 5.0f, 5.0f), AZ::Vector3(0.0f, -1.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(
              AZ::Vector3(0.0f, 0.0f, 5.0f)),
              0.0f, 0.0f },
            // Result: hit, distance, epsilon
            { true, 0.0f, 1e-4f }   }
    };

    const std::vector<RayIntersectParams> CylinderShapeRayIntersectTest::ShouldFail = {
        // Test case 0
        {   // Ray: src, dir
            { AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(
              AZ::Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f },
            // Result: hit, distance, epsilon
            { false, 0.0f, 0.0f }   }
    }; 
        
    const std::vector<BoundingBoxParams> CylinderShapeAABBTest::ShouldPass = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(
              AZ::Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f },
            // AABB: min, max
            { AZ::Vector3(-5.0f, -15.0f, -0.5f), AZ::Vector3(5.0f, -5.0f, 0.5f) }   },
        // Test case 1
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, -10.0f, 0.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi),
            1.0f, 5.0f },
            // AABB: min, max
            { AZ::Vector3(-12.4748f, -12.4748f, -1.0f), AZ::Vector3(-7.52512f, -7.52512f, 1.0f) }   },
        // Test case 2
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, -10.0f, 10.0f)) *
            AZ::Transform::CreateUniformScale(3.5f),
            1.0f, 5.0f },
            // AABB: min, max
            { AZ::Vector3(-13.5f, -13.5f, 1.25f), AZ::Vector3(-6.5f, -6.5f, 18.75f) }   },
        // Test case 3
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            0.0f, 1.0f },
            // AABB: min, max
            { AZ::Vector3(0.0f, 0.0f, -0.5f), AZ::Vector3(0.0f, 0.0f,-0.5f) }    },
        // Test case 4
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            1.0f, 0.0f },
            // AABB: min, max
            { AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f) }    },
        // Test case 5
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            0.0f, 0.0f },
            // AABB: min, max
            { AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f) }    }
    };

    const std::vector<BoundingBoxParams> CylinderShapeTransformAndLocalBoundsTest::ShouldPass = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateIdentity(),
            5.0f, 1.0f },
            // Local bounds: min, max
            { AZ::Vector3(-5.0f, -5.0f, -0.5f), AZ::Vector3(5.0f, 5.0f, 0.5f) } },
        // Test case 1
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, -10.0f, 10.0f)) * AZ::Transform::CreateUniformScale(3.5f),
            5.0f, 5.0f },
            // Local bounds: min, max
            { AZ::Vector3(-5.0f, -5.0f, -2.5f), AZ::Vector3(5.0f, 5.0f, 2.5f) } },
        // Test case 2
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateIdentity(),
            0.0f, 5.0f },
            // Local bounds: min, max
            { AZ::Vector3(0.0f, 0.0f, -2.5f), AZ::Vector3(0.0f, 0.0f, 2.5f) }   },
        // Test case 3
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateIdentity(),
            5.0f, 0.0f },
            // Local bounds: min, max
            { AZ::Vector3(-5.0f, -5.0f, -0.0f), AZ::Vector3(5.0f, 5.0f, 0.0f) } },
        // Test case 4
        {   // Cylinder: transform, radius, height  
            { AZ::Transform::CreateIdentity(),
            0.0f, 0.0f },
            // Local bounds: min, max
            { AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f) }    },
    };

    const std::vector<IsPointInsideParams> CylinderShapeIsPointInsideTest::ShouldPass = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            {AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) *
            AZ::Transform::CreateUniformScale(2.5f),
            0.5f, 2.0f},
            // Point
            AZ::Vector3(27.0f, 28.5f, 40.0f),
            // Result
            true    },
        // Test case 1
        {   // Cylinder: transform, radius, height
            {AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateUniformScale(0.5f),
            0.5f, 2.0f},
            // Point
            AZ::Vector3(27.0f, 28.155f, 37.82f),
            // Result
            true    }
    };

    const std::vector<IsPointInsideParams> CylinderShapeIsPointInsideTest::ShouldFail = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            {AZ::Transform::CreateTranslation(AZ::Vector3(0, 0.0f, 0.0f)),
            0.0f, 1.0f},
            // Point
            AZ::Vector3(0.0f, 0.0f, 0.0f),
            // Result
            false   },
        // Test case 1
        {   // Cylinder: transform, radius, height
            {AZ::Transform::CreateTranslation(AZ::Vector3(0, 0.0f, 0.0f)),
            1.0f, 0.0f},
            // Point
            AZ::Vector3(0.0f, 0.0f, 0.0f),
            // Result
            false   },
        // Test case 2
        {   // Cylinder: transform, radius, height
            {AZ::Transform::CreateTranslation(AZ::Vector3(0, 0.0f, 0.0f)),
            0.0f, 0.0f},
            // Point
            AZ::Vector3(0.0f, 0.0f, 0.0f),
            // Result
            false   }
    }; 

    const std::vector<DistanceFromPointParams> CylinderShapeDistanceFromPointTest::ShouldPass = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateUniformScale(2.0f),
            0.5f, 4.0f },
            // Point
            AZ::Vector3(27.0f, 28.0f, 41.0f),
            // Result: distance, epsilon
            { 2.0f, 1e-2f } },
        // Test case 1
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateUniformScale(2.0f),
            0.5f, 4.0f },
            // Point
            AZ::Vector3(22.757f, 32.243f, 38.0f),
            // Result: distance, epsilon
            { 2.0f, 1e-2f } },
        // Test case 2
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            0.0f, 1.0f },
            // Point
            AZ::Vector3(0.0f, 5.0f, 0.0f),
            // Result: distance, epsilon
            { 5.0f, 1e-1f } },
        // Test case 3
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            1.0f, 0.0f },
            // Point
            AZ::Vector3(0.0f, 5.0f, 0.0f),
            // Result: distance, epsilon
            { 5.0f, 1e-2f } },
        // Test case 4
        {   // Cylinder: transform, radius, height
            { AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)),
            0.0f, 0.0f },
            // Point
            AZ::Vector3(0.0f, 5.0f, 0.0f),
            // Result: distance, epsilon
            { 5.0f, 1e-2f } }
    };

    void CreateCylinder(const AZ::Transform& transform, float radius, float height, AZ::Entity& entity)
    {
        entity.CreateComponent<LmbrCentral::CylinderShapeComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);

        LmbrCentral::CylinderShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::CylinderShapeComponentRequestsBus::Events::SetHeight, height);
        LmbrCentral::CylinderShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::CylinderShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    void CreateDefaultCylinder(const AZ::Transform& transform, AZ::Entity& entity)
    {
        CreateCylinder(transform, 10.0f, 10.0f, entity);
    }

    bool RandomPointsAreInCylinder(const AZ::RandomDistributionType distributionType)
    {
        //Apply a simple transform to put the Cylinder off the origin
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        transform.SetRotation(AZ::Quaternion::CreateRotationY(AZ::Constants::HalfPi));
        transform.SetTranslation(AZ::Vector3(5.0f, 5.0f, 5.0f));

        AZ::Entity entity;
        CreateDefaultCylinder(transform, entity);

        const AZ::u32 testPoints = 10000;
        AZ::Vector3 testPoint;
        bool testPointInVolume = false;

        //Test a bunch of random points generated with the Normal random distribution type
        //They should all end up in the volume
        for (AZ::u32 i = 0; i < testPoints; ++i)
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(testPoint, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, distributionType);

            LmbrCentral::ShapeComponentRequestsBus::EventResult(testPointInVolume, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, testPoint);

            if (!testPointInVolume)
            {
                return false;
            }
        }

        return true;
    }

    TEST_F(CylinderShapeTest, NormalDistributionRandomPointsAreInVolume)
    {
        EXPECT_TRUE(RandomPointsAreInCylinder(AZ::RandomDistributionType::Normal));
    }

    TEST_F(CylinderShapeTest, UniformRealDistributionRandomPointsAreInVolume)
    {
        EXPECT_TRUE(RandomPointsAreInCylinder(AZ::RandomDistributionType::UniformReal));
    }

    TEST_P(CylinderShapeRayIntersectTest, GetRayIntersectCylinder)
    {      
        const auto& [ray, cylinder, result] = GetParam();
        const auto& [src, dir] = ray;
        const auto& [transform, radius, height] = cylinder;
        const auto& [expectedHit, expectedDistance, epsilon] = result;

        AZ::Entity entity;
        CreateCylinder(transform, radius, height, entity);

        AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, 0.0f, 5.0f));
        
        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            src, dir, distance);
        
        EXPECT_EQ(rayHit, expectedHit);

        if (expectedHit)
        {
            EXPECT_NEAR(distance, expectedDistance, epsilon);
        }
    }

    INSTANTIATE_TEST_CASE_P(ValidIntersections,
        CylinderShapeRayIntersectTest,
        ::testing::ValuesIn(CylinderShapeRayIntersectTest::ShouldPass)
    );

    INSTANTIATE_TEST_CASE_P(InvalidIntersections,
        CylinderShapeRayIntersectTest,
        ::testing::ValuesIn(CylinderShapeRayIntersectTest::ShouldFail)
    );
    
    TEST_P(CylinderShapeAABBTest, GetAabb)
    {
        const auto& [cylinder, AABB] = GetParam();
        const auto& [transform, radius, height] = cylinder;
        const auto& [minExtent, maxExtent] = AABB;

        AZ::Entity entity;
        CreateCylinder(transform, radius, height, entity);
    
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);
    
        EXPECT_TRUE(aabb.GetMin().IsClose(minExtent));
        EXPECT_TRUE(aabb.GetMax().IsClose(maxExtent));
    }

    INSTANTIATE_TEST_CASE_P(AABB,
        CylinderShapeAABBTest,
        ::testing::ValuesIn(CylinderShapeAABBTest::ShouldPass)
    );

    TEST_P(CylinderShapeTransformAndLocalBoundsTest, GetTransformAndLocalBounds)
    {
        const auto& [cylinder, boundingBox] = GetParam();
        const auto& [transform, radius, height] = cylinder;
        const auto& [minExtent, maxExtent] = boundingBox;

        AZ::Entity entity;
        CreateCylinder(transform, radius, height, entity);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transform));
        EXPECT_TRUE(aabb.GetMin().IsClose(minExtent));
        EXPECT_TRUE(aabb.GetMax().IsClose(maxExtent));
    }

    INSTANTIATE_TEST_CASE_P(TransformAndLocalBounds,
        CylinderShapeTransformAndLocalBoundsTest,
        ::testing::ValuesIn(CylinderShapeTransformAndLocalBoundsTest::ShouldPass)
    );

    // point inside scaled
    TEST_P(CylinderShapeIsPointInsideTest, IsPointInside)
    {
        const auto& [cylinder, point, expectedInside] = GetParam();
        const auto& [transform, radius, height] = cylinder;

        AZ::Entity entity;
        CreateCylinder(transform, radius, height, entity);

        bool inside;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, point);

        EXPECT_EQ(inside, expectedInside);
    }

    INSTANTIATE_TEST_CASE_P(ValidIsPointInside,
        CylinderShapeIsPointInsideTest,
        ::testing::ValuesIn(CylinderShapeIsPointInsideTest::ShouldPass)
    );


    INSTANTIATE_TEST_CASE_P(InvalidIsPointInside,
        CylinderShapeIsPointInsideTest,
        ::testing::ValuesIn(CylinderShapeIsPointInsideTest::ShouldFail)
    );

    // distance scaled - along length
    TEST_P(CylinderShapeDistanceFromPointTest, DistanceFromPoint)
    {
        const auto& [cylinder, point, result] = GetParam();
        const auto& [transform, radius, height] = cylinder;
        const auto& [expectedDistance, epsilon] = result;

        AZ::Entity entity;
        CreateCylinder(transform, radius, height, entity);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, point);

        EXPECT_NEAR(distance, expectedDistance, epsilon);
    }

    INSTANTIATE_TEST_CASE_P(ValidIsDistanceFromPoint,
        CylinderShapeDistanceFromPointTest,
        ::testing::ValuesIn(CylinderShapeDistanceFromPointTest::ShouldPass)
    );

    TEST_F(CylinderShapeTest, ShapeHasThreadsafeGetSetCalls)
    {
        // Verify that setting values from one thread and querying values from multiple other threads in parallel produces
        // correct, consistent results.

        // Create our cylinder centered at 0 with our height and a starting radius.
        AZ::Entity entity;
        CreateCylinder(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), ShapeThreadsafeTest::MinDimension,
            ShapeThreadsafeTest::ShapeHeight, entity);

        // Define the function for setting unimportant dimensions on the shape while queries take place.
        auto setDimensionFn = [](AZ::EntityId shapeEntityId, float minDimension, uint32_t dimensionVariance, [[maybe_unused]] float height)
        {
            float radius = minDimension + aznumeric_cast<float>(rand() % dimensionVariance);
            LmbrCentral::CylinderShapeComponentRequestsBus::Event(
                shapeEntityId, &LmbrCentral::CylinderShapeComponentRequestsBus::Events::SetRadius, radius);
        };

        // Run the test, which will run multiple queries in parallel with each other and with the dimension-setting function.
        // The number of iterations is arbitrary - it's set high enough to catch most failures, but low enough to keep the test
        // time to a minimum.
        const int numIterations = 30000;
        ShapeThreadsafeTest::TestShapeGetSetCallsAreThreadsafe(entity, numIterations, setDimensionFn);
    }
} // namespace UnitTest
