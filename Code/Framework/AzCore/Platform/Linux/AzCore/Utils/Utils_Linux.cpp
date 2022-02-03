/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/fixed_string.h>

#include <unistd.h>

namespace AZ::Utils
{
    GetExecutablePathReturnType GetExecutablePath(char* exeStorageBuffer, size_t exeStorageSize)
    {
        GetExecutablePathReturnType result;
        result.m_pathIncludesFilename = true;

        // http://man7.org/linux/man-pages/man5/proc.5.html
        const ssize_t bytesWritten = readlink("/proc/self/exe", exeStorageBuffer, exeStorageSize);
        if (bytesWritten == -1)
        {
            result.m_pathStored = ExecutablePathResult::GeneralError;
        }
        else if (bytesWritten == exeStorageSize)
        {
            result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
        }
        else
        {
            // readlink doesn't null terminate
            exeStorageBuffer[bytesWritten] = '\0';
        }

        return result;
    }

    AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultAppRootPath()
    {
        return AZStd::nullopt;
    }

    AZStd::optional<AZ::IO::FixedMaxPathString> GetDevWriteStoragePath()
    {
        return AZStd::nullopt;
    }
} // namespace AZ::Utils
