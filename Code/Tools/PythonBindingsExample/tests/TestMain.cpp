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

#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

#include <AzTest/AzTest.h>
DECLARE_AZ_UNIT_TEST_MAIN();

int runDefaultRunner(int argc, char* argv[])
{
    INVOKE_AZ_UNIT_TEST_MAIN(nullptr);
    return 0;
}

int main(int argc, char* argv[])
{
    // ran with no parameters?
    if (argc == 1)
    {
        constexpr int defaultArgc = 2;
        char unittest_arg[] = "--unittests"; // Conversion from string literal to char* is not allowed per ISO C++11
        char* defaultArgv[defaultArgc] =
        {
            argv[0],
            unittest_arg
        };
        return runDefaultRunner(defaultArgc, defaultArgv);
    }
    INVOKE_AZ_UNIT_TEST_MAIN(nullptr); 
    return 0;
}
