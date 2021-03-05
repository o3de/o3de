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