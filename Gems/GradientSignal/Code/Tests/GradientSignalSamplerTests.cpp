/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestFixtures.h>

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Vector2.h>
#include <AZTestShared/Math/MathTestHelpers.h>

#include <GradientSignal/GradientSampler.h>

namespace UnitTest
{
    struct GradientSignalSamplerTestsFixture : public GradientSignalTest
    {
        void TestGradientSampler(
            GradientSignal::GradientSampler& gradientSampler,
            const AZStd::vector<float>& gradientInput,
            const AZStd::vector<float>& expectedOutput,
            int dataSize)
        {
            auto mockGradient = CreateEntity();
            const AZ::EntityId gradientId = mockGradient->GetId();
            MockGradientArrayRequestsBus mockGradientRequestsBus(gradientId, gradientInput, dataSize);

            gradientSampler.m_gradientId = gradientId;

            TestFixedDataSampler(expectedOutput, dataSize, gradientSampler);
        }
    };

    TEST_F(GradientSignalSamplerTestsFixture, DefaultSamplerReturnsExactGradientValues)
    {
        const int dataSize = 3; // 3x3 data

        // The default gradient sampler should return back the exact same set of values that our mock gradient defines.
        AZStd::vector<float> mockInputAndExpectedOutput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };
        GradientSignal::GradientSampler gradientSampler;

        TestGradientSampler(gradientSampler, mockInputAndExpectedOutput, mockInputAndExpectedOutput, dataSize);
    }

    TEST_F(GradientSignalSamplerTestsFixture, SamplerWithInvertReturnsInvertedGradientValues)
    {
        // If "invertInput" is set, the gradient sampler should return back values that are inverted from the mock gradient.

        const int dataSize = 3; // 3x3 data

        AZStd::vector<float> mockInput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        AZStd::vector<float> expectedOutput =
        {
            1.0f, 0.9f, 0.8f,
            0.6f, 0.5f, 0.4f,
            0.2f, 0.1f, 0.0f
        };
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_invertInput = true;

        TestGradientSampler(gradientSampler, mockInput, expectedOutput, dataSize);
    }

    TEST_F(GradientSignalSamplerTestsFixture, SamplerWithOpacityReturnsGradientValuesAdjustedForOpacity)
    {
        // If "opacity" is set, the gradient sampler should return back values that match the mock gradient * opacity.

        const int dataSize = 3; // 3x3 data

        AZStd::vector<float> mockInput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.05f, 0.1f,
            0.2f, 0.25f, 0.3f,
            0.4f, 0.45f, 0.5f
        };
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_opacity = 0.5f;

        TestGradientSampler(gradientSampler, mockInput, expectedOutput, dataSize);
    }

    TEST_F(GradientSignalSamplerTestsFixture, SamplerWithTranslateReturnsTranslatedGradientValues)
    {
        // If the transform is enabled, the gradient sampler should return back values that have been transformed.
        // In this test, we're setting the translation.

        const int dataSize = 3; // 3x3 data

        AZStd::vector<float> mockInput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // We're translating one to the left in world space, which will translate one to the right in gradient lookup space,
        // which means each output value should be the one that's one position to the right in the input, with wrapping.
        // For example, the output for X=0 should match the input for X=1, the output for X=1 should match the input for X=2,
        // and the output for X=2 should match the input for X=0, because of the wrapping.
        AZStd::vector<float> expectedOutput =
        {
            0.1f, 0.2f, 0.0f,
            0.5f, 0.6f, 0.4f,
            0.9f, 1.0f, 0.8f
        };

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_enableTransform = true;
        gradientSampler.m_translate = AZ::Vector3(-1.0f, 0.0f, 0.0f);

        TestGradientSampler(gradientSampler, mockInput, expectedOutput, dataSize);
    }

    TEST_F(GradientSignalSamplerTestsFixture, SamplerWithRotationReturnsRotatedGradientValues)
    {
        // If the transform is enabled, the gradient sampler should return back values that have been transformed.
        // In this test, we're setting the rotation.

        const int dataSize = 3; // 3x3 data

        AZStd::vector<float> mockInput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // We're rotating 90 degrees to the right in world space, which should cause our output values to match the inputs
        // rotated 90 degrees to the left. This will cause our input lookups to be at:
        // (0,0) (0,1) (0,2) / (-1,0) (-1,1) (-1,2) / (-2,0) (-2,1) (-2,2)
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.4f, 0.8f,
            0.2f, 0.6f, 1.0f,
            0.1f, 0.5f, 0.9f
        };

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_enableTransform = true;
        gradientSampler.m_rotate = AZ::Vector3(0.0f, 0.0f, -90.0f);

        TestGradientSampler(gradientSampler, mockInput, expectedOutput, dataSize);
    }

    TEST_F(GradientSignalSamplerTestsFixture, SamplerWithScaleReturnsScaledGradientValues)
    {
        // If the transform is enabled, the gradient sampler should return back values that have been transformed.
        // In this test, we're setting the rotation.

        const int dataSize = 3; // 3x3 data

        AZStd::vector<float> mockInput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        // With a scale of 1/2, our output lookup will be every 2 points in input space.
        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.2f, 0.1f,
            0.8f, 1.0f, 0.9f,
            0.4f, 0.6f, 0.5f
        };

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_enableTransform = true;
        gradientSampler.m_scale = AZ::Vector3(0.5f, 0.5f, 1.0f);

        TestGradientSampler(gradientSampler, mockInput, expectedOutput, dataSize);
    }

    TEST_F(GradientSignalSamplerTestsFixture, SamplerWithInputLevelsReturnsLeveledGradientValues)
    {
        // If levels are enabled, the gradient sampler should return back values that have been leveled.
        // Input levels are defined as ((x - min) / (max - min)) ^ (1 / mid), where the "((x - min) / (max - min))" term
        // is clamped to the 0 - 1 range.
        // In this test, we're leaving the output levels alone, and setting the input levels to min=0.5, max=1.0, mid=0.5, so
        // the results should be ((x - 0.5) / (1.0 - 0.5)) ^ (1 / 0.5), or (2x - 1) ^ 2.

        const int dataSize = 3; // 3x3 data

        AZStd::vector<float> mockInput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        AZStd::vector<float> expectedOutput =
        {
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.04f,
            0.36f, 0.64f, 1.0f
        };

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_enableLevels = true;
        gradientSampler.m_inputMin = 0.5f;
        gradientSampler.m_inputMid = 0.5f;
        gradientSampler.m_inputMax = 1.0f;
        gradientSampler.m_outputMin = 0.0f;
        gradientSampler.m_outputMax = 1.0f;

        TestGradientSampler(gradientSampler, mockInput, expectedOutput, dataSize);
    }

    TEST_F(GradientSignalSamplerTestsFixture, SamplerWithOutputLevelsReturnsLeveledGradientValues)
    {
        // If levels are enabled, the gradient sampler should return back values that have been leveled.
        // In this test, we're leaving the input levels alone, and setting the output levels to 0.5 - 1.0, so
        // the results should be the input values mapped from 0.0 - 1.0 to 0.5 - 1.0, or (0.5 + input/2).

        const int dataSize = 3; // 3x3 data

        AZStd::vector<float> mockInput =
        {
            0.0f, 0.1f, 0.2f,
            0.4f, 0.5f, 0.6f,
            0.8f, 0.9f, 1.0f
        };

        AZStd::vector<float> expectedOutput =
        {
            0.5f, 0.55f, 0.6f,
            0.7f, 0.75f, 0.8f,
            0.9f, 0.95f, 1.0f
        };

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_enableLevels = true;
        gradientSampler.m_inputMin = 0.0f;
        gradientSampler.m_inputMid = 1.0f;
        gradientSampler.m_inputMax = 1.0f;
        gradientSampler.m_outputMin = 0.5f;
        gradientSampler.m_outputMax = 1.0f;

        TestGradientSampler(gradientSampler, mockInput, expectedOutput, dataSize);
    }
}


