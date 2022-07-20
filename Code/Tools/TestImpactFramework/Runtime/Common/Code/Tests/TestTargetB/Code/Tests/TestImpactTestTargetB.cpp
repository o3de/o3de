/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
