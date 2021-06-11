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

#include <AzTest/AzTest.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/containers/fixed_vector.h>

DECLARE_AZ_UNIT_TEST_MAIN();

int runDefaultRunner(int argc, char* argv[])
{
    INVOKE_AZ_UNIT_TEST_MAIN(nullptr)
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        // if no parameters are provided, add the --unittests parameter
        constexpr size_t MaxCommandArgsCount = 128;
        using ArgumentContainer = AZStd::fixed_vector<char*, MaxCommandArgsCount>;

        constexpr AZStd::basic_fixed_string unittestArg{ "--unittests" };
        ArgumentContainer argContainer{ argv[0], const_cast<char*>(unittestArg.data()) };

        return runDefaultRunner(aznumeric_cast<int>(argContainer.size()), argContainer.data());
    }
    INVOKE_AZ_UNIT_TEST_MAIN(nullptr); 
    return 0;
}
