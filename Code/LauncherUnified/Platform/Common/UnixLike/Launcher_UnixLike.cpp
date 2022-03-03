/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Launcher_UnixLike.h"

#include <AzCore/base.h>
#include <AzCore/Debug/Trace.h>

#include <sys/resource.h>
#include <sys/types.h>

#include <cerrno>
#include <cstring>
#include <limits.h>
#include <stdlib.h>

namespace
{
    // return true if the limit was updated and setlimit needs to be called, false otherwise
    typedef bool(*ResourceLimitUpdater)(rlimit&);

    bool IncreaseMaxToInfinity(rlimit& limit)
    {
        if (limit.rlim_max != RLIM_INFINITY)
        {
            limit.rlim_max = RLIM_INFINITY;
            return true;
        }
        return false;
    }

    bool IncreaseCurrentToMax(rlimit& limit)
    {
        if (limit.rlim_cur < limit.rlim_max)
        {
            limit.rlim_cur = limit.rlim_max;
            return true;
        }
        return false;
    }

    bool IncreaseResourceLimit(int resource, ResourceLimitUpdater updateLimit)
    {
        rlimit limit;
        if (getrlimit(resource, &limit) != 0)
        {
            AZ_Warning("Launcher", false, "[WARNING] Unable to get limit for resource %d.  Error: %s", resource, strerror(errno));
        }

        if (updateLimit(limit))
        {
            if (setrlimit(resource, &limit) != 0)
            {
                AZ_Warning("Launcher", false, "[WARNING] Unable to update resource limit for resource %d.  Error: %s", resource, strerror(errno));
            }
        }

        return true;
    }
}


namespace O3DELauncher
{
    bool IncreaseResourceLimits()
    {
        return (IncreaseResourceLimit(RLIMIT_CORE, IncreaseMaxToInfinity)
            && IncreaseResourceLimit(RLIMIT_STACK, IncreaseCurrentToMax));
    }

    const char* GetAbsolutePath(char* absolutePathBuffer, size_t absolutePathBufferSize, const char* inputPath)
    {
        // Normalize the path
        AZ_Assert(absolutePathBufferSize>0,"Input buffer size for absolutePathBuffer must be greater than zero.");

        char normalizedFullPathBuffer[PATH_MAX];
        const char* normalizedFullPath = NULL;
        if (strlen(inputPath)>0)
        {
            normalizedFullPath = realpath(inputPath, normalizedFullPathBuffer);
        }
        if (normalizedFullPath == NULL)
        {
            // Unable to resolve the absolute path, set the buffer to blank
            absolutePathBuffer[0] = '\0';
        }
        else
        {
            // The path was resolved to an absolute path, copy to the input buffer the result
            azstrncpy(absolutePathBuffer, absolutePathBufferSize, normalizedFullPath,strlen(normalizedFullPath)+1);
        }
        return absolutePathBuffer;
    }
}
