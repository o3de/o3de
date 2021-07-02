/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"

#include "Tests/GradientSignalTestMocks.h"

#include <GradientSignal/PerlinImprovedNoise.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <Source/Components/PerlinGradientComponent.h>
#include <Source/Components/RandomGradientComponent.h>
#include <Source/Components/LevelsGradientComponent.h>
#include <Source/Components/PosterizeGradientComponent.h>
#include <Source/Components/SmoothStepGradientComponent.h>
#include <Source/Components/ThresholdGradientComponent.h>
#include <Source/Components/GradientTransformComponent.h>

namespace UnitTest
{
    struct GradientSignalTestGeneratorFixture
        : public GradientSignalTest
    {
        void TestLevelsGradientComponent(int dataSize, const AZStd::vector<float>& inputData, const AZStd::vector<float>& expectedOutput,
                                         float inputMin, float inputMid, float inputMax, float outputMin, float outputMax)
        {
            auto entityMock = CreateEntity();
            const AZ::EntityId id = entityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

            GradientSignal::GradientTransformConfig gradientTransformConfig;
            CreateComponent<GradientSignal::GradientTransformComponent>(entityMock.get(), gradientTransformConfig);
            CreateComponent<MockShapeComponent>(entityMock.get());
            MockShapeComponentHandler mockShapeHandler(entityMock->GetId());

            ActivateEntity(entityMock.get());

            GradientSignal::LevelsGradientConfig config;
            config.m_gradientSampler.m_gradientId = entityMock->GetId();
            config.m_inputMin = inputMin;
            config.m_inputMid = inputMid;
            config.m_inputMax = inputMax;
            config.m_outputMin = outputMin;
            config.m_outputMax = outputMax;

            auto entity = CreateEntity();
            CreateComponent<GradientSignal::LevelsGradientComponent>(entity.get(), config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }

        void TestPosterizeGradientComponent(int dataSize, const AZStd::vector<float>& inputData, const AZStd::vector<float>& expectedOutput,
                                            GradientSignal::PosterizeGradientConfig::ModeType posterizeMode, int bands)
        {
            auto entityMock = CreateEntity();
            const AZ::EntityId id = entityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

            GradientSignal::GradientTransformConfig gradientTransformConfig;
            CreateComponent<GradientSignal::GradientTransformComponent>(entityMock.get(), gradientTransformConfig);
            CreateComponent<MockShapeComponent>(entityMock.get());
            MockShapeComponentHandler mockShapeHandler(entityMock->GetId());

            ActivateEntity(entityMock.get());

            GradientSignal::PosterizeGradientConfig config;
            config.m_gradientSampler.m_gradientId = entityMock->GetId();
            config.m_mode = posterizeMode;
            config.m_bands = bands;

            auto entity = CreateEntity();
            CreateComponent<GradientSignal::PosterizeGradientComponent>(entity.get(), config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }

        void TestSmoothStepGradientComponent(int dataSize, const AZStd::vector<float>& inputData, const AZStd::vector<float>& expectedOutput, 
                                            float midpoint, float range, float softness)
        {
            auto entityMock = CreateEntity();
            const AZ::EntityId id = entityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

            GradientSignal::GradientTransformConfig gradientTransformConfig;
            CreateComponent<GradientSignal::GradientTransformComponent>(entityMock.get(), gradientTransformConfig);
            CreateComponent<MockShapeComponent>(entityMock.get());
            MockShapeComponentHandler mockShapeHandler(entityMock->GetId());

            ActivateEntity(entityMock.get());

            GradientSignal::SmoothStepGradientConfig config;
            config.m_gradientSampler.m_gradientId = entityMock->GetId();
            config.m_smoothStep.m_falloffMidpoint = midpoint;
            config.m_smoothStep.m_falloffRange = range;
            config.m_smoothStep.m_falloffStrength = softness;

            auto entity = CreateEntity();
            CreateComponent<GradientSignal::SmoothStepGradientComponent>(entity.get(), config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }

        void TestThresholdGradientComponent(int dataSize, const AZStd::vector<float>& inputData, const AZStd::vector<float>& expectedOutput, float threshold)
        {
            auto entityMock = CreateEntity();
            const AZ::EntityId id = entityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

            GradientSignal::GradientTransformConfig gradientTransformConfig;
            CreateComponent<GradientSignal::GradientTransformComponent>(entityMock.get(), gradientTransformConfig);
            CreateComponent<MockShapeComponent>(entityMock.get());
            MockShapeComponentHandler mockShapeHandler(entityMock->GetId());

            ActivateEntity(entityMock.get());

            GradientSignal::ThresholdGradientConfig config;
            config.m_gradientSampler.m_gradientId = entityMock->GetId();
            config.m_threshold = threshold;

            auto entity = CreateEntity();
            CreateComponent<GradientSignal::ThresholdGradientComponent>(entity.get(), config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }
    };

    TEST_F(GradientSignalTestGeneratorFixture, GradientSampler_BasicFunctionality)
    {
        // Verify that a GradientSampler correctly handles requests and returns the mocked value.

        const float expectedOutput = 159.0f;
        auto entity = CreateEntity();
        const AZ::EntityId id = entity->GetId();
        MockGradientRequestsBus mockGradientRequestsBus(id);
        mockGradientRequestsBus.m_GetValue = expectedOutput;
        ActivateEntity(entity.get());

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = entity->GetId();
        EXPECT_EQ(expectedOutput, gradientSampler.GetValue({}));
    }

#if AZ_TRAIT_DISABLE_FAILED_GRADIENT_SIGNAL_TESTS
    TEST_F(GradientSignalTestGeneratorFixture, DISABLED_PerlinGradientComponent_GoldenTest)
#else
    TEST_F(GradientSignalTestGeneratorFixture, PerlinGradientComponent_GoldenTest)
#endif // AZ_TRAIT_DISABLE_FAILED_GRADIENT_SIGNAL_TESTS
    {
        // Make sure PerlinGradientComponent generates a set of values that
        // matches a previously-calculated "golden" set of values.

        constexpr int dataSize = 4;
        AZStd::vector<float> expectedOutput =
        {
            0.5000f, 0.5456f, 0.5138f, 0.4801f,
            0.4174f, 0.4942f, 0.5493f, 0.5431f,
            0.4984f, 0.5204f, 0.5526f, 0.5840f,
            0.5251f, 0.5029f, 0.6153f, 0.5802f,
        };

        GradientSignal::PerlinGradientConfig config;
        config.m_randomSeed = 7878;
        config.m_octave = 4;
        config.m_amplitude = 3.0f;
        config.m_frequency = 1.13f;

        auto entity = CreateEntity();
        CreateComponent<GradientSignal::PerlinGradientComponent>(entity.get(), config);

        GradientSignal::GradientTransformConfig gradientTransformConfig;
        CreateComponent<GradientSignal::GradientTransformComponent>(entity.get(), gradientTransformConfig);
        CreateComponent<MockShapeComponent>(entity.get());
        MockShapeComponentHandler mockShapeHandler(entity->GetId());

        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalTestGeneratorFixture, RandomGradientComponent_GoldenTest)
    {
        // Make sure RandomGradientComponent returns back a "golden" set
        // of values for a given random seed.

        constexpr int dataSize = 4;
        AZStd::vector<float> expectedOutput =
        {
            0.5059f, 0.4902f, 0.6000f, 0.7372f,
            0.9490f, 0.2823f, 0.6588f, 0.5804f,
            0.1490f, 0.3294f, 0.1451f, 0.6627f,
            0.2980f, 0.1608f, 0.9098f, 0.9804f,
        };

        GradientSignal::RandomGradientConfig config;
        config.m_randomSeed = 5656;

        auto entity = CreateEntity();
        CreateComponent<GradientSignal::RandomGradientComponent>(entity.get(), config);

        GradientSignal::GradientTransformConfig gradientTransformConfig;
        CreateComponent<GradientSignal::GradientTransformComponent>(entity.get(), gradientTransformConfig);
        CreateComponent<MockShapeComponent>(entity.get());
        MockShapeComponentHandler mockShapeHandler(entity->GetId());

        ActivateEntity(entity.get());

        TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
    }

    TEST_F(GradientSignalTestGeneratorFixture, LevelsGradientComponent_DefaultValues)
    {
        // Verify that with the default config values, our outputs equal our inputs.

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // default values:  input min/mid/max of 0-1-1, and output min/max of 0-1
        TestLevelsGradientComponent(dataSize, inputData, expectedOutput, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f);
    }

    TEST_F(GradientSignalTestGeneratorFixture, LevelsGradientComponent_ScaleToMinMax)
    {
        // Verify that setting the output min/max correctly scales the inputs into the output range.

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };
        AZStd::vector<float> expectedOutput;

        constexpr float outputMin = 0.25f;
        constexpr float outputMax = 0.75f;

        // We expect our inputs to be linearly scaled into the range defined by outputMin / outputMax.
        for (auto input : inputData)
        {
            expectedOutput.push_back(AZ::Lerp(outputMin, outputMax, input));
        }

        // Set input min/mid/max to 0-1-1 for no input remapping, so we only test the output params.
        TestLevelsGradientComponent(dataSize, inputData, expectedOutput, 0.0f, 1.0f, 1.0f, outputMin, outputMax);
    }

    TEST_F(GradientSignalTestGeneratorFixture, LevelsGradientComponent_BelowMinIsZero)
    {
        // Inputs at or below the min produces an output of 0.

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // Because we're adjusting our input range to 0.5 - 1, it means that values above 0.5 get lerped
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.2f,
            0.6f, 0.8f, 1.0f
        };

        // Set output min/max to 0-1 for no remapping, so we only test the input params.
        TestLevelsGradientComponent(dataSize, inputData, expectedOutput, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f);
    }

    TEST_F(GradientSignalTestGeneratorFixture, LevelsGradientComponent_AboveMaxIsOne)
    {
        // Inputs above the max produces an output of 1.

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // Because we're adjusting our input range to 0.0 - 0.5, it means that values below 0.5 get lerped
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.2f, 0.4f,
            0.8f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f
        };

        // Set output min/max to 0-1 for no remapping, so we only test the input params.
        TestLevelsGradientComponent(dataSize, inputData, expectedOutput, 0.0f, 1.0f, 0.5f, 0.0f, 1.0f);
    }

    TEST_F(GradientSignalTestGeneratorFixture, LevelsGradientComponent_AdjustedMidpoint)
    {
        // Verify that a midpoint adjusted to 0.5 correctly squares the inputs for the outputs.
        // (We're using 0.5 for verification because it's an easy value to test)

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        AZStd::vector<float> expectedOutput;

        // With a midpoint of 0.5, we expect our outputs to be the inputs squared (input ^ (1/0.5))
        for (auto input : inputData)
        {
            expectedOutput.push_back(input * input);
        }

        // Set the input midpoint to 0.5 to adjust all the values
        TestLevelsGradientComponent(dataSize, inputData, expectedOutput, 0.0f, 0.5f, 1.0f, 0.0f, 1.0f);
    }

    TEST_F(GradientSignalTestGeneratorFixture, PosterizeGradientComponent_ModeFloor)
    {
        // Verify that the "floor mode" divides into equal bands and uses the floored value for each band.
        // Ex:  For 3 bands, input bands of 0.0-0.33 / 0.33-.67 / 0.67-1.0 should map to 0.00 / 0.33 / 0.67

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // We have 3 bands, choose the lowest value from each band.
        constexpr float lowBand  = 0.0f / 3.0f;
        constexpr float midBand  = 1.0f / 3.0f;
        constexpr float highBand = 2.0f / 3.0f;

        AZStd::vector<float> expectedOutput =
        {
             lowBand,  lowBand,  lowBand,
             midBand,  midBand,  midBand,
             highBand, highBand, highBand
        };


        TestPosterizeGradientComponent(dataSize, inputData, expectedOutput, GradientSignal::PosterizeGradientConfig::ModeType::Floor, 3);
    }

    TEST_F(GradientSignalTestGeneratorFixture, PosterizeGradientComponent_ModeRound)
    {
        // Verify that the "round mode" divides into equal bands and uses the midpoint value for each band.
        // Ex:  For 3 bands, input bands of 0.0-0.33 / 0.33-.67 / 0.67-1.0 should map to 0.17 / 0.5 / 0.84

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // We have 3 bands, choose the middle value from each band.
        constexpr float lowBand = 0.5f / 3.0f;
        constexpr float midBand = 1.5f / 3.0f;
        constexpr float highBand = 2.5f / 3.0f;

        AZStd::vector<float> expectedOutput =
        {
             lowBand,  lowBand,  lowBand,
             midBand,  midBand,  midBand,
             highBand, highBand, highBand
        };

        TestPosterizeGradientComponent(dataSize, inputData, expectedOutput, GradientSignal::PosterizeGradientConfig::ModeType::Round, 3);
    }

    TEST_F(GradientSignalTestGeneratorFixture, PosterizeGradientComponent_ModeCeiling)
    {
        // Verify that the "ceiling mode" divides into equal bands and uses the high value for each band.
        // Ex:  For 3 bands, input bands of 0.0-0.33 / 0.33-.67 / 0.67-1.0 should map to 0.33 / 0.67 / 1.0

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // We have 3 bands, choose the highest value from each band.
        constexpr float lowBand  = 1.0f / 3.0f;
        constexpr float midBand  = 2.0f / 3.0f;
        constexpr float highBand = 3.0f / 3.0f;

        AZStd::vector<float> expectedOutput =
        {
             lowBand,  lowBand,  lowBand,
             midBand,  midBand,  midBand,
             highBand, highBand, highBand
        };

        TestPosterizeGradientComponent(dataSize, inputData, expectedOutput, GradientSignal::PosterizeGradientConfig::ModeType::Ceiling, 3);
    }

    TEST_F(GradientSignalTestGeneratorFixture, PosterizeGradientComponent_ModePs)
    {
        // Verify that the "Ps mode" divides into equal bands which always have 0 for the lowest band, 1 for
        // the highest band, and equally spaced ranges for every band in-between.
        // Ex:  For 3 bands, input bands of 0.0-0.33 / 0.33-.67 / 0.67-1.0 should map to 0.0 / 0.5 / 1.0

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // Ps mode has equally-spaced value ranges that always start with 0 and end with 1.
        constexpr float lowBand  = 0.0f;
        constexpr float midBand  = 0.5f;
        constexpr float highBand = 1.0f;

        AZStd::vector<float> expectedOutput =
        {
             lowBand,  lowBand,  lowBand,
             midBand,  midBand,  midBand,
             highBand, highBand, highBand
        };

        TestPosterizeGradientComponent(dataSize, inputData, expectedOutput, GradientSignal::PosterizeGradientConfig::ModeType::Ps, 3);
    }

    TEST_F(GradientSignalTestGeneratorFixture, SmoothStepGradientComponent)
    {
        // Smooth step creates a ramp up and down.  We expect the following:
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

        TestSmoothStepGradientComponent(dataSize, inputData, expectedOutput, 0.5f, 0.6f, 0.1f);
    }

    TEST_F(GradientSignalTestGeneratorFixture, ThresholdGradientComponent_ZeroThreshold)
    {
        // A threshold of 0 should make (input <= 0) go to 0, and (input > 0) go to 1.

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        AZStd::vector<float> expectedOutput =
        {
            0.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f
        };

        TestThresholdGradientComponent(dataSize, inputData, expectedOutput, 0.0f);
    }

    TEST_F(GradientSignalTestGeneratorFixture, ThresholdGradientComponent_MidpointThreshold)
    {
        // A threshold of 0.5 should make (input <= 0.5) go to 0, and (input > 0.5) go to 1.

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f
        };

        TestThresholdGradientComponent(dataSize, inputData, expectedOutput, 0.5f);
    }

    TEST_F(GradientSignalTestGeneratorFixture, ThresholdGradientComponent_OneThreshold)
    {
        // A threshold of 1.0 should make every value (input <= 1.0) drop to 0.0.

        constexpr int dataSize = 3;
        AZStd::vector<float> inputData =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f
        };

        TestThresholdGradientComponent(dataSize, inputData, expectedOutput, 1.0f);
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
