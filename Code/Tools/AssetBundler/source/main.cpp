/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(AB_BATCH_MODE)
#include <source/utils/applicationManager.h>
#else
#include <source/utils/GUIApplicationManager.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#endif

#if defined(AZ_TESTS_ENABLED)
#include <AzTest/AzTest.h>
DECLARE_AZ_UNIT_TEST_MAIN();
#endif

int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;

#if defined(AZ_TESTS_ENABLED)
    INVOKE_AZ_UNIT_TEST_MAIN();
#endif

    int runSuccess = 0;

    {
        //This nested scope is necessary as the applicationManager needs to have its destructor called BEFORE you destroy the allocators

#if defined(AB_BATCH_MODE)
        AssetBundler::ApplicationManager applicationManager(&argc, &argv);
#else 
        // Must be called before using any Qt, or the app won't be able to locate Qt libs
        AzQtComponents::PrepareQtPaths();
        AssetBundler::GUIApplicationManager applicationManager(&argc, &argv);
#endif
        if (!applicationManager.Init())
        {
            AZ_Error("AssetBundler", false, "AssetBundler initialization failed");
            runSuccess = 1;
        }
        else
        {
            runSuccess = applicationManager.Run() ? 0 : 1;
        }
    }

    return runSuccess;
}
