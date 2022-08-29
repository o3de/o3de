/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <Application.h>

int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;
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
