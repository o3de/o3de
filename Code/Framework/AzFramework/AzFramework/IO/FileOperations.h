/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzFramework/AzFrameworkAPI.h>

namespace AZ
{
    namespace IO
    {
        AZF_API int64_t Print(HandleType fileHandle, const char* format, ...);
        AZF_API int64_t PrintV(HandleType fileHandle, const char* format, va_list arglist);
        AZF_API AZ::IO::Result Move(const char* sourceFilePath, const char* destinationFilePath);
        /**
        * This function only tries to move/copy the sourceFile to the destination file
        * This will only return success if it was able to move/copy the sourceFile to the destination file.
        **/
        AZF_API AZ::IO::Result SmartMove(const char* sourceFile, const char* destinationFile);
        /**
        * Generate an unused filename from an input file name
        * by prepending "$tmpN_" to it (N is a random integer).
        * SourceFile must be an absolute path, it can have alias in the path. 
        * Returns false if unable to find an unused name despite multiple attempts.
        **/
        AZF_API bool CreateTempFileName(const char* file, AZStd::string& tempFile);
        AZF_API int GetC(HandleType fileHandle);
        AZF_API int UnGetC(int character, HandleType fileHandle);
        AZF_API char* FGetS(char* buffer, uint64_t bufferSize, HandleType fileHandle);
        AZF_API int FPutS(const char* buffer, HandleType fileHandle);
    }
}
