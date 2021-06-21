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

#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <Application.h>

int main(int argc, char* argv[])
{
    int runSuccess = 0;

    // Call before using any Qt, or the app may not be able to locate Qt libs
    AzQtComponents::PrepareQtPaths();

    O3DE::ProjectManager::Application application(&argc, &argv);
    if (!application.Init())
    {
        AZ_Error("ProjectManager", false, "Failed to initialize");
        runSuccess = 1;
    }
    else
    {
        runSuccess = application.Run() ? 0 : 1;
    }

    return runSuccess;
}
