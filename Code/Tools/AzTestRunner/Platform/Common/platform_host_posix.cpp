/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <stdio.h>                  // for freopen
#include <unistd.h>                 // for getcwd
#include <AzCore/IO/SystemFile.h>   // for AZ_MAX_PATH_LEN

namespace AzTestRunner
{
    void set_quiet_mode()
    {
        [[maybe_unused]] auto freopenResult = freopen("/dev/null", "a", stdout);
    }

    const char* get_current_working_directory()
    {
        static char cwd_buffer[AZ_MAX_PATH_LEN] = { '\0' };
        return getcwd(cwd_buffer, sizeof(cwd_buffer));
    }

    void pause_on_completion()
    {
        [[maybe_unused]] auto systemResult = system("pause");
    }

}
