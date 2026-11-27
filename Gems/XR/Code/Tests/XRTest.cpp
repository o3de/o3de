/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include "XRTest.h"

void XRTest::SetUp()
{
    SetupInternal();
}

void XRTest::TearDown()
{
    TearDownInternal();
}

#ifndef O3DE_TRAIT_DISABLE_ALL_XR_TESTS

TEST_F(XRTest, PassThisTest)
{
    EXPECT_TRUE(true);
}

#ifdef O3DE_TRAIT_DISABLE_FAILED_XR_TESTS
TEST_F(XRTest, DISABLED_ExpectTrue)
#else
TEST_F(XRTest, ExpectTrue)
#endif // defined O3DE_TRAIT_DISABLE_FAILED_XR_TESTS
{
    EXPECT_TRUE(false);
}

#endif // !O3DE_TRAIT_DISABLE_ALL_XR_TESTS

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
