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
#include "utilities/BatchApplicationManager.h"


#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <native/tests/BaseAssetProcessorTest.h>

DECLARE_AZ_UNIT_TEST_MAIN()

int RunUnitTests(int argc, char* argv[], bool& ranUnitTests)
{
    ranUnitTests = true;

    INVOKE_AZ_UNIT_TEST_MAIN(nullptr);  // nullptr turns off default test environment used to catch stray asserts

    // This looks a bit weird, but the macro returns conditionally, so *if* we get here, it means the unit tests didn't run
    ranUnitTests = false;
    return 0;
}

int main(int argc, char* argv[])
{
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

    // If "--unittest" is present on the command line, run unit testing
    // and return immediately. Otherwise, continue as normal.
    AZ::Test::addTestEnvironment(new BaseAssetProcessorTestEnvironment());
    
    bool pauseOnComplete = false;

    if (AZ::Test::ContainsParameter(argc, argv, "--pause-on-completion"))
    {
        pauseOnComplete = true;
    }
    
    bool ranUnitTests;
    int result = RunUnitTests(argc, argv, ranUnitTests);

    if (ranUnitTests)
    {
        if (pauseOnComplete)
        {
            system("pause");
        }

        return result;
    }

    BatchApplicationManager applicationManager(&argc, &argv);
    setvbuf(stdout, NULL, _IONBF, 0); // Disabling output buffering to fix test failures due to incomplete logs

    ApplicationManager::BeforeRunStatus status = applicationManager.BeforeRun();

    if (status != ApplicationManager::BeforeRunStatus::Status_Success)
    {
        if (status == ApplicationManager::BeforeRunStatus::Status_Restarting)
        {
            //AssetProcessor will restart
            return 0;
        }
        else
        {
            //Initialization failed
            return 1;
        }
    }

    return applicationManager.Run() ? 0 : 1;
}

