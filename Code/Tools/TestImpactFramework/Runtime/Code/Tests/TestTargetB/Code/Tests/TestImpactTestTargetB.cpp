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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/tuple.h>

namespace UnitTest
{
    class TestFixture
        : public AllocatorsTestFixture
    {
    };

    class TestFixtureWithParams
        : public AllocatorsTestFixture
        , public ::testing::WithParamInterface<AZStd::tuple<int, char, float>>
    {
    };

    TEST(TestCase, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, Test2_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, Test3_WillPass)
    {
        SUCCEED();
    }

    TEST_F(TestFixture, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST_P(TestFixtureWithParams, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST_P(TestFixtureWithParams, Test2_WillPass)
    {
        SUCCEED();
    }

    INSTANTIATE_TEST_CASE_P(
        PermutationA,
        TestFixtureWithParams,
        ::testing::Combine(
            ::testing::Values(1, 2, 4),
            ::testing::Values(3, 5, 7),
            ::testing::Values(-0.0f, 0.0f, 1.0f)
        ));

    INSTANTIATE_TEST_CASE_P(
        ,
        TestFixtureWithParams,
        ::testing::Combine(
            ::testing::Values(8, 16, 32),
            ::testing::Values(9, 13, 17),
            ::testing::Values(-10.0f, 0.05f, 10.0f)
        ));
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
