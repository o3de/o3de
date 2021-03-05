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
#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include "Util/PathUtil.h"
#include <CrySystemBus.h>

TEST(PathUtil, GamePathToFullPath_DoesNotBufferOverflow)
{
    // There are no test assertions in this test because the purpose is just to verify that the test runs without crashing
    QString pngExtension(".png");

    // Create a string of lenth AZ_MAX_PATH_LEN that ends in .png
    QString longStringMaxPath(AZ_MAX_PATH_LEN, 'x');
    longStringMaxPath.replace(longStringMaxPath.length() - pngExtension.length(), longStringMaxPath.length(), pngExtension);
    Path::GamePathToFullPath(longStringMaxPath);

    QString longStringMaxPathPlusOne(AZ_MAX_PATH_LEN + 1, 'x');
    longStringMaxPathPlusOne.replace(longStringMaxPathPlusOne.length() - pngExtension.length(), longStringMaxPathPlusOne.length(), pngExtension);
    Path::GamePathToFullPath(longStringMaxPathPlusOne);
}
