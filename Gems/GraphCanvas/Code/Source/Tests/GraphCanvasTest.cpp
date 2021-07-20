/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"
#include <AzTest/AzTest.h>

class GraphCanvasTest
    : public ::testing::Test
{
};

TEST_F(GraphCanvasTest, Sanity_Pass)
{
    EXPECT_TRUE(1 == 1);
}

AZ_UNIT_TEST_HOOK();
