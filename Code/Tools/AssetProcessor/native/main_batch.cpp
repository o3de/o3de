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


int main(int argc, char* argv[])
{
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

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

