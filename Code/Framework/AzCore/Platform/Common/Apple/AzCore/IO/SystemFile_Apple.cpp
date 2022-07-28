/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/wildcard.h>
#include <../Common/UnixLike/AzCore/IO/Internal/SystemFileUtils_UnixLike.h>

#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

namespace AZ::IO::Platform
{
    void FindFiles(const char* filter, SystemFile::FindFileCB cb)
    {
        // Separate the directory from the filename portion of the filter
        AZ::IO::PathView filterPath(filter);
        AZ::IO::FixedMaxPathString filterDir{ filterPath.ParentPath().Native() };
        AZStd::string_view fileFilter{ filterPath.Filename().Native() };

        DIR* dir = opendir(filterDir.c_str());

        if (dir != nullptr)
        {
            // clear the errno state so we can distinguish between real errors and end of stream
            errno = 0;
            struct dirent* entry = readdir(dir);

            // List all the other files in the directory.
            while (entry != nullptr)
            {
                if (AZStd::wildcard_match(fileFilter, entry->d_name))
                {
                    cb(entry->d_name, (entry->d_type & DT_DIR) == 0);
                }
                entry = readdir(dir);
            }

            closedir(dir);
        }
    }
}

namespace AZ::IO::PosixInternal
{
    int Pipe(int(&pipeFileDescriptors)[2], int, OpenFlags readStatusFlags)
    {
        // Apple does not support pipe2
        // Therefore use a combination of pipe + fcntl
        // to set the read end of the pipe status flags
        const int pipeResult = pipe(pipeFileDescriptors);
        if (pipeResult == 0)
        {
            // Query the file status flags from the read end and bit-wise or the input status flags to
            const int fcntlFlags = fcntl(pipeFileDescriptors[0], F_GETFL);
            if (const int fcntlStatusResult = fcntl(pipeFileDescriptors[0], F_SETFL, fcntlFlags | static_cast<int>(readStatusFlags));
                fcntlStatusResult == -1)
            {
                // If the status flags could not be set, close the pipe and return the fcntl status result
                close(pipeFileDescriptors[1]);
                close(pipeFileDescriptors[0]);
                return fcntlStatusResult;
            }
        }
        return pipeResult;
    }
} // namespace AZ::IO::PosixInternal
