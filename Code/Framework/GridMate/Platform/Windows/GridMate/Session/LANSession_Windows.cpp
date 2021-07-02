/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/String/string.h>
#include <WinSock2.h>
#include <stdlib.h>

namespace GridMate
{
    namespace Platform
    {
        void AssignExtendedName(GridMate::string& extendedName)
        {
            char hostName[64];
            gethostname(hostName, AZ_ARRAY_SIZE(hostName));

            char procPath[256];
            char procName[256];
            DWORD ret = GetModuleFileName(NULL, procPath, 256);
            if (ret > 0)
            {
                ::_splitpath_s(procPath, 0, 0, 0, 0, procName, 256, 0, 0);
            }
            else
            {
                azsnprintf(procName, AZ_ARRAY_SIZE(procName), "Unknown");
            }

            extendedName = GridMate::string::format("%s::%s", hostName, procName);
        }
    }
}
