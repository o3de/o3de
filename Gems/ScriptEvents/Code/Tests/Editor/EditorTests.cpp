/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"
#include <AzTest/AzTest.h>

class ScriptEventsEditorTests
    : public ::testing::Test
{
protected:
    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

// Provide a stub test so that AssetProcessorBatch.exe can run Editor Test builds safely.
TEST_F(ScriptEventsEditorTests, StubTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
