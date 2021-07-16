/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/UnitTest.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX
{
    class MotionDataTests
        : public SystemComponentFixture
        , public UnitTest::TraceBusRedirector
    {
    public:
        void SetUp()
        {
            UnitTest::TraceBusRedirector::BusConnect();
            SystemComponentFixture::SetUp();
        }

        void TearDown()
        {
            SystemComponentFixture::TearDown();
            UnitTest::TraceBusRedirector::BusDisconnect();
        }
    };

    TEST_F(MotionDataTests, CalculateSampleInformation)
    {
        struct TestSample
        {
            float m_duration;
            float m_sampleRate;
            float m_expectedSampleRate;
            float m_expectedSampleSpacing;
            size_t m_expectedNumSamples;
        };

        AZStd::vector<TestSample> samples{
            { 1.0f, 10.0f, 10.0f, 0.1f, 11 },
            { 1.0f, 3.0f, 3.0f, 1.0f / 3.0f, 4 },
            { 0.5f, 1.0f, 2.0f, 0.5f, 2 },
            { 1.05f, 10.0f, 1.0f / (1.05f / 10.0f), 1.05f / 10.0f, 11 }
        };

        for (const TestSample& sample : samples)
        {
            float sampleRate = sample.m_sampleRate;
            float sampleSpacing = -1.0f;
            size_t numSamples = InvalidIndex;
            MotionData::CalculateSampleInformation(sample.m_duration, sampleRate, numSamples, sampleSpacing);
            EXPECT_NEAR(sampleSpacing, sample.m_expectedSampleSpacing, 0.0001f);
            EXPECT_NEAR(sampleRate, sample.m_expectedSampleRate, 0.0001f);
            EXPECT_EQ(numSamples, sample.m_expectedNumSamples);
        }
    }

    TEST_F(MotionDataTests, CalculateNumRequiredSamples)
    {
        EXPECT_EQ(MotionData::CalculateNumRequiredSamples(1.0f, 0.1f), 11);
        EXPECT_EQ(MotionData::CalculateNumRequiredSamples(1.0f, 2.0f), 2);
        EXPECT_EQ(MotionData::CalculateNumRequiredSamples(1.0f, 0.333333f), 4);
        EXPECT_EQ(MotionData::CalculateNumRequiredSamples(1.0f, 1.0f), 2);
    }

    TEST_F(MotionDataTests, CalculateInterpolationIndices)
    {
        struct TestSample
        {
            float m_sampleTime = 0.0f;
            size_t m_indexA = InvalidIndex;
            size_t m_indexB = InvalidIndex;
            float m_t = -1.0f;
        };

        AZStd::vector<TestSample> testSamples = {
            {
                // Negative time value, out of range.
                -1.0f, // Sample time.
                0, // Expected IndexA.
                0, // Expected IndexB.
                0.0f // Expected t.
            },
            {
                // Exactly on the first sample.
                0.0f,
                0,
                1,
                0.0f
            },
            {
                // Exactly on the last sample.
                1.0f,
                10,
                10,
                0.0f
            },
            {
                // Exactly on the second sample.
                0.1f,
                1,
                2,
                0.0f
            },
            {
                // In between two samples.
                0.15f,
                1,
                2,
                0.5f
            },
            {
                // Another in-between two samples test.
                0.725f,
                7,
                8,
                0.25f
            },
            {
                // Past the maximum duration.
                100.0f,
                10,
                10,
                0.0f
            }
        };

        // Create some track for our non-uniform test.
        AZStd::vector<float> track;
        const float sampleSpacing = 0.1f;
        const float duration = 1.0f;
        const size_t numSamples = 11;
        track.resize(numSamples);
        for (size_t i = 0; i < numSamples; ++i)
        {
            track[i] = static_cast<float>(i * sampleSpacing);
        }

        for (TestSample& testSample : testSamples)
        {
            // Test uniform sampling.
            size_t indexA = InvalidIndex;
            size_t indexB = InvalidIndex;
            float t = -1.0f;
            MotionData::CalculateInterpolationIndicesUniform(testSample.m_sampleTime, sampleSpacing, duration, numSamples, indexA, indexB, t);
            EXPECT_EQ(indexA, testSample.m_indexA);
            EXPECT_EQ(indexB, testSample.m_indexB);
            EXPECT_TRUE(AZ::IsClose(t, testSample.m_t, 0.0001f));

            // Test non-uniform sampling.
            indexA = InvalidIndex;
            indexB = InvalidIndex;
            t = -1.0f;
            MotionData::CalculateInterpolationIndicesNonUniform(track, testSample.m_sampleTime, indexA, indexB, t);
            EXPECT_EQ(indexA, testSample.m_indexA);
            EXPECT_EQ(indexB, testSample.m_indexB);
            EXPECT_TRUE(AZ::IsClose(t, testSample.m_t, 0.0001f));
        }
    }
}; // namespace EMotionFX
