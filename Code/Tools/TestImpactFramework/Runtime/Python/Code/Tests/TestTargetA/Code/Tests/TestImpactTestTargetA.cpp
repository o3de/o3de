/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class TestFixture
        : public AllocatorsTestFixture
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

    TEST(TestCase, Test4_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, Test5_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, Test6_WillPass)
    {
        SUCCEED();
    }

    TEST(TestCase, Test7_WillFail)
    {
        FAIL();
    }

    TEST_F(TestFixture, Test1_WillPass)
    {
        SUCCEED();
    }

    TEST_F(TestFixture, Test2_WillPass)
    {
        SUCCEED();
    }

    TEST_F(TestFixture, Test3_WillPass)
    {
        SUCCEED();
    }
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
