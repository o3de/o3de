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

// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "SystemUtilsApple.h"
#include <AzCore/base.h>
#include <AzCore/Debug/Trace.h>
#include <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <sys/utsname.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SystemUtilsApplePrivate
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Performs a 'safe' string copy from 'NString* source' to 'char* buffer' of 'size_t bufferLen'.
    // Returns the length of the string, or 0 if the buffer is not large enough to hold the source.
    // Copying an empty or null string will return 0, and null-terminate the buffer if possible.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CopyNSStringToBuffer(NSString* source, char* buffer, const size_t bufferLen)
    {
        if (!buffer || !bufferLen)
        {
            return 0;
        }

        if (source)
        {
            const char* src = [source UTF8String];
            const size_t srcLen = strlen(src);
            if (srcLen < bufferLen - 1)
            {
                azstrncpy(buffer, bufferLen, src, srcLen);
                buffer[srcLen] = '\0';
                return srcLen;
            }
        }

        // Could not copy the source to the destination buffer.
        buffer[0] = '\0';
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Get the path to the specified user domain directory.
    // Returns length of the string or 0 on failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    size_t GetPathToUserDirectory(NSSearchPathDirectory dir, char* buffer, const size_t bufferLen)
    {
        NSArray* userDomainDirectoryPaths = NSSearchPathForDirectoriesInDomains(dir, NSUserDomainMask, YES);
        if ([userDomainDirectoryPaths count] != 0)
        {
            NSString* userDomainDirectoryPath = static_cast<NSString*>([userDomainDirectoryPaths objectAtIndex:0]);
            return CopyNSStringToBuffer(userDomainDirectoryPath, buffer, bufferLen);
        }
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
size_t SystemUtilsApple::GetPathToApplicationBundle(char* buffer, const size_t bufferLen)
{
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    return SystemUtilsApplePrivate::CopyNSStringToBuffer(bundlePath, buffer, bufferLen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
size_t SystemUtilsApple::GetPathToApplicationExecutable(char* buffer, const size_t bufferLen)
{
    NSString* executablePath = [[NSBundle mainBundle] executablePath];
    return SystemUtilsApplePrivate::CopyNSStringToBuffer(executablePath, buffer, bufferLen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
size_t SystemUtilsApple::GetPathToApplicationResources(char* buffer, const size_t bufferLen)
{
    NSString* resourcesPath = [[NSBundle mainBundle] resourcePath];
    return SystemUtilsApplePrivate::CopyNSStringToBuffer(resourcesPath, buffer, bufferLen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
size_t SystemUtilsApple::GetPathToUserApplicationSupportDirectory(char* buffer, const size_t bufferLen)
{
    return SystemUtilsApplePrivate::GetPathToUserDirectory(NSApplicationSupportDirectory, buffer, bufferLen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
size_t SystemUtilsApple::GetPathToUserCachesDirectory(char* buffer, const size_t bufferLen)
{
    return SystemUtilsApplePrivate::GetPathToUserDirectory(NSCachesDirectory, buffer, bufferLen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
size_t SystemUtilsApple::GetPathToUserDocumentDirectory(char* buffer, const size_t bufferLen)
{
    return SystemUtilsApplePrivate::GetPathToUserDirectory(NSDocumentDirectory, buffer, bufferLen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
size_t SystemUtilsApple::GetPathToUserLibraryDirectory(char* buffer, const size_t bufferLen)
{
    return SystemUtilsApplePrivate::GetPathToUserDirectory(NSLibraryDirectory, buffer, bufferLen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
size_t SystemUtilsApple::GetUserName(char* buffer, const size_t bufferLen)
{
    return SystemUtilsApplePrivate::CopyNSStringToBuffer(NSUserName(), buffer, bufferLen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string SystemUtilsApple::GetMachineName() 
{
    utsname systemInfo;
    if (uname(&systemInfo) == -1)
    {
        return "";
    }
    return systemInfo.machine;
}
