/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#include "StdAfx.h"
#include <gtest/gtest.h>

// minimal export required for AzTestScanner
extern "C" __declspec(dllexport) int AzRunUnitTests()
{
    int argc = 0;
    wchar_t* argv[1] = { L"" };

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(MetricsPluginSanityTest, Sanity)
{
    EXPECT_EQ(1, 1);
}
