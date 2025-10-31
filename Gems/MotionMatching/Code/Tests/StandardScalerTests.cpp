/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FeatureMatrixStandardScaler.h>
#include <Fixture.h>

namespace EMotionFX::MotionMatching
{
    /*
        # ground truth test data generated in python using the StandardScaler from scikit-learn

        from sklearn.preprocessing import StandardScaler
        import numpy as np

        data = [[0, -1],
                [0, -1],
                [1, 1],
                [1, 1]]
        print("data")
        print(data)

        scaler = StandardScaler()
        print(scaler.fit(data))

        print("means")
        print(scaler.mean_)
        print("standard deviations")
        print(np.sqrt(scaler.var_))

        print("transformed data")
        print(scaler.transform(data))

        print("transform -> inverse_transform roundtrip")
        print(scaler.inverse_transform(scaler.transform(data)))

        print("transformed test sample")
        print(scaler.transform([[2, 2]]))
    */

    class StandardScalerFixture : public Fixture
    {
    public:
        static constexpr float s_testEpsilon = 1e-6f;

        // First transforms and then inverse-transforms the data and compares the roundtrip to the original data.
        void RoundtripTest(const FeatureMatrix& data, const FeatureMatrixTransformer& scaler, float epsilon = s_testEpsilon)
        {
            FeatureMatrix roundTrip = scaler.InverseTransform(scaler.Transform(data));

            const FeatureMatrix::Index numRows = data.rows();
            const FeatureMatrix::Index numColumns = data.cols();
            for (FeatureMatrix::Index column = 0; column < numColumns; ++column)
            {
                for (FeatureMatrix::Index row = 0; row < numRows; ++row)
                {
                    EXPECT_NEAR(data(row, column), roundTrip(row, column), epsilon)
                        << "Value at (" << row << ", " << column << ") does not match roundtrip value.";
                }
            }
        }
    };

    TEST_F(StandardScalerFixture, BasicTransform1)
    {
        FeatureMatrix m;
        m.resize(4, 2);
        m(0, 0) = 0.0f;
        m(0, 1) = -1.0f;
        m(1, 0) = 0.0f;
        m(1, 1) = -1.0f;
        m(2, 0) = 1.0f;
        m(2, 1) = 1.0f;
        m(3, 0) = 1.0f;
        m(3, 1) = 1.0f;

        StandardScaler scaler;
        EXPECT_TRUE(scaler.Fit(m));

        // Test mean and standard deviations
        const AZStd::vector<float>& means = scaler.GetMeans();
        const AZStd::vector<float>& standardDeviations = scaler.GetStandardDeviations();
        EXPECT_NEAR(means[0], 0.5f, s_testEpsilon);
        EXPECT_NEAR(means[1], 0.0f, s_testEpsilon);
        EXPECT_NEAR(standardDeviations[0], 0.5f, s_testEpsilon);
        EXPECT_NEAR(standardDeviations[1], 1.0f, s_testEpsilon);

        // Test transform
        EXPECT_NEAR(scaler.Transform(2.0f, 0), 3.0f, s_testEpsilon);
        EXPECT_NEAR(scaler.Transform(2.0f, 1), 2.0f, s_testEpsilon);

        RoundtripTest(m, scaler);
    }

    TEST_F(StandardScalerFixture, BasicTransform2)
    {
        FeatureMatrix m;
        m.resize(4, 2);
        m(0, 0) = 1.0f;
        m(0, 1) = -1.0f;
        m(1, 0) = 2.0f;
        m(1, 1) = -4.0f;
        m(2, 0) = 3.0f;
        m(2, 1) = 2.0f;
        m(3, 0) = 4.0f;
        m(3, 1) = 0.5f;

        StandardScaler scaler;
        EXPECT_TRUE(scaler.Fit(m));

        // Test mean and standard deviations
        const AZStd::vector<float>& means = scaler.GetMeans();
        const AZStd::vector<float>& standardDeviations = scaler.GetStandardDeviations();
        EXPECT_NEAR(means[0], 2.5f, s_testEpsilon);
        EXPECT_NEAR(means[1], -0.625f, s_testEpsilon);
        EXPECT_NEAR(standardDeviations[0], 1.11803399f, s_testEpsilon);
        EXPECT_NEAR(standardDeviations[1], 2.21852992f, s_testEpsilon);

        // Test transform
        EXPECT_NEAR(scaler.Transform(2.0f, 0), -0.4472136f, s_testEpsilon);
        EXPECT_NEAR(scaler.Transform(2.0f, 1), 1.18321596f, s_testEpsilon);

        RoundtripTest(m, scaler);
    }

    TEST_F(StandardScalerFixture, LargeValues)
    {
        const float largeValueTestEpsilon = 0.001f;

        FeatureMatrix m;
        m.resize(4, 2);
        m(0, 0) = 10000.0f;
        m(0, 1) = 4242.0f;
        m(1, 0) = -10000.0f;
        m(1, 1) = -4242.0f;
        m(2, 0) = 300.0f;
        m(2, 1) = 4242.0f;
        m(3, 0) = -250.0f;
        m(3, 1) = 4242.0f;

        StandardScaler scaler;
        EXPECT_TRUE(scaler.Fit(m));

        // Test mean and standard deviations
        const AZStd::vector<float>& means = scaler.GetMeans();
        const AZStd::vector<float>& standardDeviations = scaler.GetStandardDeviations();
        EXPECT_NEAR(means[0], 12.5f, largeValueTestEpsilon);
        EXPECT_NEAR(means[1], 2121.0f, largeValueTestEpsilon);
        EXPECT_NEAR(standardDeviations[0], 7073.75209843f, largeValueTestEpsilon);
        EXPECT_NEAR(standardDeviations[1], 3673.67976285f, largeValueTestEpsilon);

        // Test transform
        EXPECT_NEAR(scaler.Transform(2.0f, 0), -0.00148436f, s_testEpsilon);
        EXPECT_NEAR(scaler.Transform(2.0f, 1), -0.57680586, s_testEpsilon);

        RoundtripTest(m, scaler, largeValueTestEpsilon);
    }

    TEST_F(StandardScalerFixture, ClosebyValues1)
    {
        const float closebyValueTestEpsilon = 0.001f;

        FeatureMatrix m;
        m.resize(4, 2);
        m(0, 0) = 1.0f;
        m(0, 1) = 1.01f;
        m(1, 0) = 1.0f;
        m(1, 1) = 1.0f;
        m(2, 0) = 1.0f;
        m(2, 1) = 1.0f;
        m(3, 0) = 1.0f;
        m(3, 1) = 1.0f;

        StandardScaler scaler;
        EXPECT_TRUE(scaler.Fit(m));

        // Test mean and standard deviations
        const AZStd::vector<float>& means = scaler.GetMeans();
        const AZStd::vector<float>& standardDeviations = scaler.GetStandardDeviations();
        EXPECT_NEAR(means[0], 1.0f, s_testEpsilon);
        EXPECT_NEAR(means[1], 1.0025f, s_testEpsilon);
        EXPECT_NEAR(standardDeviations[0], 0.0f, s_testEpsilon);
        EXPECT_NEAR(standardDeviations[1], 0.00433013f, s_testEpsilon);

        // Test transform
        EXPECT_NEAR(scaler.Transform(2.0f, 0), 1.0, s_testEpsilon);
        EXPECT_NEAR(scaler.Transform(2.0f, 1), 230.36275741f, closebyValueTestEpsilon);

        RoundtripTest(m, scaler, closebyValueTestEpsilon);
    }

    TEST_F(StandardScalerFixture, ClosebyValues2)
    {
        const float closebyValueTestEpsilon = 0.001f;

        FeatureMatrix m;
        m.resize(4, 2);
        m(0, 0) = 100000.0f;
        m(0, 1) = 1.001f;
        m(1, 0) = 100000.0f;
        m(1, 1) = 1.0f;
        m(2, 0) = 100000.0f;
        m(2, 1) = 1.0f;
        m(3, 0) = 100000.0f;
        m(3, 1) = 1.0f;

        StandardScaler scaler;
        EXPECT_TRUE(scaler.Fit(m));

        // Test mean and standard deviations
        const AZStd::vector<float>& means = scaler.GetMeans();
        const AZStd::vector<float>& standardDeviations = scaler.GetStandardDeviations();
        EXPECT_NEAR(means[0], 1.00000e+05f, s_testEpsilon);
        EXPECT_NEAR(means[1], 1.00025e+00f, s_testEpsilon);
        EXPECT_NEAR(standardDeviations[0], 0.0f, s_testEpsilon);
        EXPECT_NEAR(standardDeviations[1], 0.00043301f, s_testEpsilon);

        // Test transform
        const float floatPrecisionErrorEpsilon = 0.2f;
        EXPECT_NEAR(scaler.Transform(2.0f, 0), -99998.0f, floatPrecisionErrorEpsilon);
        EXPECT_NEAR(scaler.Transform(2.0f, 1), 2308.82372649f, floatPrecisionErrorEpsilon);

        RoundtripTest(m, scaler, closebyValueTestEpsilon);
    }
} // namespace EMotionFX::MotionMatching
