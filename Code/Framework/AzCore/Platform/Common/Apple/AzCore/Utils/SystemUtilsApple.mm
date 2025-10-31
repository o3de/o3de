/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SystemUtilsApple.h"
#include <AzCore/base.h>
#include <AzCore/Debug/Trace.h>
#include <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <sys/utsname.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ::SystemUtilsApple::Private
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Performs a 'safe' string copy from 'NString* source' to 'char* buffer' of 'size_t bufferLen'.
    // Returns string_view pointing to a copy of the string in the buffer on success,
    // or an AppSupportErrorResult instance detailing the error on failure
    // The buffer will be null-terminated if possible.
    // However since a string_view of the data is returned, the buffer doesn't explicitly need to be
    ////////////////////////////////////////////////////////////////////////////////////////////////
    static AppSupportOutcome CopyNSStringToBuffer(NSString* source, AZStd::span<char> buffer)
    {
        if (source == nullptr)
        {
            return AZ::Failure(AppSupportErrorResult{ AppSupportErrorCode::PathNotAvailable });
        }

        AZStd::string_view src = [source UTF8String];

        if (src.size() > buffer.size())
        {
            return AZ::Failure(AppSupportErrorResult{ AppSupportErrorCode::BufferTooSmall, src.size() });
        }


        const size_t copyCount = src.copy(buffer.data(), src.size());
        // Null-terminate the buffer if possible
        if (copyCount < buffer.size())
        {
            buffer[copyCount] = '\0';
        }
        return AZStd::string_view(buffer.data(), copyCount);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Get the path to the specified user domain directory.
    // Returns length of the string or 0 on failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    AppSupportOutcome GetPathToUserDirectory(NSSearchPathDirectory dir, AZStd::span<char> buffer)
    {
        NSArray* userDomainDirectoryPaths = NSSearchPathForDirectoriesInDomains(dir, NSUserDomainMask, YES);
        if ([userDomainDirectoryPaths count] != 0)
        {
            NSString* userDomainDirectoryPath = static_cast<NSString*>([userDomainDirectoryPaths objectAtIndex:0]);
            return CopyNSStringToBuffer(userDomainDirectoryPath, buffer);
        }
        return AZ::Failure(AppSupportErrorResult{ AppSupportErrorCode::PathNotAvailable });
    }
}

namespace AZ::SystemUtilsApple
{
    AppSupportOutcome GetPathToApplicationBundle(AZStd::span<char> buffer)
    {
        NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
        return Private::CopyNSStringToBuffer(bundlePath, buffer);
    }

    AppSupportOutcome GetPathToApplicationExecutable(AZStd::span<char> buffer)
    {
        NSString* executablePath = [[NSBundle mainBundle] executablePath];
        return Private::CopyNSStringToBuffer(executablePath, buffer);
    }

    AppSupportOutcome GetPathToApplicationFrameworks(AZStd::span<char> buffer)
    {
        NSString* frameworksPath = [[NSBundle mainBundle] privateFrameworksPath];
        return Private::CopyNSStringToBuffer(frameworksPath, buffer);
    }

    AppSupportOutcome GetPathToApplicationResources(AZStd::span<char> buffer)
    {
        NSString* resourcesPath = [[NSBundle mainBundle] resourcePath];
        return Private::CopyNSStringToBuffer(resourcesPath, buffer);
    }

    AppSupportOutcome GetPathToUserApplicationSupportDirectory(AZStd::span<char> buffer)
    {
        return Private::GetPathToUserDirectory(NSApplicationSupportDirectory, buffer);
    }

    AppSupportOutcome GetPathToUserCachesDirectory(AZStd::span<char> buffer)
    {
        return Private::GetPathToUserDirectory(NSCachesDirectory, buffer);
    }

    AppSupportOutcome GetPathToUserDocumentDirectory(AZStd::span<char> buffer)
    {
        return Private::GetPathToUserDirectory(NSDocumentDirectory, buffer);
    }

    AppSupportOutcome GetPathToUserLibraryDirectory(AZStd::span<char> buffer)
    {
        return Private::GetPathToUserDirectory(NSLibraryDirectory, buffer);
    }

    AppSupportOutcome GetUserName(AZStd::span<char> buffer)
    {
        return Private::CopyNSStringToBuffer(NSUserName(), buffer);
    }

} // namespace AZ::SystemUtilsApple
