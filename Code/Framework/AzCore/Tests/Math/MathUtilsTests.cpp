/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Lerp, Test)
    {
        // Float
        EXPECT_EQ(2.5f, AZ::Lerp(2.0f, 4.0f, 0.25f));
        EXPECT_EQ(6.0f, AZ::Lerp(2.0f, 4.0f, 2.0f));

        // Double
        EXPECT_EQ(3.5, AZ::Lerp(2.0, 4.0, 0.75));
        EXPECT_EQ(0.0, AZ::Lerp(2.0, 4.0, -1.0));
    }

    TEST(MATH_LerpInverse, Test)
    {
        // Float
        EXPECT_NEAR(0.25f, AZ::LerpInverse(2.0f, 4.0f, 2.5f), 0.0001f);
        EXPECT_NEAR(2.0f, AZ::LerpInverse(2.0f, 4.0f, 6.0f), 0.0001f);

        // Double
        EXPECT_NEAR(0.75, AZ::LerpInverse(2.0, 4.0, 3.5), 0.0001);
        EXPECT_NEAR(-1.0, AZ::LerpInverse(2.0, 4.0, 0.0), 0.0001);

        // min/max need to be substantially different to return a useful t value

        // Float
        const float epsilonF = std::numeric_limits<float>::epsilon();
        const float doesntMatterF = std::numeric_limits<float>::signaling_NaN();
        float lowerF = 2.3f, upperF = 2.3f;
        EXPECT_EQ(0.0f, AZ::LerpInverse(lowerF, upperF, doesntMatterF));
        EXPECT_EQ(0.0f, AZ::LerpInverse(0.0f, 0.5f * epsilonF, doesntMatterF));
        EXPECT_EQ(0.0f, AZ::LerpInverse(0.0f, 5.0f * epsilonF, 0.0f));
        EXPECT_NEAR(0.4f, AZ::LerpInverse(0.0f, 5.0f * epsilonF, 2.0f * epsilonF), epsilonF);
        EXPECT_NEAR(0.6f, AZ::LerpInverse(1.0f, 1.0f + 5.0f * epsilonF, 1.0f + 3.0f * epsilonF), epsilonF);
        EXPECT_NEAR(1.0f, AZ::LerpInverse(1.0f, 1.0f + 5.0f * epsilonF, 1.0f + 5.0f * epsilonF), epsilonF);

        // Double
        const double epsilonD = std::numeric_limits<double>::epsilon();
        const double doesntMatterD = std::numeric_limits<double>::signaling_NaN();
        double lowerD = 2.3, upperD = 2.3;
        EXPECT_EQ(0.0, AZ::LerpInverse(lowerD, upperD, doesntMatterD));
        EXPECT_EQ(0.0, AZ::LerpInverse(0.0, 0.5 * epsilonD, doesntMatterD));
        EXPECT_EQ(0.0, AZ::LerpInverse(0.0, 5.0 * epsilonD, 0.0));
        EXPECT_NEAR(0.4, AZ::LerpInverse(0.0, 5.0 * epsilonD, 2.0 * epsilonD), epsilonD);
        EXPECT_NEAR(0.6, AZ::LerpInverse(1.0, 1.0 + 5.0 * epsilonD, 1.0 + 3.0 * epsilonD), epsilonD);
        EXPECT_NEAR(1.0, AZ::LerpInverse(1.0, 1.0 + 5.0 * epsilonD, 1.0 + 5.0 * epsilonD), epsilonD);
    }
}
