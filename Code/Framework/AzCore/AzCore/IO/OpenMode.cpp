/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/OpenMode.h>
#include <AzCore/IO/SystemFile.h>

namespace AZ::IO
{
    OpenMode GetOpenModeFromStringMode(const char* mode)
    {
        OpenMode openMode = OpenMode::Invalid;

        if (strstr(mode, "w"))
        {
            openMode |= OpenMode::ModeWrite;
        }

        if (strstr(mode, "r"))
        {
            openMode |= OpenMode::ModeRead;
        }

        if (strstr(mode, "a"))
        {
            openMode |= OpenMode::ModeAppend;
        }

        if (strstr(mode, "b"))
        {
            openMode |= OpenMode::ModeBinary;
        }

        if (strstr(mode, "t"))
        {
            openMode |= OpenMode::ModeText;
        }

        if (strstr(mode, "+"))
        {
            openMode |= OpenMode::ModeUpdate;
        }

        UpdateOpenModeForReading(openMode);

        return openMode;
    }

    const char* GetStringModeFromOpenMode(OpenMode mode)
    {
        UpdateOpenModeForReading(mode);
        // Append is highest priority, followed by write and then read
        // APPEND
        if (AnyFlag(mode & OpenMode::ModeAppend))
        {
            if (AnyFlag(mode & OpenMode::ModeUpdate))
            {
                if (AnyFlag(mode & OpenMode::ModeBinary))
                {
                    return "a+b";
                }
                if (AnyFlag(mode & OpenMode::ModeText))
                {
                    return "a+t";
                }
                return "a+";
            }
            if (AnyFlag(mode & OpenMode::ModeBinary))
            {
                return "ab";
            }
            if (AnyFlag(mode & OpenMode::ModeText))
            {
                return "at";
            }
            return "a";
        }

        // WRITE
        if (AnyFlag(mode & OpenMode::ModeWrite))
        {
            if (AnyFlag(mode & OpenMode::ModeUpdate))
            {
                if (AnyFlag(mode & OpenMode::ModeBinary))
                {
                    return "w+b";
                }
                if (AnyFlag(mode & OpenMode::ModeText))
                {
                    return "w+t";
                }
                return "w+";
            }
            if (AnyFlag(mode & OpenMode::ModeBinary))
            {
                return "wb";
            }
            if (AnyFlag(mode & OpenMode::ModeText))
            {
                return "wt";
            }
            return "w";
        }

        // READ
        if (AnyFlag(mode & OpenMode::ModeRead))
        {
            if (AnyFlag(mode & OpenMode::ModeUpdate))
            {
                if (AnyFlag(mode & OpenMode::ModeBinary))
                {
                    return "r+b";
                }
                if (AnyFlag(mode & OpenMode::ModeText))
                {
                    return "r+t";
                }
                return "r+";
            }
            if (AnyFlag(mode & OpenMode::ModeBinary))
            {
                return "rb";
            }
            if (AnyFlag(mode & OpenMode::ModeText))
            {
                return "rt";
            }
            return "r";
        }

        // Bad open mode passed in
        AZ_Error("FileIO", false, "A bad open mode was sent to GetStringModeFromOpenMode()");
        return "";
    }

    void UpdateOpenModeForReading(OpenMode& openMode)
    {
        if (AnyFlag(openMode & OpenMode::ModeRead))
        {
            if (AnyFlag(openMode & OpenMode::ModeText))
            {
                OpenMode extraModes = openMode & (OpenMode::ModeUpdate | OpenMode::ModeAppend);
                openMode = OpenMode::ModeRead | OpenMode::ModeBinary | extraModes;
            }
            else if (!AnyFlag(openMode & OpenMode::ModeBinary))
            {
                // if you haven't supplied any flag, supply binary
                openMode = openMode | OpenMode::ModeBinary;
            }
        }
    }

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
} // namespace AZ::IO
