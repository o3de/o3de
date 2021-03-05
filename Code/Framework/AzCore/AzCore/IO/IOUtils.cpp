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
#include <AzCore/base.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/parallel/thread.h> /// this_thread sleep_for.

namespace AZ
{
    namespace IO
    {      
        int TranslateOpenModeToSystemFileMode(const char* path, OpenMode mode)
        {
            int systemFileMode = 0;
            bool read = AnyFlag(mode & OpenMode::ModeRead) || AnyFlag(mode & OpenMode::ModeUpdate);
            bool write = AnyFlag(mode & OpenMode::ModeWrite) || AnyFlag(mode & OpenMode::ModeUpdate) || AnyFlag(mode & OpenMode::ModeAppend);
            if (write)
            {
                // If writing the file, create the file in all cases (except r+)
                if (!SystemFile::Exists(path) && !(AnyFlag(mode & OpenMode::ModeRead) && AnyFlag(mode & OpenMode::ModeUpdate)))
                {
                    // LocalFileIO creates by default
                    systemFileMode |= SystemFile::SF_OPEN_CREATE;
                }

                if (AnyFlag(mode & OpenMode::ModeCreatePath))
                {
                    systemFileMode |= SystemFile::SF_OPEN_CREATE_PATH;
                }

                // If appending, append.
                if (AnyFlag(mode & OpenMode::ModeAppend))
                {
                    systemFileMode |= SystemFile::SF_OPEN_APPEND;
                }
                // If writing and not appending, empty the file
                else if (AnyFlag(mode & OpenMode::ModeWrite))
                {
                    systemFileMode |= SystemFile::SF_OPEN_TRUNCATE;
                }

                // If reading, set read/write, otherwise just write
                if (read)
                {
                    systemFileMode |= SystemFile::SF_OPEN_READ_WRITE;
                }
                else
                {
                    systemFileMode |= SystemFile::SF_OPEN_WRITE_ONLY;
                }
            }
            else if (read)
            {
                systemFileMode |= SystemFile::SF_OPEN_READ_ONLY;
            }

            return systemFileMode;
        }

        bool RetryOpenStream(FileIOStream& stream, int numRetries, int delayBetweenRetry)
        {
            while ((!stream.IsOpen()) && (numRetries > 0))
            {
                numRetries--;
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(delayBetweenRetry));
                stream.ReOpen();
            }
            return stream.IsOpen();
        }
    }   // namespace IO
}   // namespace AZ


