/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

class SceneCoreTestEnvironment
    : public AZ::Test::ITestEnvironment
{
protected:
    void SetupEnvironment() override
    {
    }

    void TeardownEnvironment() override
    {
    }
};

AZ_UNIT_TEST_HOOK(new SceneCoreTestEnvironment);
