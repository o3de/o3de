/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Runner.h>

AZ_TOOLS_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV)


namespace UnitTest
{
    using SerializeContextToolsFixture = UnitTest::LeakDetectionFixture;

    TEST_F(SerializeContextToolsFixture, SerializeContextTools_HelpOption_Works)
    {
        AZStd::fixed_string<32> appName{ "SerializeContextTools" };
        AZStd::fixed_string<32> helpArg{ "--help" };
        AZStd::fixed_vector<char*, 2> commandArgs{ appName.data(), helpArg.data() };
        EXPECT_EQ(0, SerializeContextTools::LaunchSerializeContextTools(int(commandArgs.size()),
            commandArgs.data()));
    }
}
