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
#include <stdio.h>                  // for freopen
#include <unistd.h>                 // for getcwd
#include <AzCore/IO/SystemFile.h>   // for AZ_MAX_PATH_LEN

namespace AzTestRunner
{
    void set_quiet_mode()
    {
        freopen("/dev/null", "a", stdout);
    }

    const char* get_current_working_directory()
    {
        static char cwd_buffer[AZ_MAX_PATH_LEN] = { '\0' };
        return getcwd(cwd_buffer, sizeof(cwd_buffer));
    }

    void pause_on_completion()
    {
        system("pause");
    }

}
