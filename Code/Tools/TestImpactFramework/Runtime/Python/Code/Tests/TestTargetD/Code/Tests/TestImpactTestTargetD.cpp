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
    class TestFixture1
        : public AllocatorsTestFixture
    {
    };

    class DISABLED_TestFixture2
        : public AllocatorsTestFixture
    {
    };

    template<typename T>
    class TestFixtureWithTypes1
        : public AllocatorsTestFixture
    {
    };

    template<typename T>
    class DISABLED_TestFixtureWithTypes2
        : public AllocatorsTestFixture
    {
    };

    class TestFixtureWithParams1
        : public AllocatorsTestFixture
        , public ::testing::WithParamInterface<AZStd::tuple<int, char, float>>
    {
    };

    class DISABLED_TestFixtureWithParams2
        : public AllocatorsTestFixture
        , public ::testing::WithParamInterface<AZStd::tuple<int, char, float>>
    {
    };

    using TestTypes = testing::Types<int, float, double, char>;
    TYPED_TEST_CASE(TestFixtureWithTypes1, TestTypes);
    TYPED_TEST_CASE(DISABLED_TestFixtureWithTypes2, TestTypes);

    TEST(TestCase, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, DISABLED_Test2_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, Test3_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, Test4_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, Test5_WillPass)
    {
        SUCCEED();
    }

    TEST_F(TestFixture1, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST_F(TestFixture1, Test2_WillPass)
    {
        SUCCEED();
    }

    TEST_F(DISABLED_TestFixture2, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST_F(DISABLED_TestFixture2, Test2_WillPass)
    {
        SUCCEED();
    }

    TEST_P(TestFixtureWithParams1, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST_P(TestFixtureWithParams1, DISABLED_Test2_WillPass)
    {
        SUCCEED();
    }

    TEST_P(DISABLED_TestFixtureWithParams2, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST_P(DISABLED_TestFixtureWithParams2, DISABLED_Test2_WillPass)
    {
        SUCCEED();
    }

    INSTANTIATE_TEST_CASE_P(
        PermutationA,
        TestFixtureWithParams1,
        ::testing::Combine(
            ::testing::Values(1, 2, 4),
            ::testing::Values(3, 5, 7),
            ::testing::Values(-0.0f, 0.0f, 1.0f)
        ));

    INSTANTIATE_TEST_CASE_P(
        ,
        TestFixtureWithParams1,
        ::testing::Combine(
            ::testing::Values(8, 16, 32),
            ::testing::Values(9, 13, 17),
            ::testing::Values(-10.0f, 0.05f, 10.0f)
        ));

    INSTANTIATE_TEST_CASE_P(
        PermutationA,
        DISABLED_TestFixtureWithParams2,
        ::testing::Combine(
            ::testing::Values(1, 2, 4),
            ::testing::Values(3, 5, 7),
            ::testing::Values(-0.0f, 0.0f, 1.0f)
        ));

    INSTANTIATE_TEST_CASE_P(
        ,
        DISABLED_TestFixtureWithParams2,
        ::testing::Combine(
            ::testing::Values(8, 16, 32),
            ::testing::Values(9, 13, 17),
            ::testing::Values(-10.0f, 0.05f, 10.0f)
        ));

    TYPED_TEST(TestFixtureWithTypes1, Test1_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(TestFixtureWithTypes1, DISABLED_Test2_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(TestFixtureWithTypes1, Test3_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(DISABLED_TestFixtureWithTypes2, Test1_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(DISABLED_TestFixtureWithTypes2, DISABLED_Test2_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(DISABLED_TestFixtureWithTypes2, Test3_WillPass)
    {
        SUCCEED();
    }
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
