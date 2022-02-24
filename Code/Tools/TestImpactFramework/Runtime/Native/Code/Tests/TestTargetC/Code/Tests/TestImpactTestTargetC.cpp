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

    template<typename T>
    class TestFixtureWithTypes
        : public AllocatorsTestFixture
    {
    };

    using TestTypes = testing::Types<int, float, double, char>;
    TYPED_TEST_CASE(TestFixtureWithTypes, TestTypes);

    TEST_F(TestFixture, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST_F(TestFixture, Test2_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(TestFixtureWithTypes, Test1_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(TestFixtureWithTypes, Test2_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(TestFixtureWithTypes, Test3_WillPass)
    {
        SUCCEED();
    }

    TYPED_TEST(TestFixtureWithTypes, Test4_WillPass)
    {
        SUCCEED();
    }
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
