/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzTest/AzTest.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Math/MathUtils.h>
#include <Tests/GradientSignalTestFixtures.h>

#include <GradientSignal/Components/MixedGradientComponent.h>
#include <GradientSignal/Components/ReferenceGradientComponent.h>
#include <GradientSignal/Components/ShapeAreaFalloffGradientComponent.h>
#include <GradientSignal/Components/SurfaceAltitudeGradientComponent.h>
#include <GradientSignal/Components/SurfaceMaskGradientComponent.h>
#include <GradientSignal/Components/SurfaceSlopeGradientComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>
#include <GradientSignal/Components/ConstantGradientComponent.h>
#include <GradientSignal/Components/DitherGradientComponent.h>
#include <GradientSignal/Components/ImageGradientComponent.h>

namespace UnitTest
{
    struct GradientSignalReferencesTestsFixture
        : public GradientSignalTest
    {
        void TestMixedGradientComponent(int dataSize, const AZStd::vector<float>& layer1Data, const AZStd::vector<float>& layer2Data,
                                        const AZStd::vector<float>& expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation operation, float opacity)
        {
            auto mockLayer1 = CreateEntity();
            const AZ::EntityId id1 = mockLayer1->GetId();
            MockGradientArrayRequestsBus mockLayer1GradientRequestsBus(id1, layer1Data, dataSize);

            auto mockLayer2 = CreateEntity();
            const AZ::EntityId id2 = mockLayer2->GetId();
            MockGradientArrayRequestsBus mockLayer2GradientRequestsBus(id2, layer2Data, dataSize);

            GradientSignal::MixedGradientConfig config;

            GradientSignal::MixedGradientLayer layer;
            layer.m_enabled = true;

            layer.m_operation = GradientSignal::MixedGradientLayer::MixingOperation::Initialize;
            layer.m_gradientSampler.m_gradientId = mockLayer1->GetId();
            layer.m_gradientSampler.m_opacity = 1.0f;
            config.m_layers.push_back(layer);

            layer.m_operation = operation;
            layer.m_gradientSampler.m_gradientId = mockLayer2->GetId();
            layer.m_gradientSampler.m_opacity = opacity;
            config.m_layers.push_back(layer);

            auto entity = CreateEntity();
            entity->CreateComponent<GradientSignal::MixedGradientComponent>(config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }

        void TestSurfaceSlopeGradientComponent(int dataSize, const AZStd::vector<float>& inputAngles, const AZStd::vector<float>& expectedOutput, 
                                                float slopeMin, float slopeMax, GradientSignal::SurfaceSlopeGradientConfig::RampType rampType,
                                                float falloffMidpoint, float falloffRange, float falloffStrength)
        {
            auto surfaceEntity = CreateEntity();
            auto mockSurface = surfaceEntity->CreateComponent<MockSurfaceProviderComponent>();
            mockSurface->m_bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(aznumeric_cast<float>(dataSize)));
            mockSurface->m_tags.emplace_back("test_mask");

            AzFramework::SurfaceData::SurfacePoint point;

            // Fill our mock surface with the correct normal value for each point based on our test angle set.
            for (int y = 0; y < dataSize; y++)
            {
                for (int x = 0; x < dataSize; x++)
                {
                    float angle = AZ::DegToRad(inputAngles[(y * dataSize) + x]);
                    point.m_position = AZ::Vector3(aznumeric_cast<float>(x), aznumeric_cast<float>(y), 0.0f);
                    point.m_normal = AZ::Vector3(sinf(angle), 0.0f, cosf(angle));
                    mockSurface->m_surfacePoints[AZStd::make_pair(static_cast<float>(x), static_cast<float>(y))] =
                        AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&point, 1);
                }
            }
            ActivateEntity(surfaceEntity.get());

            GradientSignal::SurfaceSlopeGradientConfig config;
            config.m_slopeMin = slopeMin;
            config.m_slopeMax = slopeMax;
            config.m_rampType = rampType;
            config.m_smoothStep.m_falloffMidpoint = falloffMidpoint;
            config.m_smoothStep.m_falloffRange = falloffRange;
            config.m_smoothStep.m_falloffStrength = falloffStrength;

            auto entity = CreateEntity();
            entity->CreateComponent<GradientSignal::SurfaceSlopeGradientComponent>(config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }

    };

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationInitialize)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Initialize" with an opacity of 0.5f.
        // The output should exactly match the second layer at an opacity of 0.5f.  (i.e. doesn't blend with layer 1, just overwrites)

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.06f, 0.16f, 0.26f,
            0.46f, 0.56f, 0.66f,
            0.86f, 0.94f, 0.96f
        };

        // These values should be layer 2 * 0.5f, with no influence from layer 1.
        AZStd::vector<float> expectedOutput =
        {
            0.03f, 0.08f, 0.13f,
            0.23f, 0.28f, 0.33f,
            0.43f, 0.47f, 0.48f
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Initialize, 0.5f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationNormal)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Normal" with an opacity of 0.5f.
        // Unlike "Initialize", this should blend the two layers based on the opacity.

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.06f, 0.16f, 0.26f,
            0.46f, 0.56f, 0.66f,
            0.86f, 0.94f, 0.96f
        };

        // These values should be layer 2 * 0.5f, with no influence from layer 1.
        AZStd::vector<float> expectedOutput =
        {
            0.03f, 0.13f, 0.23f,
            0.43f, 0.53f, 0.63f,
            0.83f, 0.92f, 0.98f
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Normal, 0.5f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationMin)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Min".
        // Tests a < b, a = b, a > b, and extreme ranges (0's and 1's)

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.0f, 1.0f, 1.0f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.2f, 0.2f, 0.2f,
            0.4f, 0.4f, 0.4f,
            1.0f, 0.0f, 1.0f
        };
        AZStd::vector<float> expectedOutput = 
        {
            0.0f, 0.1f, 0.2f,       // layer 1 <= layer 2
            0.4f, 0.4f, 0.4f,       // layer 2 <= layer 1
            0.0f, 0.0f, 1.0f        // test the extremes
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Min, 1.0f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationMax)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Max".
        // Tests a < b, a = b, a > b, and extreme ranges (0's and 1's)

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.0f, 1.0f, 1.0f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.2f, 0.2f, 0.2f,
            0.4f, 0.4f, 0.4f,
            1.0f, 0.0f, 1.0f
        };
        AZStd::vector<float> expectedOutput =
        {
            0.2f, 0.2f, 0.2f,       // layer 2 >= layer 1
            0.4f, 0.5f, 0.6f,       // layer 1 >= layer 2
            1.0f, 1.0f, 1.0f        // test the extremes
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Max, 1.0f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationAdd)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Add".
        // Tests a + b = 0, a + b < 1, a + b = 1, and a + b > 1 (clamps to 1)

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.0f, 0.1f, 0.1f,
            0.4f, 0.4f, 0.4f,
            0.6f, 0.6f, 1.0f
        };
        AZStd::vector<float> expectedOutput = 
        {
            0.0f, 0.2f, 0.3f,
            0.8f, 0.9f, 1.0f,
            1.0f, 1.0f, 1.0f
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Add, 1.0f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationSubtract)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Subtract".
        // Tests a - b = 0, a - b = 1, a - b > 0, and a - b < 0 (clamps to 0)

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.3f, 1.0f,
            0.5f, 0.7f, 1.0f,
            0.5f, 0.4f, 0.3f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.0f, 0.3f, 0.0f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> expectedOutput = 
        {
            0.0f, 0.0f, 1.0f,       // a - b = 0, a - b = 0, a - b = 1
            0.1f, 0.2f, 0.4f,       // a - b > 0 
            0.0f, 0.0f, 0.0f        // a - b < 0
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Subtract, 1.0f);
    }
    
    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationMultiply)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Multiply".
        // Tests a * 0 = 0, 0 * b = 0, a * 1 = a, 1 * b = b, a * b < 1

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.0f,
            0.4f, 1.0f, 1.0f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.0f, 0.0f, 0.2f,
            1.0f, 0.5f, 1.0f,
            0.6f, 0.3f, 0.5f
        };
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f, 0.0f,       // 0 * 0 = 0, a * 0 = 0, 0 * b = 0
            0.4f, 0.5f, 1.0f,       // a * 1 = a, 1 * b = b, 1 * 1 = 1
            0.48f, 0.27f, 0.5f      // a * b = c
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Multiply, 1.0f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationScreen)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Screen".
        // Screen is defined as "1 - (1 - a) * (1 - b)"

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.0f,
            0.4f, 1.0f, 1.0f,
            0.8f, 0.9f, 0.2f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.0f, 0.0f, 0.2f,
            1.0f, 0.5f, 1.0f,
            0.6f, 0.3f, 0.4f
        };
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.1f, 0.2f,       // 1 - (1 - 0) * (1 - 0) = 0, 1 - (1 - a) * (1 - 0) = a, 1 - (1 - 0) * (1 - b) = b
            1.0f, 1.0f, 1.0f,       // 1 - (1 - a) * (1 - 1) = 1, 1 - (1 - 1) * (1 - b) = 1, 1 - (1 - 1) * (1 - 1) = 1
            0.92f, 0.93f, 0.52f     // 1 - (1 - a) * (1 - b) = c where c >= a and c >= b
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Screen, 1.0f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationAverage)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Average".
        // Tests a < b, a > b, a = b, 0, 1

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.0f, 0.5f, 0.6f,
            0.2f, 0.0f, 0.2f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> expectedOutput = 
        {
            0.0f, 0.3f, 0.4f,    // 0, a < b
            0.3f, 0.25f, 0.4f,   // a > b
            0.8f, 0.9f, 1.0f     // a = b
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Average, 1.0f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, MixedGradientComponent_OperationOverlay)
    {
        // Mixed Gradient:  Create two layers and set the second one to blend with "Overlay".
        // When a < 0.5, the output should be 2 * a * b
        // When a > 0.5, the output should be (1 - (2 * (1 - a) * (1 - b)))
        // (At a = 0.5, both formulas are equivalent)

        constexpr int dataSize = 3;
        AZStd::vector<float> inputLayer1 =
        {
            0.0f, 0.1f, 0.2f,
            0.5f, 0.6f, 0.7f,
            1.0f, 0.9f, 1.0f
        };
        AZStd::vector<float> inputLayer2 =
        {
            0.1f, 0.4f, 0.8f,
            0.9f, 0.2f, 0.3f,
            0.7f, 1.0f, 1.0f
        };
        AZStd::vector<float> expectedOutput = 
        {
            0.0f, 0.08f, 0.32f,      // a < 0.5, 2 * a * b
            0.9f, 0.36f, 0.58f,     // a >= 0.5, (1 - (2 * (1 - a) * (1 - b)))  
            1.0f, 1.0f, 1.0f        // if a > 0.5 and a or b = 1, the result should be 1
        };

        TestMixedGradientComponent(dataSize, inputLayer1, inputLayer2, expectedOutput, GradientSignal::MixedGradientLayer::MixingOperation::Overlay, 1.0f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, ReferenceGradientComponent_KnownValues)
    {
        // Verify that the Reference Gradient successfully "passes through" and provides back the
        // exact same values as the gradient it's referencing.

        constexpr int dataSize = 2;
        AZStd::vector<float> inputData =
        {
            0.0f, 1.0f,
            0.2f, 0.1122f
        };
        AZStd::vector<float> expectedOutput = inputData;

        auto mockReference = CreateEntity();
        const AZ::EntityId id = mockReference->GetId();
        MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

        // Create a reference gradient with an arbitrary box shape on it.
        const float HalfBounds = 64.0f;
        auto entity = BuildTestReferenceGradient(HalfBounds, mockReference->GetId());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalReferencesTestsFixture, ReferenceGradientComponent_CyclicReferences)
    {
        // Verify that gradient references can validate and disconnect cyclic connections

        // Create a constant gradient with an arbitrary box shape on it.
        const float HalfBounds = 64.0f;
        auto constantGradientEntity = BuildTestConstantGradient(HalfBounds);

        // Verify cyclic reference test passes when pointing to gradient generator entity
        auto referenceGradientEntity1 = CreateEntity();
        GradientSignal::ReferenceGradientConfig referenceGradientConfig1;
        referenceGradientConfig1.m_gradientSampler.m_ownerEntityId = referenceGradientEntity1->GetId();
        referenceGradientConfig1.m_gradientSampler.m_gradientId = constantGradientEntity->GetId();
        referenceGradientEntity1->CreateComponent<GradientSignal::ReferenceGradientComponent>(referenceGradientConfig1);
        ActivateEntity(referenceGradientEntity1.get());
        EXPECT_TRUE(referenceGradientConfig1.m_gradientSampler.ValidateGradientEntityId());

        // Verify cyclic reference test passes when nesting references to gradient generator entity
        auto referenceGradientEntity2 = CreateEntity();
        GradientSignal::ReferenceGradientConfig referenceGradientConfig2;
        referenceGradientConfig2.m_gradientSampler.m_ownerEntityId = referenceGradientEntity2->GetId();
        referenceGradientConfig2.m_gradientSampler.m_gradientId = referenceGradientEntity1->GetId();
        referenceGradientEntity2->CreateComponent<GradientSignal::ReferenceGradientComponent>(referenceGradientConfig2);
        ActivateEntity(referenceGradientEntity2.get());
        EXPECT_TRUE(referenceGradientConfig2.m_gradientSampler.ValidateGradientEntityId());

        // Verify cyclic reference test fails when referencing self
        auto referenceGradientEntity3 = CreateEntity();
        GradientSignal::ReferenceGradientConfig referenceGradientConfig3;
        referenceGradientConfig3.m_gradientSampler.m_ownerEntityId = referenceGradientEntity3->GetId();
        referenceGradientConfig3.m_gradientSampler.m_gradientId = referenceGradientEntity3->GetId();
        referenceGradientEntity3->CreateComponent<GradientSignal::ReferenceGradientComponent>(referenceGradientConfig3);
        ActivateEntity(referenceGradientEntity3.get());
        EXPECT_FALSE(referenceGradientConfig3.m_gradientSampler.ValidateGradientEntityId());
        EXPECT_EQ(referenceGradientConfig3.m_gradientSampler.m_gradientId, AZ::EntityId());

        // Verify cyclic reference test fails with nested, circular reference
        auto referenceGradientEntity4 = CreateEntity();
        auto referenceGradientEntity5 = CreateEntity();
        auto referenceGradientEntity6 = CreateEntity();

        GradientSignal::ReferenceGradientConfig referenceGradientConfig4;
        referenceGradientConfig4.m_gradientSampler.m_ownerEntityId = referenceGradientEntity4->GetId();
        referenceGradientConfig4.m_gradientSampler.m_gradientId = referenceGradientEntity5->GetId();
        referenceGradientEntity4->CreateComponent<GradientSignal::ReferenceGradientComponent>(referenceGradientConfig4);
        ActivateEntity(referenceGradientEntity4.get());

        GradientSignal::ReferenceGradientConfig referenceGradientConfig5;
        referenceGradientConfig5.m_gradientSampler.m_ownerEntityId = referenceGradientEntity5->GetId();
        referenceGradientConfig5.m_gradientSampler.m_gradientId = referenceGradientEntity6->GetId();
        referenceGradientEntity5->CreateComponent<GradientSignal::ReferenceGradientComponent>(referenceGradientConfig5);
        ActivateEntity(referenceGradientEntity5.get());

        GradientSignal::ReferenceGradientConfig referenceGradientConfig6;
        referenceGradientConfig6.m_gradientSampler.m_ownerEntityId = referenceGradientEntity6->GetId();
        referenceGradientConfig6.m_gradientSampler.m_gradientId = referenceGradientEntity4->GetId();
        referenceGradientEntity6->CreateComponent<GradientSignal::ReferenceGradientComponent>(referenceGradientConfig6);
        ActivateEntity(referenceGradientEntity6.get());

        EXPECT_FALSE(referenceGradientConfig6.m_gradientSampler.ValidateGradientEntityId());
        EXPECT_EQ(referenceGradientConfig6.m_gradientSampler.m_gradientId, AZ::EntityId());
    }

    TEST_F(GradientSignalReferencesTestsFixture, ShapeAreaFalloffGradientComponent_ZeroFalloff)
    {
        // Verify that if we have a 0-width falloff, only the points that fall directly on the shape
        // get a 1, and everything else gets a 0 

        constexpr int dataSize = 3;
        AZStd::vector<float> expectedOutput =
        {
            1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f
        };

        // Create an AABB from -1 to 1, so points at coorindates 0 and 1 fall on it, but any points at coordinate 2 won't.
        auto entityShape = CreateEntity();
        entityShape->CreateComponent<MockShapeComponent>();
        MockShapeComponentHandler mockShapeComponentHandler(entityShape->GetId());
        mockShapeComponentHandler.m_GetEncompassingAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(1.0f));

        GradientSignal::ShapeAreaFalloffGradientConfig config;
        config.m_shapeEntityId = entityShape->GetId();
        config.m_falloffWidth = 0.0f;
        config.m_falloffType = GradientSignal::FalloffType::Outer;
        
        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::ShapeAreaFalloffGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }
    
    TEST_F(GradientSignalReferencesTestsFixture, ShapeAreaFalloffGradientComponent_NonZeroFalloff)
    {
        // Verify for a range of non-zero falloffs that we get back expected 1-0 values across the falloff range.
        // We should get 1 on the shape, and "falloff" down to 0 as we get further away.  
        // For this test, we put the corner of our shape at (0, 0) so that everything past (0, 0) is falloff.


        // Create our test shape from -1 to 0, so we have a corner directly on (0, 0).
        auto entityShape = CreateEntity();
        entityShape->CreateComponent<MockShapeComponent>();
        MockShapeComponentHandler mockShapeComponentHandler(entityShape->GetId());
        mockShapeComponentHandler.m_GetEncompassingAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(0.0f));

        // Run through a range of falloffs
        for (float falloff = 1.0f; falloff <= 5.0f; falloff++)
        {
            // Choose a dataSize larger than our largest tested falloff value to additionally test that
            // we get consistent 0 values eveywhere past the falloff distance.
            constexpr int dataSize = 7;
            AZStd::vector<float> expectedOutput;

            GradientSignal::ShapeAreaFalloffGradientConfig config;
            config.m_shapeEntityId = entityShape->GetId();
            config.m_falloffWidth = falloff;
            config.m_falloffType = GradientSignal::FalloffType::Outer;

            // To determine our expected output, we get the distance from (0, 0) and inverse lerp across the falloff - 0 range
            // to convert into our expected 0 - 1 output value range.
            for (int y = 0; y < dataSize; y++)
            {
                for (int x = 0; x < dataSize; x++)
                {
                    // Get the number of meters away from the corner of the AABB sitting at (0, 0).
                    float distance = AZ::Vector3::CreateZero().GetDistance(AZ::Vector3(static_cast<float>(x), static_cast<float>(y), 0.0f));
                    // We inverse lerp from falloff - 0 so that our values go from 1 at 0 distance down to 0 at falloff distance.
                    expectedOutput.push_back(AZ::GetClamp(AZ::LerpInverse(config.m_falloffWidth, 0.0f, distance), 0.0f, 1.0f));
                }
            }

            auto entity = CreateEntity();
            entity->CreateComponent<GradientSignal::ShapeAreaFalloffGradientComponent>(config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }
    }

    TEST_F(GradientSignalReferencesTestsFixture, SurfaceAltitudeGradientComponent_PinnedShape)
    {
        // When using a Surface Altitude Gradient with a pinned shape, the altitude values that
        // come back should be based on the AABB range of the pinned shape.

        constexpr int dataSize = 2;
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.2f,
            0.5f, 1.0f,
        };

        // We're pinning a shape, so the bounding box of (0, 0, 0) - (10, 10, 10) will be the one that applies.
        auto entityShape = CreateEntity();
        entityShape->CreateComponent<MockShapeComponent>();
        MockShapeComponentHandler mockShapeComponentHandler(entityShape->GetId());
        mockShapeComponentHandler.m_GetEncompassingAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3::CreateZero(), AZ::Vector3(10.0f));

        // Set a different altitude for each point we're going to test.  We'll use 0, 2, 5, 10 to test various points along the range.
        auto surfaceEntity = CreateEntity();
        auto mockSurface = surfaceEntity->CreateComponent<MockSurfaceProviderComponent>();
        mockSurface->m_bounds = mockShapeComponentHandler.m_GetEncompassingAabb;
        AzFramework::SurfaceData::SurfacePoint mockOutputs[] =
        {
            { AZ::Vector3(0.0f, 0.0f,  0.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f,  2.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f,  5.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f, 10.0f), AZ::Vector3::CreateAxisZ() },
        };
        
        mockSurface->m_surfacePoints[AZStd::make_pair(0.0f, 0.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[0], 1);
        mockSurface->m_surfacePoints[AZStd::make_pair(1.0f, 0.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[1], 1);
        mockSurface->m_surfacePoints[AZStd::make_pair(0.0f, 1.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[2], 1);
        mockSurface->m_surfacePoints[AZStd::make_pair(1.0f, 1.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[3], 1);
        ActivateEntity(surfaceEntity.get());

        // We set the min/max to values other than 0-10 to help validate that they aren't used in the case of the pinned shape.
        GradientSignal::SurfaceAltitudeGradientConfig config;
        config.m_shapeEntityId = entityShape->GetId();
        config.m_altitudeMin = 1.0f;
        config.m_altitudeMax = 24.0f;

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::SurfaceAltitudeGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }
    
    TEST_F(GradientSignalReferencesTestsFixture, SurfaceAltitudeGradientComponent_NoShape)
    {
        // When using a Surface Altitude Gradient without a shape, the altitude values that
        // come back should be based on the min / max range of the component.

        constexpr int dataSize = 2;
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.2f,
            0.5f, 1.0f,
        };

        auto entityShape = CreateEntity();

        // Set a different altitude for each point we're going to test.  We'll use 0, 2, 5, 10 to test various points along the range.
        auto surfaceEntity = CreateEntity();
        auto mockSurface = surfaceEntity->CreateComponent<MockSurfaceProviderComponent>();
        mockSurface->m_bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(1.0f));
        AzFramework::SurfaceData::SurfacePoint mockOutputs[] = {
            { AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f, 2.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f, 5.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f, 10.0f), AZ::Vector3::CreateAxisZ() },
        };

        mockSurface->m_surfacePoints[AZStd::make_pair(0.0f, 0.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[0], 1);
        mockSurface->m_surfacePoints[AZStd::make_pair(1.0f, 0.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[1], 1);
        mockSurface->m_surfacePoints[AZStd::make_pair(0.0f, 1.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[2], 1);
        mockSurface->m_surfacePoints[AZStd::make_pair(1.0f, 1.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[3], 1);
        ActivateEntity(surfaceEntity.get());

        // We set the min/max to 0-10, but don't set a shape.
        GradientSignal::SurfaceAltitudeGradientConfig config;
        config.m_altitudeMin = 0.0f;
        config.m_altitudeMax = 10.0f;

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::SurfaceAltitudeGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalReferencesTestsFixture, SurfaceAltitudeGradientComponent_MissingSurfaceIsZero)
    {
        // Querying altitude where the surface doesn't exist results in a value of 0.

        constexpr int dataSize = 2;
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f,
            0.0f, 0.0f,
        };

        auto entityShape = CreateEntity();

        // We set the min/max to -5 - 15 so that a height of 0 would produce a non-zero value.
        GradientSignal::SurfaceAltitudeGradientConfig config;
        config.m_altitudeMin = -5.0f;
        config.m_altitudeMax = 15.0f;

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::SurfaceAltitudeGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalReferencesTestsFixture, SurfaceAltitudeGradientComponent_ClampToMinMax)
    {
        // Verify that surface altitudes outside of the min / max range get clamped to 0.0 and 1.0.

        constexpr int dataSize = 2;
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f,
            1.0f, 1.0f,
        };

        auto entityShape = CreateEntity();

        auto surfaceEntity = CreateEntity();
        auto mockSurface = surfaceEntity->CreateComponent<MockSurfaceProviderComponent>();
        mockSurface->m_bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(1.0f));
        AzFramework::SurfaceData::SurfacePoint mockOutputs[] = {
            { AZ::Vector3(0.0f, 0.0f, -10.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f,  -5.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f,  15.0f), AZ::Vector3::CreateAxisZ() },
            { AZ::Vector3(0.0f, 0.0f,  20.0f), AZ::Vector3::CreateAxisZ() },
        };

        // Altitude value below min - should result in 0.0f.
        mockSurface->m_surfacePoints[AZStd::make_pair(0.0f, 0.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[0], 1);
        // Altitude value at exactly min - should result in 0.0f.
        mockSurface->m_surfacePoints[AZStd::make_pair(1.0f, 0.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[1], 1);
        // Altitude value at exactly max - should result in 1.0f.
        mockSurface->m_surfacePoints[AZStd::make_pair(0.0f, 1.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[2], 1);
        // Altitude value above max - should result in 1.0f.
        mockSurface->m_surfacePoints[AZStd::make_pair(1.0f, 1.0f)] =
            AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&mockOutputs[3], 1);
        ActivateEntity(surfaceEntity.get());

        // We set the min/max to -5 - 15.  By using a range without 0 at either end, and not having 0 as the midpoint, 
        // it should be easier to verify that we're successfully clamping to 0 and 1.
        GradientSignal::SurfaceAltitudeGradientConfig config;
        config.m_altitudeMin = -5.0f;
        config.m_altitudeMax = 15.0f;

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::SurfaceAltitudeGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalReferencesTestsFixture, SurfaceMaskGradientComponent_SingleMaskExpectedValues)
    {
        // When querying a surface that contains the expected mask, verify we get back exactly the
        // values we expect for each point.

        constexpr int dataSize = 2;
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.2f,
            0.5f, 1.0f,
        };

        auto surfaceEntity = CreateEntity();
        auto mockSurface = surfaceEntity->CreateComponent<MockSurfaceProviderComponent>();
        mockSurface->m_bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(aznumeric_cast<float>(dataSize)));
        mockSurface->m_tags.emplace_back("test_mask");

        AzFramework::SurfaceData::SurfacePoint point;

        // Fill our mock surface with the test_mask set and the expected gradient value at each point.
        for (int y = 0; y < dataSize; y++)
        {
            for (int x = 0; x < dataSize; x++)
            {
                point.m_position = AZ::Vector3(aznumeric_cast<float>(x), aznumeric_cast<float>(y), 0.0f);
                point.m_normal = AZ::Vector3::CreateAxisZ();
                point.m_surfaceTags.clear();
                point.m_surfaceTags.emplace_back(AZ_CRC_CE("test_mask"), expectedOutput[(y * dataSize) + x]);
                mockSurface->m_surfacePoints[AZStd::make_pair(static_cast<float>(x), static_cast<float>(y))] =
                    AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&point, 1);
            }
        }
        ActivateEntity(surfaceEntity.get());

        GradientSignal::SurfaceMaskGradientConfig config;
        config.m_surfaceTagList.push_back(AZ_CRC_CE("test_mask"));

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::SurfaceMaskGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }
  
    TEST_F(GradientSignalReferencesTestsFixture, SurfaceMaskGradientComponent_NoValues)
    {
        // When querying a surface that contains no points (either lack of surface, or filtered-out surface tag), verify we get back 0.0f.
        // NOTE: Because we're mocking the SurfaceDataSystem, which is the system that contains the mask filtering logic,
        // we don't have separate tests for wrong mask vs no points.  From the gradient's perspective, these should both
        // get no points returned from the system.

        constexpr int dataSize = 2;

        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f,
            0.0f, 0.0f,
        };

        GradientSignal::SurfaceMaskGradientConfig config;
        config.m_surfaceTagList.push_back(AZ_CRC_CE("test_mask"));

        auto entity = CreateEntity();
        entity->CreateComponent<GradientSignal::SurfaceMaskGradientComponent>(config);
        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalReferencesTestsFixture, SurfaceSlopeGradientComponent_KnownValues)
    {
        // When using a Surface Slope Gradient, verify that we get back expected slope values
        // for given sets of normals and min / max ranges.

        constexpr int dataSize = 3;
        AZStd::vector<float> expectedOutput =
        {
            1.0f, 0.9f, 0.8f,
            0.6f, 0.5f, 0.4f,
            0.2f, 0.1f, 0.0f
        };

        AZStd::vector<AZStd::pair<float, float>> minMaxTests =
        {
            AZStd::make_pair(0.0f, 90.0f),      // Test the regular full min/max range (note that values above 90 degrees aren't supported)
            AZStd::make_pair(90.0f, 0.0f),      // Test an inverted min/max range
            AZStd::make_pair(10.0f, 70.0f)      // Test an asymmetric range within the full 0 - 90 degree range.
        };

        AZStd::vector<GradientSignal::SurfaceSlopeGradientConfig::RampType> rampTests =
        {
            GradientSignal::SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_DOWN,
            GradientSignal::SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_UP,
        };

        for (auto rampTest : rampTests)
        {
            for (auto minMax : minMaxTests)
            {
                AZStd::vector<float> inputAngles;
                const float slopeMin = minMax.first;
                const float slopeMax = minMax.second;

                // Fill our mock surface with normals that match the correct test angle for each point.
                for (int y = 0; y < dataSize; y++)
                {
                    for (int x = 0; x < dataSize; x++)
                    {
                        float angle = 0.0f;

                        // For linear ramps, the input angle should be whatever our desired output is,
                        // lerped either between slopeMin-slopeMax, or slopeMax-slopeMin, depending on the
                        // direction of the ramp.
                        switch (rampTest)
                        {
                            case GradientSignal::SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_DOWN:
                                angle = AZ::Lerp(slopeMax, slopeMin, expectedOutput[(y * dataSize) + x]);
                                break;
                            case GradientSignal::SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_UP:
                                angle = AZ::Lerp(slopeMin, slopeMax, expectedOutput[(y * dataSize) + x]);
                                break;
                        }
                        inputAngles.push_back(angle);
                    }
                }

                TestSurfaceSlopeGradientComponent(dataSize, inputAngles, expectedOutput,
                                                  slopeMin, slopeMax, rampTest, 0.0f, 0.0f, 0.0f);
            }
        }
     }

    TEST_F(GradientSignalReferencesTestsFixture, SurfaceSlopeGradientComponent_ClampToMinMax)
    {
        // Verify that surface slope outside of the min / max range get clamped to 1.0 and 0.0.
        // NOTE: We expect the Surface Slope Gradient to produce a signal value of 1.0 at or below the min,
        // and 0.0 at or above the max.

        constexpr int dataSize = 2;
        AZStd::vector<float> inputAngles =
        {
            5.0f, 20.0f,        // test that values below or at the min clamp to 1.0
            50.0f, 70.0f,       // test that values at or above the max clamp to 0.0
        };

        AZStd::vector<float> expectedOutput =
        {
            1.0f, 1.0f,
            0.0f, 0.0f,
        };

        // We set the min/max to 20 - 50 as a mostly arbitrary choice that represents a range that's not 
        // centered around the midpoint of a full 0 - 90 degree range.
        TestSurfaceSlopeGradientComponent(dataSize, inputAngles, expectedOutput, 20.0f, 50.0f,
                                          GradientSignal::SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_DOWN, 0.0f, 0.0f, 0.0f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, SurfaceSlopeGradientComponent_SmoothStep)
    {
        // Verify that surface slope produces expected results when used with a smooth step.

        // Smooth step creates a ramp up and down.  We expect the following (within our min/max angle range):
        // inputs 0 to (midpoint - range/2):  0
        // inputs (midpoint - range/2) to (midpoint - range/2)+softness:  ramp up
        // inputs (midpoint - range/2)+softness to (midpoint + range/2)-softness:  1
        // inputs (midpoint + range/2)-softness) to (midpoint + range/2):  ramp down
        // inputs (midpoint + range/2) to 1:  0

        // We'll test with midpoint = 0.5, range = 0.6, softness = 0.1 so that we have easy ranges to verify.

        constexpr int dataSize = 5;
        AZStd::vector<float> inputData =
        {
            0.00f, 0.05f, 0.10f, 0.15f, 0.20f,      // Should all be 0
            0.21f, 0.23f, 0.25f, 0.27f, 0.29f,      // Should ramp up
            0.30f, 0.40f, 0.50f, 0.60f, 0.70f,      // Should all be 1
            0.71f, 0.73f, 0.75f, 0.77f, 0.79f,      // Should ramp down
            0.80f, 0.85f, 0.90f, 0.95f, 1.00f       // Should all be 0
        };

        // For smoothstep ramp curves, we expect the values to be symmetric between the up and down ramp,
        // hit 0.5 at the middle of the ramp, and be symmetric on both sides of the midpoint of the ramp.
        AZStd::vector<float> expectedOutput =
        {
            0.000f, 0.000f, 0.000f, 0.000f, 0.000f,           // 0.00 - 0.20 input -> 0.0 output
            0.028f, 0.216f, 0.500f, 0.784f, 0.972f,           // 0.21 - 0.29 input -> pre-verified ramp up values
            1.000f, 1.000f, 1.000f, 1.000f, 1.000f,           // 0.30 - 0.70 input -> 1.0 output
            0.972f, 0.784f, 0.500f, 0.216f, 0.028f,           // 0.71 - 0.79 input -> pre-verified ramp down values
            0.000f, 0.000f, 0.000f, 0.000f, 0.000f,           // 0.80 - 1.00 input -> 0.0 output
        };

        AZStd::vector<float> inputAngles;

        // We set the min/max to 20 - 50 as a mostly arbitrary choice that represents a range that's not 
        // centered around the midpoint of a full 0 - 90 degree range.
        const float slopeMin = 20.0f;
        const float slopeMax = 50.0f;

        // Fill our mock surface with the correct normal value for each point based on our test angle set.
        for (int y = 0; y < dataSize; y++)
        {
            for (int x = 0; x < dataSize; x++)
            {
                // Map our input values of 0-1 into our slope Min-Max range to create our desired input angles.
                inputAngles.push_back(AZ::Lerp(slopeMin, slopeMax, inputData[(y * dataSize) + x]));
            }
        }

        TestSurfaceSlopeGradientComponent(dataSize, inputAngles, expectedOutput, slopeMin, slopeMax, 
                                          GradientSignal::SurfaceSlopeGradientConfig::RampType::SMOOTH_STEP, 0.5f, 0.6f, 0.1f);
    }

    TEST_F(GradientSignalReferencesTestsFixture, SurfaceSlopeGradientComponent_SmoothStep_ClampToZero)
    {
        // Verify that surface slope outside of the min / max range get clamped to 0.0 when using smooth step.

        constexpr int dataSize = 2;
        AZStd::vector<float> inputAngles =
        {
            5.0f, 20.0f,        // test that values below or at the min clamp to 0.0
            50.0f, 70.0f,       // test that values at or above the max clamp to 0.0
        };

        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f,
            0.0f, 0.0f,
        };

        TestSurfaceSlopeGradientComponent(dataSize, inputAngles, expectedOutput, 20.0f, 50.0f,
                                          GradientSignal::SurfaceSlopeGradientConfig::RampType::SMOOTH_STEP, 0.5f, 0.6f, 0.1f);
    }

}

