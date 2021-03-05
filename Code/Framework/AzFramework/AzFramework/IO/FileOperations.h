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
#pragma once
#ifndef CRYCOMMON_FILEOPERATIONS_H
#define CRYCOMMON_FILEOPERATIONS_H

#include <AzCore/IO/FileIO.h>

namespace AZ
{
    namespace IO
    {
        int64_t Print(HandleType fileHandle, const char* format, ...);
        int64_t PrintV(HandleType fileHandle, const char* format, va_list arglist);
        AZ::IO::Result Move(const char* sourceFilePath, const char* destinationFilePath);
        /**
        * This function only tries to move/copy the sourceFile to the destination file
        * This will only return success if it was able to move/copy the sourceFile to the destination file.
        **/
        AZ::IO::Result SmartMove(const char* sourceFile, const char* destinationFile);
        /**
        * Generate an unused filename from an input file name
        * by prepending "$tmpN_" to it (N is a random integer).
        * SourceFile must be an absolute path, it can have alias in the path. 
        * Returns false if unable to find an unused name despite multiple attempts.
        **/
        bool CreateTempFileName(const char* file, AZStd::string& tempFile);
        int GetC(HandleType fileHandle);
        int UnGetC(int character, HandleType fileHandle);
        char* FGetS(char* buffer, uint64_t bufferSize, HandleType fileHandle);
        int FPutS(const char* buffer, HandleType fileHandle);
    }
}

#endif // #ifndef CRYCOMMON_FILEOPERATIONS_H