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

#include <TestImpactConfigurationFactory.h>

#include <AzTest/AzTest.h>

#include <fstream>

namespace UnitTest
{
    TEST(FOO, BAR)
    {
        std::ifstream configFile("C:\\o3de_TIF_Feature\\windows_vs2019\\bin\\debug\\tiaf.debug.json");
        AZStd::string configData((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());
        const auto configuration = TestImpact::ConfigurationFactory(configData);
    }
}
