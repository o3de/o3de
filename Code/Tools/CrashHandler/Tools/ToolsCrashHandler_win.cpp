/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
