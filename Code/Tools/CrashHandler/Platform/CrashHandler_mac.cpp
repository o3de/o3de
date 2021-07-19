/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Base Crashpad Mac implementation

#include <CrashHandler.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/SystemFile.h>

#include <mach-o/dyld.h>

namespace CrashHandler
{
    const char* crashHandlerPath = "ToolsCrashUploader";

    const char* CrashHandlerBase::GetCrashHandlerExecutableName() const
    {
        return crashHandlerPath;
    }
    
    void CrashHandlerBase::GetOSAnnotations(CrashHandlerAnnotations& annotations) const
    {
        annotations["os"] = "mac";
    }
}


