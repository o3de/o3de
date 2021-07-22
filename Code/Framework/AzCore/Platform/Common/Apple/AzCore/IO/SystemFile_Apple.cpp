/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/wildcard.h>
#include <../Common/UnixLike/AzCore/IO/Internal/SystemFileUtils_UnixLike.h>

#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

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

            int lastError = errno;
            if (lastError != 0)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, filter, lastError);
            }

            closedir(dir);
        }
        else
        {
            int lastError = errno;
            if (lastError != ENOENT)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, filter, 0);
            }
        }
    }
}
