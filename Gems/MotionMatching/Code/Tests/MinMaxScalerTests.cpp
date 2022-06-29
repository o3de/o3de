/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Fixture.h>
#include <FeatureMatrixMinMaxScaler.h>

namespace EMotionFX::MotionMatching
{
    class MinMaxScalerFixture
        : public Fixture
    {
    public:
        void SetUp() override
        {
            Fixture::SetUp();

            // Construct 3x3 matrix:
            // 1 2 3
            // 4 5 6
            // 7 8 9
            m_featureMatrix.resize(3, 3);

            float counter = 1.0f;
            for (size_t row = 0; row < 3; ++row)
            {
                for (size_t column = 0; column < 3; ++column)
                {
                    m_featureMatrix(row, column) = counter;
                    counter++;
                }
            }
        }

        FeatureMatrix m_featureMatrix;
    };

    TEST_F(MinMaxScalerFixture, MinMaxValues)
    {
        FeatureMatrix m;
        m.resize(3, 3);
        m(0,0) = 0.0f;    m(0,1) =-1.0f;    m(0,2) = 9.0f;
        m(1,0) = 0.5f;    m(1,1) = 5.0f;    m(1,2) = 6.0f;
        m(2,0) = 7.0f;    m(2,1) = 0.1f;    m(2,2) = 3.0f;

        MinMaxScaler minMaxScaler;
        EXPECT_TRUE(minMaxScaler.Fit(m));

        const AZStd::vector<float>& min = minMaxScaler.GetMin();
        const AZStd::vector<float>& max = minMaxScaler.GetMax();
        EXPECT_EQ(min[0], 0.0f); EXPECT_EQ(max[0], 7.0f);
        EXPECT_EQ(min[1],-1.0f); EXPECT_EQ(max[1], 5.0f);
        EXPECT_EQ(min[2], 3.0f); EXPECT_EQ(max[2], 9.0f);
    }

    TEST_F(MinMaxScalerFixture, Transform)
    {
        FeatureMatrix m;
        m.resize(3, 4);
        m(0,0) = 0.0f;    m(0,1) =-1.0f;    m(0,2) = 10.0f;    m(0, 3) = 3.0f;
        m(1,0) = 0.5f;    m(1,1) = 1.0f;    m(1,2) =-10.0f;    m(1, 3) = 3.0f;
        m(2,0) = 1.0f;    m(2,1) = 0.5f;    m(2,2) =-5.0f;     m(2, 3) = 3.0f;

        MinMaxScaler minMaxScaler;
        EXPECT_TRUE(minMaxScaler.Fit(m)); // default to normalization (feature range = [0, 1]
        FeatureMatrix t = minMaxScaler.Transform(m);

        EXPECT_EQ(t(0,0), 0.0f); EXPECT_EQ(t(0, 1), 0.0f); EXPECT_EQ(t(0, 2), 1.0f); EXPECT_EQ(t(0, 3), 3.0f);
        EXPECT_EQ(t(1,0), 0.5f); EXPECT_EQ(t(1, 1), 1.0f); EXPECT_EQ(t(1, 2), 0.0f); EXPECT_EQ(t(1, 3), 3.0f);
        EXPECT_EQ(t(2,0), 1.0f); EXPECT_EQ(t(2, 1), 0.75f);EXPECT_EQ(t(2, 2), 0.25f);EXPECT_EQ(t(1, 3), 3.0f);
    }

    TEST_F(MinMaxScalerFixture, TransformValueNoClipping)
    {
        FeatureMatrix m;
        m.resize(2, 1);
        m(0, 0) = -2.0f;
        m(1, 0) = 2.0f; // range = 4.0

        MinMaxScaler minMaxScaler;
        EXPECT_TRUE(minMaxScaler.Fit(m));

        EXPECT_EQ(minMaxScaler.Transform(-6.0f, 0), -1.0f);
        EXPECT_EQ(minMaxScaler.Transform(4.0f, 0), 1.5f);
    }

    TEST_F(MinMaxScalerFixture, TransformValueClip)
    {
        FeatureMatrix m;
        m.resize(2, 1);
        m(0, 0) = -2.0f;
        m(1, 0) = 2.0f;

        MinMaxScaler minMaxScaler;
        EXPECT_TRUE(minMaxScaler.Fit(m, {/*featureMin=*/0.0f, /*featureMax=*/1.0f, /*clip=*/true}));

        EXPECT_EQ(minMaxScaler.Transform(-6.0f, 0), 0.0f);
        EXPECT_EQ(minMaxScaler.Transform(8.0f, 0), 1.0f);
    }

    TEST_F(MinMaxScalerFixture, FeatureRangeTest)
    {
        FeatureMatrix m;
        m.resize(2, 1);
        m(0, 0) = -2.0f;
        m(1, 0) = 2.0f;

        MinMaxScaler minMaxScaler;
        EXPECT_TRUE(minMaxScaler.Fit(m, {/*featureMin=*/6.0f, /*featureMax=*/10.0f, /*clip=*/true}));

        EXPECT_EQ(minMaxScaler.Transform(-2.0f, 0), 6.0f);
        EXPECT_EQ(minMaxScaler.Transform(0.0f, 0), 8.0f);
        EXPECT_EQ(minMaxScaler.Transform(2.0f, 0), 10.0f);

        //--

        m(0, 0) = 10.0f;
        m(1, 0) = 20.0f;

        FeatureMatrixTransformer::Settings fitSettings;
        fitSettings.m_featureMin = -5.0f;
        fitSettings.m_featureMax = 5.0f;
        EXPECT_TRUE(minMaxScaler.Fit(m, fitSettings));

        EXPECT_EQ(minMaxScaler.Transform(10.0f, 0), -5.0f);
        EXPECT_EQ(minMaxScaler.Transform(15.0f, 0), 0.0f);
        EXPECT_EQ(minMaxScaler.Transform(20.0f, 0), 5.0f);
    }

    TEST_F(MinMaxScalerFixture, SameValues)
    {
        FeatureMatrix m;
        m.resize(3, 1);
        m(0, 0) = 2.0f;
        m(1, 0) = 2.0f;
        m(2, 0) = 2.0f;

        MinMaxScaler minMaxScaler;
        EXPECT_TRUE(minMaxScaler.Fit(m));

        EXPECT_EQ(minMaxScaler.GetMin()[0], 2.0f);
        EXPECT_EQ(minMaxScaler.GetMax()[0], 2.0f);

        EXPECT_EQ(minMaxScaler.Transform(2.0f, 0), 2.0f);
        EXPECT_EQ(minMaxScaler.InverseTransform(2.0f, 0), 2.0f);

        // Test out of data range.
        // In case the feature was constant, it is expected to not transform the value.
        EXPECT_EQ(minMaxScaler.Transform(0.0f, 0), 0.0f);
        EXPECT_EQ(minMaxScaler.Transform(10.0f, 0), 10.0f);

        // Test out of feature range.
        // As the feature is constant, no matter what the input is, the constant feature should be returned.
        EXPECT_EQ(minMaxScaler.InverseTransform(0.0f, 0), 2.0f);
        EXPECT_EQ(minMaxScaler.InverseTransform(10.0f, 0), 2.0f);
    }

    TEST_F(MinMaxScalerFixture, CloseEpsilonValues)
    {
        FeatureMatrix m;
        m.resize(3, 1);
        m(0, 0) = 2.0f + MinMaxScaler::s_epsilon;
        m(1, 0) = 2.0f - MinMaxScaler::s_epsilon;
        m(2, 0) = 2.0f;

        MinMaxScaler minMaxScaler;
        EXPECT_TRUE(minMaxScaler.Fit(m));

        EXPECT_EQ(minMaxScaler.GetMin()[0], 2.0f - MinMaxScaler::s_epsilon);
        EXPECT_EQ(minMaxScaler.GetMax()[0], 2.0f + MinMaxScaler::s_epsilon);

        EXPECT_EQ(minMaxScaler.Transform(2.0f, 0), 2.0f);
        EXPECT_EQ(minMaxScaler.InverseTransform(2.0f, 0), 2.0f);

        EXPECT_EQ(minMaxScaler.Transform(2.0f + MinMaxScaler::s_epsilon, 0), 2.0f + MinMaxScaler::s_epsilon);
        EXPECT_EQ(minMaxScaler.InverseTransform(2.0f + MinMaxScaler::s_epsilon, 0), 2.0f + MinMaxScaler::s_epsilon);

        // Test out of data range.
        // In case the feature was constant, it is expected to not transform the value.
        EXPECT_EQ(minMaxScaler.Transform(0.0f, 0), 0.0f);
        EXPECT_EQ(minMaxScaler.Transform(10.0f, 0), 10.0f);

        // Test out of feature range.
        // As the feature is constant, no matter what the input is, the constant feature should be returned.
        EXPECT_FLOAT_EQ(minMaxScaler.InverseTransform(0.0f, 0), 2.0f - MinMaxScaler::s_epsilon);
        EXPECT_FLOAT_EQ(minMaxScaler.InverseTransform(10.0f, 0), 2.0f + MinMaxScaler::s_epsilon);
    }

    TEST_F(MinMaxScalerFixture, SimpleRoundTrip)
    {
        FeatureMatrix m;
        m.resize(2, 1);
        m(0, 0) = -2.0f;
        m(1, 0) = 2.0f; // range = 4.0

        MinMaxScaler minMaxScaler;

        FeatureMatrixTransformer::Settings fitSettings;
        fitSettings.m_featureMin = -10.0f;
        fitSettings.m_featureMax = 10.0f;
        EXPECT_TRUE(minMaxScaler.Fit(m, fitSettings));

        const float tVal = minMaxScaler.Transform(0.0f, 0);
        EXPECT_EQ(tVal, 0.0f);

        const float orgVal = minMaxScaler.InverseTransform(tVal, 0);
        EXPECT_EQ(orgVal, 0.0f);
    }

    TEST_F(MinMaxScalerFixture, RoundTripFeatureRange)
    {
        FeatureMatrix m;
        m.resize(3, 4);
        m(0, 0) = 0.0f;    m(0, 1) = -1.0f;   m(0, 2) = 10.0f;     m(0, 3) = 3.0f;
        m(1, 0) = 0.5f;    m(1, 1) = 1.0f;    m(1, 2) = -10.0f;    m(1, 3) = 3.0f + MinMaxScaler::s_epsilon;
        m(2, 0) = 1.0f;    m(2, 1) = 0.5f;    m(2, 2) = -5.0f;     m(2, 3) = 3.0f;

        MinMaxScaler minMaxScaler;

        FeatureMatrixTransformer::Settings fitSettings;
        fitSettings.m_featureMin = -36.0f;
        fitSettings.m_featureMax = 250.0f;
        EXPECT_TRUE(minMaxScaler.Fit(m, fitSettings));

        FeatureMatrix t = minMaxScaler.Transform(m);
        FeatureMatrix inv = minMaxScaler.InverseTransform(t);

        const FeatureMatrix::Index numRows = m.rows();
        const FeatureMatrix::Index numColumns = m.cols();
        for (FeatureMatrix::Index row = 0; row < numRows; ++row)
        {
            for (FeatureMatrix::Index column = 0; column < numColumns; ++column)
            {
                EXPECT_EQ(inv(row, column), m(row, column));
            }
        }
    }
} // EMotionFX::MotionMatching
