/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "CrySystem_precompiled.h"
#include <AzTest/AzTest.h>
#include <Cry_Math.h>

namespace UnitTest
{
    class CryMathTestFixture
        : public ::testing::Test
    {};

    TEST_F(CryMathTestFixture, InverserSqrt_HasAtLeast22BitsOfAccuracy)
    {
        float testFloat(0.336950600);
        const float result = isqrt_safe_tpl(testFloat * testFloat);
        const float epsilon = 0.00001f;
        EXPECT_NEAR(2.96779, result, epsilon);
    }

    TEST_F(CryMathTestFixture, SimdSqrt_HasAtLeast23BitsOfAccuracy)
    {
        float testFloat(3434.34839439);
        const float result = sqrt_tpl(testFloat);
        const float epsilon = 0.00001f;
        EXPECT_NEAR(58.60331, result, epsilon);
    }
}
