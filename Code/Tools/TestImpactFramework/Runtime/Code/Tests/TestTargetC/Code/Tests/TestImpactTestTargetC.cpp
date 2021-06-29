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
