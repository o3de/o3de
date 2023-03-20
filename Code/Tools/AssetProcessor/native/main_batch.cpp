/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "utilities/BatchApplicationManager.h"


int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;
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

