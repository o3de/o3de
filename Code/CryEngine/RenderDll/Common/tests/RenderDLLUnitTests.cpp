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
#include "RenderDll_precompiled.h"
#include <AzTest/AzTest.h>

TEST(RenderDllUnitTests, TestPathUtilityHelpers)
{
    //normal cases
    const char* somefile = "some.tga";
    const char* somefile_ext = somefile + 4;
    const char* path_somefile = "path/some.tga";
    const char* path_somefile_ext = path_somefile + 9;
    const char* path_somefile2 = "path\\some.tga";
    const char* path_somefile2_ext = path_somefile2 + 9;

    EXPECT_TRUE(fpGetExtension(somefile) == somefile_ext);               //filename
    EXPECT_TRUE(fpGetExtension(path_somefile) == path_somefile_ext);     //path + filename
    EXPECT_TRUE(fpGetExtension(path_somefile2) == path_somefile2_ext);   //path + filename

    //edge cases
    const char* path_somefile3 = "path\\some.tga\\some";
    const char* path_somefile4 = "path\\some.tga/some";
    const char* path_somefile5 = "path/some.tga\\some";

    EXPECT_TRUE(fpGetExtension(path_somefile3) == nullptr);          //malformed string
    EXPECT_TRUE(fpGetExtension(path_somefile4) == nullptr);          //malformed string
    EXPECT_TRUE(fpGetExtension(path_somefile5) == nullptr);          //malformed string
    EXPECT_TRUE(fpGetExtension("") == nullptr);                      //empty string
    EXPECT_TRUE(fpGetExtension(nullptr) == nullptr);                 //nullptr
}
