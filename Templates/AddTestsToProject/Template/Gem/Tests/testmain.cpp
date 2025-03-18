// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <AzTest/AzTest.h>

 TEST(MyTestSuiteName, ExampleTest)
 {
     ASSERT_TRUE(true);
 }

 // If you need to have your own environment preconditions
 // you can replace the test env with your own.
 AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
 