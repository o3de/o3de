/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MathConversion.h>

#include <AzTest/AzTest.h>

#include <AzCore/std/sort.h>
#include <AzCore/Component/ComponentApplication.h>

#include <Source/Pipeline/PrimitiveShapeFitter/AbstractShapeParameterization.h>
#include <Source/Pipeline/PrimitiveShapeFitter/PrimitiveShapeFitter.h>

namespace PhysX::Pipeline
{
    static constexpr double FitterTolerance = 0.1;

    extern const AbstractShapeParameterization::Ptr SimpleSphere;
    extern const AbstractShapeParameterization::Ptr TransformedSphere;
    extern const AbstractShapeParameterization::Ptr DegenerateSphere;
    extern const AbstractShapeParameterization::Ptr SimpleBox;
    extern const AbstractShapeParameterization::Ptr TransformedBox;
    extern const AbstractShapeParameterization::Ptr DegenerateBox;
    extern const AbstractShapeParameterization::Ptr SimpleCapsule;
    extern const AbstractShapeParameterization::Ptr TransformedCapsule;
    extern const AbstractShapeParameterization::Ptr DegenerateCapsule;

    extern const AZStd::array<AZ::Transform, 5> TestTransforms;

    extern const AZStd::array<AZ::Vector3, 482> SphereVertices;
    extern const AZStd::array<AZ::Vector3, 14> BoxVertices;
    extern const AZStd::array<AZ::Vector3, 514> CapsuleVertices;
    extern const AZStd::array<AZ::Vector3, 8> MinimalVertices;

    #if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    // NEON has a lower float precision than SIMD/SCALAR
    static constexpr double defaultTolerance = 1.0e-4;
    #else
    static constexpr double defaultTolerance = 1.0e-6;
    #endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON


    static void ExpectNear(const AZ::Vector3& actual, const AZ::Vector3& expected, const double tolerance = defaultTolerance)
    {
        EXPECT_NEAR(actual(0), expected(0), tolerance);
        EXPECT_NEAR(actual(1), expected(1), tolerance);
        EXPECT_NEAR(actual(2), expected(2), tolerance);
    }

    static void ExpectParallel(const AZ::Vector3& actual, const AZ::Vector3& expected, const double tolerance = defaultTolerance)
    {
        if (expected.Dot(actual) > 0.0)
        {
            ExpectNear(actual, expected, tolerance);
        }
        else
        {
            ExpectNear(actual, -expected, tolerance);
        }
    }

    static void ExpectRightHandedOrthonormalBasis(
        const AZ::Vector3& xAxis,
        const AZ::Vector3& yAxis,
        const AZ::Vector3& zAxis
    )
    {
        EXPECT_NEAR(xAxis.GetLengthSq(), 1.0, defaultTolerance);
        EXPECT_NEAR(yAxis.GetLengthSq(), 1.0, defaultTolerance);
        EXPECT_NEAR(zAxis.GetLengthSq(), 1.0, defaultTolerance);

        EXPECT_NEAR(xAxis.Dot(yAxis), 0.0, defaultTolerance);
        ExpectNear(zAxis, xAxis.Cross(yAxis), defaultTolerance);
    }

    static AZStd::vector<Vec3> AZVerticesToLYVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        AZStd::vector<Vec3> converted(vertices.size(), Vec3(ZERO));

        AZStd::transform(
            vertices.begin(),
            vertices.end(),
            converted.begin(),
            [] (const AZ::Vector3& vertex) {
                return AZVec3ToLYVec3(vertex);
            }
        );

        return converted;
    }

    template<size_t N>
    static AZStd::vector<AZ::Vector3> TransformVertices(
        const AZStd::array<AZ::Vector3, N>& vertices,
        const AZ::Transform& transform
    )
    {
        AZStd::vector<AZ::Vector3> transformed(N, AZ::Vector3::CreateZero());

        AZStd::transform(
            vertices.begin(),
            vertices.end(),
            transformed.begin(),
            [&transform] (const AZ::Vector3& vertex) {
                return transform.TransformVector(vertex) + transform.GetTranslation();
            }
        );

        return transformed;
    }


    class ArgumentPackingTestFixture
        : public ::testing::TestWithParam<AbstractShapeParameterization*>
    {
    };

    TEST_P(ArgumentPackingTestFixture, ArgumentPackingTest)
    {
        AbstractShapeParameterization* shape = GetParam();

        AZStd::vector<double> argsBefore = shape->PackArguments();
        shape->UnpackArguments(argsBefore);
        AZStd::vector<double> argsAfter = shape->PackArguments();

        EXPECT_THAT(argsBefore, ::testing::SizeIs(shape->GetDegreesOfFreedom()));
        EXPECT_THAT(argsAfter, ::testing::SizeIs(shape->GetDegreesOfFreedom()));

        for (AZ::u32 i = 0; i < shape->GetDegreesOfFreedom(); ++i)
        {
            EXPECT_NEAR(argsBefore[i], argsAfter[i], 1e-6);
        }
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        ArgumentPackingTestFixture,
        ::testing::Values(
            // Spheres
            SimpleSphere.get(),
            TransformedSphere.get(),
            DegenerateSphere.get(),

            // Boxes
            SimpleBox.get(),
            TransformedBox.get(),
            DegenerateBox.get(),

            // Capsules
            SimpleCapsule.get(),
            TransformedCapsule.get(),
            DegenerateCapsule.get()
        )
    );


    struct VolumeTestData
    {
        const AbstractShapeParameterization* m_shape;
        const double m_expectedVolume;
    };

    class VolumeTestFixture
        : public ::testing::TestWithParam<VolumeTestData>
    {
    };

    TEST_P(VolumeTestFixture, VolumeTest)
    {
        const VolumeTestData& testData = GetParam();
        EXPECT_NEAR(testData.m_shape->GetVolume(), testData.m_expectedVolume, 1e-6);
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        VolumeTestFixture,
        ::testing::Values(
            // Spheres
            VolumeTestData{ SimpleSphere.get(), 4.188790205 },
            VolumeTestData{ TransformedSphere.get(), 113.0973355 },
            VolumeTestData{ DegenerateSphere.get(), 0.0 },

            // Boxes
            VolumeTestData{ SimpleBox.get(), 8.0 },
            VolumeTestData{ TransformedBox.get(), 120.0 },
            VolumeTestData{ DegenerateBox.get(), 8.0e-6 },

            // Capsules
            VolumeTestData{ SimpleCapsule.get(), 16.75516082 },
            VolumeTestData{ TransformedCapsule.get(), 16.75516082 },
            VolumeTestData{ DegenerateCapsule.get(), 6.283183213e-12 }
        )
    );


    const AZStd::array<Vector, 5> squaredDistanceTestPoints{
        Vector{{ 0.0, 0.0, 0.0 }},
        Vector{{ 1.0, 2.0, 3.0 }},
        Vector{{ -0.5, 0.0, 0.0 }},
        Vector{{ 0.0, 1.8, 4.0 }},
        Vector{{ -10.0, -10.0, -10.0 }}
    };

    struct SquaredDistanceTestData
    {
        const AbstractShapeParameterization* m_shape;
        const AZStd::array<double, 5> m_expectedSquaredDistances;
    };

    class SquaredDistanceTestFixture
        : public ::testing::TestWithParam<SquaredDistanceTestData>
    {
    };

    TEST_P(SquaredDistanceTestFixture, SquaredDistanceTest)
    {
        const SquaredDistanceTestData& testData = GetParam();

        // The following checks for an error in the test, so use a hard assert.
        ASSERT_EQ(squaredDistanceTestPoints.size(), testData.m_expectedSquaredDistances.size());

        for (AZ::u32 i = 0; i < squaredDistanceTestPoints.size(); ++i)
        {
            EXPECT_NEAR(
                testData.m_shape->SquaredDistanceToShape(squaredDistanceTestPoints[i]),
                testData.m_expectedSquaredDistances[i],
                1.0e-6
            );
        }
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        SquaredDistanceTestFixture,
        ::testing::Values(
            // Spheres
            SquaredDistanceTestData{
                SimpleSphere.get(),
                {{
                    1.0,
                    7.516685226,
                    0.25,
                    11.46731512,
                    266.3589838
                }}
            },
            SquaredDistanceTestData{
                TransformedSphere.get(),
                {{
                    0.5500556794,
                    9.0,
                    0.8192509723,
                    2.470285886,
                    318.0040001
                }}
            },
            SquaredDistanceTestData{
                DegenerateSphere.get(),
                {{
                    0.0,
                    14.0,
                    0.25,
                    19.24,
                    300.0
                }}
            },

            // Boxes
            SquaredDistanceTestData{
                SimpleBox.get(),
                {{
                    1.0,
                    5.0,
                    0.25,
                    9.64,
                    243.0
                }}
            },
            SquaredDistanceTestData{
                TransformedBox.get(),
                {{
                    1.0,
                    1.0,
                    1.0,
                    0.64,
                    259.2590209
                }}
            },
            SquaredDistanceTestData{
                DegenerateBox.get(),
                {{
                    1.0e-6,
                    9.999994,
                    1.0e-6,
                    16.639992,
                    261.99998
                }}
            },

            // Capsules
            SquaredDistanceTestData{
                SimpleCapsule.get(),
                {{
                    1.0,
                    6.788897449,
                    1.0,
                    11.46731512,
                    232.5038464
                }}
            },
            SquaredDistanceTestData{
                TransformedCapsule.get(),
                {{
                    7.222962325,
                    1.0,
                    8.409713211,
                    0.3397693547,
                    385.6204406
                }}
            },
            SquaredDistanceTestData{
                DegenerateCapsule.get(),
                {{
                    1.0e-6,
                    12.99999279,
                    1.0e-6,
                    19.23999123,
                    248.9999824
                }}
            }
        )
    );


    TEST(GetShapeConfigurationTestFixture, SimpleSphereTest)
    {
        const MeshAssetData::ShapeConfigurationPair pair = SimpleSphere->GetShapeConfigurationPair();

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Sphere);
        EXPECT_NEAR(static_cast<const Physics::SphereShapeConfiguration&>(*shapePtr).m_radius, 1.0, 1.0e-6);

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());
        ExpectNear(configPtr->m_transform->GetTranslation(), AZ::Vector3::CreateZero());
    }

    TEST(GetShapeConfigurationTestFixture, TransformedSphereTest)
    {
        const MeshAssetData::ShapeConfigurationPair pair = TransformedSphere->GetShapeConfigurationPair();

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Sphere);
        EXPECT_NEAR(static_cast<const Physics::SphereShapeConfiguration&>(*shapePtr).m_radius, 3.0, 1.0e-6);

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());
        ExpectNear(configPtr->m_transform->GetTranslation(), AZ::Vector3(1.0, 2.0, 3.0));
    }

    TEST(GetShapeConfigurationTestFixture, SimpleBoxTest)
    {
        const MeshAssetData::ShapeConfigurationPair pair = SimpleBox->GetShapeConfigurationPair();

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Box);
        ExpectNear(
            static_cast<const Physics::BoxShapeConfiguration&>(*shapePtr).m_dimensions,
            AZ::Vector3(2.0, 2.0, 2.0)
        );

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());
        ExpectNear(configPtr->m_transform->GetTranslation(), AZ::Vector3::CreateZero());
        ExpectNear(configPtr->m_transform->GetBasisX(), AZ::Vector3(1.0, 0.0, 0.0));
        ExpectNear(configPtr->m_transform->GetBasisY(), AZ::Vector3(0.0, 1.0, 0.0));
        ExpectNear(configPtr->m_transform->GetBasisZ(), AZ::Vector3(0.0, 0.0, 1.0));
    }

    TEST(GetShapeConfigurationTestFixture, TransformedBoxTest)
    {
        const MeshAssetData::ShapeConfigurationPair pair = TransformedBox->GetShapeConfigurationPair();

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Box);
        ExpectNear(
            static_cast<const Physics::BoxShapeConfiguration&>(*shapePtr).m_dimensions,
            AZ::Vector3(6.0, 2.0, 10.0)
        );

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());
        ExpectNear(configPtr->m_transform->GetTranslation(), AZ::Vector3(1.0, 2.0, 3.0));
        ExpectNear(configPtr->m_transform->GetBasisX(), AZ::Vector3(0.8660254038, 0.0, -0.5));
        ExpectNear(configPtr->m_transform->GetBasisY(), AZ::Vector3(0.0, 1.0, 0.0));
        ExpectNear(configPtr->m_transform->GetBasisZ(), AZ::Vector3(0.5, 0.0, 0.8660254038));
    }

    TEST(GetShapeConfigurationTestFixture, SimpleCapsuleTest)
    {
        const MeshAssetData::ShapeConfigurationPair pair = SimpleCapsule->GetShapeConfigurationPair();

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Capsule);
        EXPECT_NEAR(static_cast<const Physics::CapsuleShapeConfiguration&>(*shapePtr).m_height, 6.0, defaultTolerance);
        EXPECT_NEAR(static_cast<const Physics::CapsuleShapeConfiguration&>(*shapePtr).m_radius, 1.0, defaultTolerance);

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());
        ExpectNear(configPtr->m_transform->GetTranslation(), AZ::Vector3::CreateZero());

        // For capsules, the z-axis is the primary axis.
        ExpectNear(configPtr->m_transform->GetBasisZ(), AZ::Vector3(1.0, 0.0, 0.0));
    }

    TEST(GetShapeConfigurationTestFixture, TransformedCapsuleTest)
    {
        const MeshAssetData::ShapeConfigurationPair pair = TransformedCapsule->GetShapeConfigurationPair();

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Capsule);
        EXPECT_NEAR(static_cast<const Physics::CapsuleShapeConfiguration&>(*shapePtr).m_height, 6.0, defaultTolerance);
        EXPECT_NEAR(static_cast<const Physics::CapsuleShapeConfiguration&>(*shapePtr).m_radius, 1.0, defaultTolerance);

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());
        ExpectNear(configPtr->m_transform->GetTranslation(), AZ::Vector3(1.0, 2.0, 3.0));

        // For capsules, the z-axis is the primary axis.
        ExpectNear(configPtr->m_transform->GetBasisZ(), AZ::Vector3(0.8660254038, 0.0, -0.5));
    }


    class GetDegenerateShapeConfigurationTestFixture
        : public ::testing::TestWithParam<AbstractShapeParameterization*>
    {
    };

    TEST_P(GetDegenerateShapeConfigurationTestFixture, GetDegenerateShapeConfigurationTest)
    {
        const MeshAssetData::ShapeConfigurationPair pair = GetParam()->GetShapeConfigurationPair();
        EXPECT_THAT(pair.second, ::testing::IsNull());
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        GetDegenerateShapeConfigurationTestFixture,
        ::testing::Values(
            DegenerateSphere.get(),
            DegenerateBox.get(),
            DegenerateCapsule.get()
        )
    );


    class FitPrimitiveShapeTestFixture
        : public ::testing::TestWithParam<AZ::Transform>
    {
    };

    TEST_P(FitPrimitiveShapeTestFixture, SphereTest)
    {
        const AZ::Transform& transform = GetParam();

        MeshAssetData::ShapeConfigurationPair pair = FitPrimitiveShape(
            "sphere",
            AZVerticesToLYVertices(TransformVertices(SphereVertices, transform)),
            0.0,
            PrimitiveShapeTarget::Sphere
        );

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Sphere);
        EXPECT_NEAR(static_cast<const Physics::SphereShapeConfiguration&>(*shapePtr).m_radius, 1.0, FitterTolerance);

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());
        ExpectNear(configPtr->m_transform->GetTranslation(), transform.GetTranslation(), FitterTolerance);
        ExpectRightHandedOrthonormalBasis(
            configPtr->m_transform->GetBasisX(),
            configPtr->m_transform->GetBasisY(),
            configPtr->m_transform->GetBasisZ()
        );
    }

    TEST_P(FitPrimitiveShapeTestFixture, BoxTest)
    {
        const AZ::Transform& transform = GetParam();

        MeshAssetData::ShapeConfigurationPair pair = FitPrimitiveShape(
            "box",
            AZVerticesToLYVertices(TransformVertices(BoxVertices, transform)),
            0.0,
            PrimitiveShapeTarget::Box
        );

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Box);
        ExpectNear(
            static_cast<const Physics::BoxShapeConfiguration&>(*shapePtr).m_dimensions,
            AZ::Vector3(10.0, 6.0, 2.0),
            FitterTolerance
        );

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());

        const AZ::Vector3 xAxis = configPtr->m_transform->GetBasisX();
        const AZ::Vector3 yAxis = configPtr->m_transform->GetBasisY();
        const AZ::Vector3 zAxis = configPtr->m_transform->GetBasisZ();

        ExpectNear(configPtr->m_transform->GetTranslation(), transform.GetTranslation(), FitterTolerance);
        ExpectParallel(xAxis, transform.GetBasisX(), FitterTolerance);
        ExpectParallel(yAxis, transform.GetBasisY(), FitterTolerance);
        ExpectParallel(zAxis, transform.GetBasisZ(), FitterTolerance);
        ExpectRightHandedOrthonormalBasis(xAxis, yAxis, zAxis);
    }

    TEST_P(FitPrimitiveShapeTestFixture, CapsuleTest)
    {
        const AZ::Transform& transform = GetParam();

        MeshAssetData::ShapeConfigurationPair pair = FitPrimitiveShape(
            "capsule",
            AZVerticesToLYVertices(TransformVertices(CapsuleVertices, transform)),
            0.0,
            PrimitiveShapeTarget::Capsule
        );

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Capsule);
        EXPECT_NEAR(static_cast<const Physics::CapsuleShapeConfiguration&>(*shapePtr).m_height, 4.0, FitterTolerance);
        EXPECT_NEAR(static_cast<const Physics::CapsuleShapeConfiguration&>(*shapePtr).m_radius, 1.0, FitterTolerance);

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());

        const AZ::Vector3 xAxis = configPtr->m_transform->GetBasisX();
        const AZ::Vector3 yAxis = configPtr->m_transform->GetBasisY();
        const AZ::Vector3 zAxis = configPtr->m_transform->GetBasisZ();

        ExpectNear(configPtr->m_transform->GetTranslation(), transform.GetTranslation(), FitterTolerance);
        ExpectParallel(zAxis, transform.GetBasisX(), FitterTolerance);
        ExpectRightHandedOrthonormalBasis(xAxis, yAxis, zAxis);
    }

    TEST_P(FitPrimitiveShapeTestFixture, VolumeMinimizationTest)
    {
        // This test verifies that the volume minimization coefficient works as expected. The vertices used here form a
        // 2x2x2 cube centered at the origin. We let the fitter decide which primitive fits best, which should always be
        // a cube that wraps the cube snugly. Note that this test can fail for certain very specific initializations
        // which are at a local maximum with regard to the orientation parameters, so the derivatives for those parameters
        // are zero and there is never any progress in updating them. This can happen for example with a 45 degree
        // rotation about one of the axes.

        const AZ::Transform& expectedTransform = GetParam();
        AZStd::vector<AZ::Vector3> expectedVertices = TransformVertices(MinimalVertices, expectedTransform);

        MeshAssetData::ShapeConfigurationPair pair = FitPrimitiveShape(
            "minimal",
            AZVerticesToLYVertices(expectedVertices),
            5.0e-4,
            PrimitiveShapeTarget::BestFit
        );

        const AZStd::shared_ptr<AssetColliderConfiguration> configPtr = pair.first;
        const AZStd::shared_ptr<Physics::ShapeConfiguration> shapePtr = pair.second;

        // Validate shape.
        ASSERT_THAT(shapePtr, ::testing::NotNull());
        ASSERT_TRUE(shapePtr->GetShapeType() == Physics::ShapeType::Box);
        ExpectNear(
            static_cast<const Physics::BoxShapeConfiguration&>(*shapePtr).m_dimensions,
            AZ::Vector3(2.0, 2.0, 2.0),
            FitterTolerance
        );

        // Validate transform.
        ASSERT_THAT(configPtr, ::testing::NotNull());
        ASSERT_TRUE(configPtr->m_transform.has_value());

        const AZ::Transform& actualTransform = *(configPtr->m_transform);

        const AZ::Vector3 xAxis = actualTransform.GetBasisX();
        const AZ::Vector3 yAxis = actualTransform.GetBasisY();
        const AZ::Vector3 zAxis = actualTransform.GetBasisZ();

        ExpectNear(actualTransform.GetTranslation(), expectedTransform.GetTranslation(), FitterTolerance);
        ExpectRightHandedOrthonormalBasis(xAxis, yAxis, zAxis);

        // The basis vectors of the returned transform could be reflections and/or rotations of the basis vectors of the
        // expected transform. Because of this, we instead check that the returned transform moves the eight vertices of
        // the cube close to the expected vertices.
        AZStd::vector<AZ::Vector3> actualVertices = TransformVertices(MinimalVertices, actualTransform);

        // Sanity check.
        ASSERT_EQ(expectedVertices.size(), actualVertices.size());

        // Sort both sets of vertices so that we can compare them element by element. We sort them by x-coordinate
        // first, then y-coordinate and finally z-cooridnate.
        auto comparator = [] (const AZ::Vector3& lhs, const AZ::Vector3& rhs) {
            return lhs(0) < rhs(0) || (lhs(0) == rhs(0) && lhs(1) < rhs(1) || (lhs(1) == rhs(1) && lhs(2) < rhs(2)));
        };

        AZStd::sort(expectedVertices.begin(), expectedVertices.end(), comparator);
        AZStd::sort(actualVertices.begin(), actualVertices.end(), comparator);

        for (AZ::u32 i = 0; i < actualVertices.size(); ++i)
        {
            ExpectNear(expectedVertices[i], actualVertices[i], FitterTolerance);
        }
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        FitPrimitiveShapeTestFixture,
        ::testing::ValuesIn(TestTransforms)
    );
} // namespace PhysX::Pipeline
