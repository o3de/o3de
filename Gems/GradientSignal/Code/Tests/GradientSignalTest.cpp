/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/GradientSignalTestFixtures.h>

#include <GradientSignal/PerlinImprovedNoise.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Components/PerlinGradientComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>
#include <GradientSignal/Components/LevelsGradientComponent.h>
#include <GradientSignal/Components/PosterizeGradientComponent.h>
#include <GradientSignal/Components/SmoothStepGradientComponent.h>
#include <GradientSignal/Components/ThresholdGradientComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>

namespace UnitTest
{
    struct GradientSignalTestGeneratorFixture
        : public GradientSignalTest
    {
        void TestLevelsGradientComponent(int dataSize, const AZStd::vector<float>& inputData, const AZStd::vector<float>& expectedOutput,
                                         float inputMin, float inputMid, float inputMax, float outputMin, float outputMax)
        {
            auto entityMock = CreateTestEntity(1.0f);
            const AZ::EntityId id = entityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

            GradientSignal::GradientTransformConfig gradientTransformConfig;
            entityMock->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

            ActivateEntity(entityMock.get());

            GradientSignal::LevelsGradientConfig config;
            config.m_gradientSampler.m_gradientId = entityMock->GetId();
            config.m_inputMin = inputMin;
            config.m_inputMid = inputMid;
            config.m_inputMax = inputMax;
            config.m_outputMin = outputMin;
            config.m_outputMax = outputMax;

            auto entity = CreateEntity();
            entity->CreateComponent<GradientSignal::LevelsGradientComponent>(config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }

        void TestPosterizeGradientComponent(int dataSize, const AZStd::vector<float>& inputData, const AZStd::vector<float>& expectedOutput,
                                            GradientSignal::PosterizeGradientConfig::ModeType posterizeMode, int bands)
        {
            auto entityMock = CreateTestEntity(0.5f);
            const AZ::EntityId id = entityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

            GradientSignal::GradientTransformConfig gradientTransformConfig;
            entityMock->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

            ActivateEntity(entityMock.get());

            GradientSignal::PosterizeGradientConfig config;
            config.m_gradientSampler.m_gradientId = entityMock->GetId();
            config.m_mode = posterizeMode;
            config.m_bands = bands;

            auto entity = CreateEntity();
            entity->CreateComponent<GradientSignal::PosterizeGradientComponent>(config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }

        void TestSmoothStepGradientComponent(int dataSize, const AZStd::vector<float>& inputData, const AZStd::vector<float>& expectedOutput, 
                                            float midpoint, float range, float softness)
        {
            auto entityMock = CreateTestEntity(0.5f);
            const AZ::EntityId id = entityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

            GradientSignal::GradientTransformConfig gradientTransformConfig;
            entityMock->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

            ActivateEntity(entityMock.get());

            GradientSignal::SmoothStepGradientConfig config;
            config.m_gradientSampler.m_gradientId = entityMock->GetId();
            config.m_smoothStep.m_falloffMidpoint = midpoint;
            config.m_smoothStep.m_falloffRange = range;
            config.m_smoothStep.m_falloffStrength = softness;

            auto entity = CreateEntity();
            entity->CreateComponent<GradientSignal::SmoothStepGradientComponent>(config);
            ActivateEntity(entity.get());

            TestFixedDataSampler(expectedOutput, dataSize, entity->GetId());
        }

        void TestThresholdGradientComponent(int dataSize, const AZStd::vector<float>& inputData, const AZStd::vector<float>& expectedOutput, float threshold)
        {
            auto entityMock = CreateTestEntity(0.5f);
            const AZ::EntityId id = entityMock->GetId();
            UnitTest::MockGradientArrayRequestsBus mockGradientRequestsBus(id, inputData, dataSize);

            GradientSignal::GradientTransformConfig gradientTransformConfig;
            entityMock->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

            ActivateEntity(entityMock.get());

            GradientSignal::ThresholdGradientConfig config;
            config.m_gradientSampler.m_gradientId = entityMock->GetId();
            config.m_threshold = threshold;

            auto entity = CreateEntity();
            entity->CreateComponent<GradientSignal::ThresholdGradientComponent>(config);
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

    TEST_F(GradientSignalTestGeneratorFixture, PerlinGradientComponent_GoldenTest)
    {
        // Make sure PerlinGradientComponent generates a set of values that
        // matches a previously-calculated "golden" set of values.

        constexpr int dataSize = 4;

        GradientSignal::PerlinGradientConfig config;
        config.m_randomSeed = 7878;
        config.m_octave = 4;
        config.m_amplitude = 3.0f;
        config.m_frequency = 1.13f;

        // The random seed to generate the input for the permutation table is platform independent, but
        // is not deterministic per platform due to the inconsistent implementaion of the mersenne twister
        // engine in different standard libraries. This will lead to deterministic results by platform,
        // so the values cannot be relied upon per platform. In order to generate consistent values, we
        // will pregenerate the permutation table using the above 7878 seed and the results of the
        // permutation generation based on the windows implementation of the RNG, so we can have a fixed
        // value to compare against the results to validate the perlin component. The values below represent
        // the original permutation table that was based on the seed and windows environment.
        AZStd::array<int, 512> testPerlinPermutationTable =
        {
            0x5e, 0xdd, 0x95, 0xf6, 0x43, 0x0f, 0x7e, 0x20, 0xf7, 0xb7, 0x82, 0x98, 0x73, 0x58, 0xf5, 0xa0,
            0xa7, 0x12, 0xbf, 0x9c, 0xba, 0x88, 0x08, 0x2d, 0xd6, 0x1f, 0xd0, 0x4f, 0x0e, 0x9e, 0x4a, 0xe4,
            0x93, 0xac, 0x5a, 0x89, 0x13, 0x8b, 0x62, 0x3c, 0x69, 0x78, 0xda, 0xcd, 0x57, 0xa6, 0x0d, 0xde,
            0xb5, 0xb2, 0x70, 0x04, 0x16, 0x2a, 0x91, 0x2c, 0x07, 0x6a, 0x81, 0x4c, 0x9d, 0xad, 0xe1, 0x2b,
            0x30, 0x3b, 0x83, 0x9b, 0x31, 0x38, 0x9f, 0xaf, 0x3e, 0x1c, 0x06, 0x97, 0x46, 0x00, 0xae, 0x90,
            0xc3, 0xd9, 0xf2, 0xd2, 0xcf, 0x11, 0x10, 0xe7, 0x56, 0xfa, 0x87, 0x09, 0x1b, 0xb4, 0x61, 0x25,
            0xcc, 0x7c, 0x50, 0x94, 0xc6, 0x0c, 0xe3, 0xc1, 0x26, 0x96, 0xdc, 0x02, 0xa8, 0x19, 0xe9, 0x68,
            0xf4, 0xb3, 0x4b, 0x33, 0x52, 0xb1, 0x6f, 0xec, 0x51, 0x1e, 0x24, 0xc7, 0xaa, 0xc8, 0xc9, 0x15,
            0x18, 0x48, 0x0a, 0xa3, 0xdf, 0x59, 0xf8, 0x92, 0x64, 0xd5, 0xfb, 0x8f, 0x99, 0xca, 0xea, 0x79,
            0x63, 0x84, 0x6b, 0x67, 0x2e, 0x28, 0xab, 0xcb, 0xf1, 0x2f, 0x71, 0x5c, 0x27, 0x72, 0xdb, 0x03,
            0xd1, 0x36, 0x65, 0x14, 0x7a, 0x23, 0xf3, 0x5f, 0xb0, 0x86, 0xe6, 0x8c, 0xa4, 0x6d, 0xf9, 0x22,
            0xce, 0x40, 0x01, 0x8e, 0xbd, 0x17, 0x7b, 0x66, 0xa1, 0x5b, 0xa9, 0xa2, 0xe5, 0x1a, 0xee, 0x3f,
            0x85, 0xeb, 0xef, 0xff, 0x4d, 0xfc, 0xb9, 0xd3, 0x5d, 0x53, 0xd4, 0x76, 0x49, 0xbc, 0x41, 0xc0,
            0x39, 0x21, 0x74, 0xed, 0x54, 0xd7, 0xc5, 0x8a, 0xd8, 0xc4, 0xfe, 0x29, 0x9a, 0x6e, 0x7d, 0xb8,
            0xc2, 0x55, 0x1d, 0xfd, 0x05, 0x42, 0x4e, 0x3d, 0xe8, 0x60, 0xe2, 0x75, 0x6c, 0x7f, 0x45, 0xbe,
            0x47, 0x44, 0xbb, 0xe0, 0x3a, 0xb6, 0xa5, 0x77, 0x34, 0x0b, 0x37, 0x32, 0x8d, 0x35, 0xf0, 0x80,
            0x5e, 0xdd, 0x95, 0xf6, 0x43, 0x0f, 0x7e, 0x20, 0xf7, 0xb7, 0x82, 0x98, 0x73, 0x58, 0xf5, 0xa0,
            0xa7, 0x12, 0xbf, 0x9c, 0xba, 0x88, 0x08, 0x2d, 0xd6, 0x1f, 0xd0, 0x4f, 0x0e, 0x9e, 0x4a, 0xe4,
            0x93, 0xac, 0x5a, 0x89, 0x13, 0x8b, 0x62, 0x3c, 0x69, 0x78, 0xda, 0xcd, 0x57, 0xa6, 0x0d, 0xde,
            0xb5, 0xb2, 0x70, 0x04, 0x16, 0x2a, 0x91, 0x2c, 0x07, 0x6a, 0x81, 0x4c, 0x9d, 0xad, 0xe1, 0x2b,
            0x30, 0x3b, 0x83, 0x9b, 0x31, 0x38, 0x9f, 0xaf, 0x3e, 0x1c, 0x06, 0x97, 0x46, 0x00, 0xae, 0x90,
            0xc3, 0xd9, 0xf2, 0xd2, 0xcf, 0x11, 0x10, 0xe7, 0x56, 0xfa, 0x87, 0x09, 0x1b, 0xb4, 0x61, 0x25,
            0xcc, 0x7c, 0x50, 0x94, 0xc6, 0x0c, 0xe3, 0xc1, 0x26, 0x96, 0xdc, 0x02, 0xa8, 0x19, 0xe9, 0x68,
            0xf4, 0xb3, 0x4b, 0x33, 0x52, 0xb1, 0x6f, 0xec, 0x51, 0x1e, 0x24, 0xc7, 0xaa, 0xc8, 0xc9, 0x15,
            0x18, 0x48, 0x0a, 0xa3, 0xdf, 0x59, 0xf8, 0x92, 0x64, 0xd5, 0xfb, 0x8f, 0x99, 0xca, 0xea, 0x79,
            0x63, 0x84, 0x6b, 0x67, 0x2e, 0x28, 0xab, 0xcb, 0xf1, 0x2f, 0x71, 0x5c, 0x27, 0x72, 0xdb, 0x03,
            0xd1, 0x36, 0x65, 0x14, 0x7a, 0x23, 0xf3, 0x5f, 0xb0, 0x86, 0xe6, 0x8c, 0xa4, 0x6d, 0xf9, 0x22,
            0xce, 0x40, 0x01, 0x8e, 0xbd, 0x17, 0x7b, 0x66, 0xa1, 0x5b, 0xa9, 0xa2, 0xe5, 0x1a, 0xee, 0x3f,
            0x85, 0xeb, 0xef, 0xff, 0x4d, 0xfc, 0xb9, 0xd3, 0x5d, 0x53, 0xd4, 0x76, 0x49, 0xbc, 0x41, 0xc0,
            0x39, 0x21, 0x74, 0xed, 0x54, 0xd7, 0xc5, 0x8a, 0xd8, 0xc4, 0xfe, 0x29, 0x9a, 0x6e, 0x7d, 0xb8,
            0xc2, 0x55, 0x1d, 0xfd, 0x05, 0x42, 0x4e, 0x3d, 0xe8, 0x60, 0xe2, 0x75, 0x6c, 0x7f, 0x45, 0xbe,
            0x47, 0x44, 0xbb, 0xe0, 0x3a, 0xb6, 0xa5, 0x77, 0x34, 0x0b, 0x37, 0x32, 0x8d, 0x35, 0xf0, 0x80
        };

        // The 'golden' expected value based on the seed 7878 (see above comment)
        AZStd::vector<float> expectedOutput = 
        {
            0.50000f, 0.54557f, 0.51378f, 0.48007f,
            0.41741f, 0.49420f, 0.54927f, 0.54314f,
            0.49841f, 0.52041f, 0.55258f, 0.58404f,
            0.52507f, 0.50288f, 0.61527f, 0.58024f
        };

        auto entity = CreateEntity();
        auto* mockGradientSignal = entity->CreateComponent<MockGradientSignal>(config);

        mockGradientSignal->SetPerlinNoisePermutationTableForTest(testPerlinPermutationTable);

        GradientSignal::GradientTransformConfig gradientTransformConfig;
        entity->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);
        entity->CreateComponent<MockShapeComponent>();
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
        entity->CreateComponent<GradientSignal::RandomGradientComponent>(config);

        GradientSignal::GradientTransformConfig gradientTransformConfig;
        entity->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);
        entity->CreateComponent<MockShapeComponent>();
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

// This uses custom test / benchmark hooks so that we can load LmbrCentral and use Shape components in our unit tests and benchmarks.
AZ_UNIT_TEST_HOOK(new UnitTest::GradientSignalTestEnvironment, UnitTest::GradientSignalBenchmarkEnvironment);
