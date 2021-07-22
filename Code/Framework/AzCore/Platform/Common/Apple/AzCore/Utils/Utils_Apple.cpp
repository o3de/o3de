/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/PlatformIncl.h>
#include <mach-o/dyld.h>

namespace AZ
{
    namespace Utils
    {
        GetExecutablePathReturnType GetExecutablePath(char* exeStorageBuffer, size_t exeStorageSize)
        {
            GetExecutablePathReturnType result;
            result.m_pathIncludesFilename = true;
            // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/dyld.3.html
            auto bufSize = static_cast<uint32_t>(exeStorageSize);
            const int getExecutableResultCode = _NSGetExecutablePath(exeStorageBuffer, &bufSize);
            if (getExecutableResultCode == -1)
            {
                result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
            }

            return result;
        }
    }
}
