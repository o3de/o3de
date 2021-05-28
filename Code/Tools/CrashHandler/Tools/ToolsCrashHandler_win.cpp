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

// LY Editor Crashpad Hook - Windows

#include <ToolsCrashHandler.h>
#include <AzCore/PlatformIncl.h>
#include <CrashSupport.h>

namespace CrashHandler
{
    std::string ToolsCrashHandler::GetAppRootFromCWD() const
    {
        std::string returnPath;
        GetExecutableFolder(returnPath);

        std::string devRoot{ "/dev/" };

        auto devpos = returnPath.rfind(devRoot);
        if (devpos != std::string::npos)
        {
            /* Cut off everything beyond \\dev\\ */
            returnPath.erase(devpos + devRoot.length());
        }
        return returnPath;
    }
}
