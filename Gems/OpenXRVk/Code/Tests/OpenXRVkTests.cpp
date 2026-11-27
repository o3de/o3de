/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include "OpenXRVkTests.h"

void OpenXRVkTest::SetUp()
{
    SetupInternal();
}

void OpenXRVkTest::TearDown()
{
    TearDownInternal();
}

#ifndef O3DE_TRAIT_DISABLE_ALL_OPENXRVK_TESTS

TEST_F(OpenXRVkTest, PassThisTest)
{
    EXPECT_TRUE(true);
}

#ifdef O3DE_TRAIT_DISABLE_FAILED_OPENXRVK_TESTS
TEST_F(OpenXRVkTest, DISABLED_ExpectTrue)
#else
TEST_F(OpenXRVkTest, ExpectTrue)
#endif // O3DE_TRAIT_DISABLE_FAILED_OPENXRVK_TESTS
{
    EXPECT_TRUE(false);
}

#endif // !O3DE_TRAIT_DISABLE_ALL_OPENXRVK_TESTS

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
