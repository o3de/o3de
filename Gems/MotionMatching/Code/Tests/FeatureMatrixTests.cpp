/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Fixture.h>
#include <FeatureMatrix.h>

namespace EMotionFX::MotionMatching
{
    class FeatureMatrixFixture
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

    TEST_F(FeatureMatrixFixture, AccessOperators)
    {
        EXPECT_FLOAT_EQ(m_featureMatrix(1, 1), 5.0f);
        EXPECT_FLOAT_EQ(m_featureMatrix(0, 2), 3.0f);
        EXPECT_FLOAT_EQ(m_featureMatrix.coeff(2, 1), 8.0f);
        EXPECT_FLOAT_EQ(m_featureMatrix.coeff(1, 2), 6.0f);
    }

    TEST_F(FeatureMatrixFixture, SetValue)
    {
        m_featureMatrix(1, 1) = 100.0f;
        EXPECT_FLOAT_EQ(m_featureMatrix(1, 1), 100.0f);
    }

    TEST_F(FeatureMatrixFixture, Size)
    {
        EXPECT_EQ(m_featureMatrix.size(), 9);
        EXPECT_EQ(m_featureMatrix.rows(), 3);
        EXPECT_EQ(m_featureMatrix.cols(), 3);
    }
} // EMotionFX::MotionMatching
