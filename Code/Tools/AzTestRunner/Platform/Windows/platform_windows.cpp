/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <sstream>                  // for freopen_s
#include <direct.h>                 // for _getcwd
#include <AzCore/IO/SystemFile.h>   // for AZ_MAX_PATH_LEN

namespace AzTestRunner
{
    void set_quiet_mode()
    {
        FILE* stream;
        freopen_s(&stream, "nul", "w", stdout);
    }

    const char* get_current_working_directory()
    {
        static char cwd_buffer[AZ_MAX_PATH_LEN] = { '\0' };
        return _getcwd(cwd_buffer, sizeof(cwd_buffer));
    }

    void pause_on_completion()
    {
        system("pause");
    }

}
